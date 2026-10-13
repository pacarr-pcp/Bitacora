/**
 * Bitácora de equipo — backend (Google Apps Script)
 *
 * Este script debe vivir DENTRO del Google Sheet (Extensiones > Apps Script),
 * así SpreadsheetApp.getActiveSpreadsheet() apunta directo a esa hoja.
 *
 * Hojas esperadas en el spreadsheet:
 *
 *  Usuarios (primera hoja, cualquiera sea su nombre) → Nombre | Código | Activo | Encargado | Correo
 *    - Activo:    checkbox. Vacío o TRUE = activo. Solo FALSE explícito lo desactiva.
 *    - Encargado: checkbox. Solo las filas en TRUE reciben alertas de "Importante".
 *    - Correo:    email del encargado (solo se usa si Encargado = TRUE).
 *
 *  "Registros" → ID | Timestamp | Código | Usuario | Etiqueta | Descripción
 *    - Se completa sola, no hace falta tocarla a mano.
 */

const HOJA_USUARIOS = 'Usuarios';
const HOJA_REGISTROS = 'Registros';
const ETIQUETAS = ['Asistencia', 'Importante', 'Anonimo', 'Seguridad', 'Terreno'];
const ETIQUETA_ALERTA = 'Importante';
const VENTANA_BORRADO_MS = 60 * 60 * 1000; // 1 hora
const DIAS_HISTORIAL = 30;

function doGet() {
  return HtmlService.createHtmlOutputFromFile('Index')
    .setTitle('Bitácora')
    .addMetaTag('viewport', 'width=device-width, initial-scale=1')
    .setXFrameOptionsMode(HtmlService.XFrameOptionsMode.ALLOWALL);
}

function obtenerEtiquetas() {
  return ETIQUETAS;
}

// Libro "Bitacora". Se usa por ID por si el script no está vinculado al libro.
const ID_LIBRO = '1n95G24oj5YDXh8A2hj_Y-MgQi-D-y_kaCPhjnTCIi_0';

function getLibro_() {
  return SpreadsheetApp.getActiveSpreadsheet() || SpreadsheetApp.openById(ID_LIBRO);
}

function getSheet_(nombre) {
  const libro = getLibro_();
  // Los usuarios están siempre en la primera hoja, sin importar cómo se llame la pestaña.
  if (nombre === HOJA_USUARIOS) return libro.getSheets()[0];
  const hoja = libro.getSheetByName(nombre);
  if (!hoja) throw new Error('No existe la hoja "' + nombre + '". Revisa el nombre exacto de la pestaña.');
  return hoja;
}

function buscarUsuario_(codigo) {
  if (!codigo) return null;
  const datos = getSheet_(HOJA_USUARIOS).getDataRange().getValues();
  for (let i = 1; i < datos.length; i++) {
    const [nombre, cod, activo] = datos[i];
    if (!cod) continue;
    if (String(cod).trim().toLowerCase() === String(codigo).trim().toLowerCase()) {
      if (activo === false) return null;
      return { nombre: String(nombre).trim(), codigo: String(cod).trim() };
    }
  }
  return null;
}

/** Llamada desde el cliente para validar un código antes de habilitar el formulario. */
function validarCodigo(codigo) {
  const usuario = buscarUsuario_(codigo);
  return usuario ? { valido: true, nombre: usuario.nombre } : { valido: false };
}

function obtenerCorreosEncargados_() {
  const datos = getSheet_(HOJA_USUARIOS).getDataRange().getValues();
  const correos = [];
  for (let i = 1; i < datos.length; i++) {
    const [, , , encargado, correo] = datos[i];
    if (encargado === true && correo) correos.push(String(correo).trim());
  }
  return correos;
}

function enviarAlerta_(registro) {
  const correos = obtenerCorreosEncargados_();
  if (correos.length === 0) return;
  const asunto = '⚠️ Bitácora — registro IMPORTANTE';
  const cuerpo = [
    'Se cargó un nuevo registro importante en la bitácora:',
    '',
    'Fecha: ' + registro.timestampTexto,
    'Usuario: ' + registro.usuario,
    'Descripción: ' + registro.descripcion
  ].join('\n');
  MailApp.sendEmail({ to: correos.join(','), subject: asunto, body: cuerpo });
}

/** data = { codigo, nombreInvitado, etiqueta, descripcion } */
function registrarLog(data) {
  const codigo = String(data.codigo || '').trim();
  const nombreInvitado = String(data.nombreInvitado || '').trim();
  const etiqueta = data.etiqueta;
  const descripcion = String(data.descripcion || '').trim();

  if (ETIQUETAS.indexOf(etiqueta) === -1) throw new Error('Etiqueta inválida.');
  if (!descripcion) throw new Error('Falta la descripción.');

  let usuario, codigoFinal;
  if (codigo) {
    const encontrado = buscarUsuario_(codigo);
    if (!encontrado) throw new Error('Código no reconocido.');
    usuario = encontrado.nombre;
    codigoFinal = encontrado.codigo;
  } else {
    if (!nombreInvitado) throw new Error('Ingresa tu nombre.');
    usuario = nombreInvitado;
    codigoFinal = '';
  }

  const hoja = getSheet_(HOJA_REGISTROS);
  const id = Utilities.getUuid();
  const fecha = new Date();
  hoja.appendRow([id, fecha, codigoFinal, usuario, etiqueta, descripcion]);

  const registro = {
    id: id,
    timestamp: fecha.getTime(),
    timestampTexto: Utilities.formatDate(fecha, Session.getScriptTimeZone(), 'dd/MM/yyyy HH:mm'),
    codigo: codigoFinal,
    usuario: usuario,
    etiqueta: etiqueta,
    descripcion: descripcion
  };

  if (etiqueta === ETIQUETA_ALERTA) {
    enviarAlerta_(registro);
  }

  return registro;
}

/** Devuelve los registros de los últimos DIAS_HISTORIAL días, más recientes primero. */
function listarRegistros(etiquetaFiltro) {
  const datos = getSheet_(HOJA_REGISTROS).getDataRange().getValues();
  const limite = new Date();
  limite.setDate(limite.getDate() - DIAS_HISTORIAL);

  const resultado = [];
  for (let i = 1; i < datos.length; i++) {
    const [id, timestamp, codigo, usuario, etiqueta, descripcion] = datos[i];
    if (!id) continue;
    const fecha = new Date(timestamp);
    if (fecha < limite) continue;
    if (etiquetaFiltro && etiquetaFiltro !== 'Todas' && etiqueta !== etiquetaFiltro) continue;

    resultado.push({
      id: id,
      timestamp: fecha.getTime(),
      timestampTexto: Utilities.formatDate(fecha, Session.getScriptTimeZone(), 'dd/MM/yyyy HH:mm'),
      codigo: codigo,
      usuario: usuario,
      etiqueta: etiqueta,
      descripcion: descripcion
    });
  }
  resultado.sort((a, b) => b.timestamp - a.timestamp);
  return resultado;
}

function borrarRegistro(id, codigo) {
  if (!codigo) throw new Error('Necesitás tu código para borrar un registro.');
  const hoja = getSheet_(HOJA_REGISTROS);
  const datos = hoja.getDataRange().getValues();

  for (let i = 1; i < datos.length; i++) {
    if (datos[i][0] !== id) continue;

    const [, timestamp, codigoFila] = datos[i];
    if (String(codigoFila || '').trim() !== String(codigo).trim()) {
      throw new Error('Ese registro no te pertenece.');
    }
    if (Date.now() - new Date(timestamp).getTime() > VENTANA_BORRADO_MS) {
      throw new Error('Ya pasó más de 1 hora desde que se cargó, no se puede borrar.');
    }
    hoja.deleteRow(i + 1);
    return { ok: true };
  }
  throw new Error('Registro no encontrado (puede que ya lo hayan borrado).');
}
