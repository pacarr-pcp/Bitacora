# Bitácora

Bitácora de equipo, simple y gratuita, para dejar registro de eventos (fallas,
novedades, notas de terreno, etc.) sin depender de que alguien se acuerde a fin
de mes. Corre 100% sobre **Google Sheets + Google Apps Script** — sin
servidores ni costos.

## Cómo funciona

- Un único Google Sheet actúa como base de datos.
- Un Web App de Apps Script sirve el formulario y la vista de consulta.
- Cada persona se identifica con un código propio (el mismo que ya usa en otra
  app), escaneándolo con la cámara o tipeándolo a mano. Quien no tiene código
  puede cargar un registro igual, escribiendo su nombre ("invitado").
- Los registros importantes disparan un correo automático a los encargados.

## Estructura del Google Sheet

Creá (o reutilizá) un Sheet con estas dos pestañas exactas:

### Pestaña `Usuarios`

| Nombre | Código | Activo | Encargado | Correo |
|---|---|---|---|---|
| Juan Pérez | 1234 | ☑ | ☐ | |
| María Gómez | 5678 | ☑ | ☑ | maria@empresa.com |

- **Activo**: checkbox. Vacío o tildado = activo. Solo destildado (`FALSE`)
  desactiva el código.
- **Encargado**: checkbox. Las filas tildadas reciben el email cuando se carga
  un registro con etiqueta `Importante`.
- **Correo**: solo se usa si `Encargado` está tildado.

### Pestaña `Registros`

| ID | Timestamp | Código | Usuario | Etiqueta | Descripción |
|---|---|---|---|---|---|

Esta pestaña se completa sola desde la app — no hace falta escribir nada a
mano ahí, alcanza con crearla vacía con esos encabezados.

## Instalación

1. Abrí tu Google Sheet → **Extensiones → Apps Script**.
2. Se abre un editor con un archivo `Código.gs` por defecto. Borrá su
   contenido y pegá el de [`apps-script/Code.gs`](apps-script/Code.gs) de este
   repo.
3. Creá un archivo HTML nuevo (ícono `+` → HTML) llamado exactamente `Index`.
   Pegá el contenido de [`apps-script/Index.html`](apps-script/Index.html).
4. Abrí el archivo de manifiesto (ícono de engranaje → tildar "Mostrar archivo
   de manifiesto") y reemplazá `appsscript.json` por el contenido de
   [`apps-script/appsscript.json`](apps-script/appsscript.json). Si tu zona
   horaria no es Argentina, ajustá el campo `timeZone`.
5. Guardá todo (Ctrl+S / ⌘+S).

## Desplegar como Web App

1. Arriba a la derecha → **Implementar → Nueva implementación**.
2. Tipo: **Aplicación web**.
3. "Ejecutar como": **Yo** (tu cuenta, la dueña del Sheet).
4. "Quién tiene acceso": **Cualquier usuario** (así entra cualquiera con el
   link, sin necesitar cuenta de Google — la identificación la maneja el
   código interno, no el login de Google).
5. Implementar → copiá la URL que te da. Esa es la URL de la bitácora.
6. Generá un QR apuntando a esa URL (con cualquier generador gratuito online)
   y ponelo donde te convenga — es un único módulo para todo el equipo.

Cada vez que edites el código en el editor de Apps Script, tenés que hacer
**Implementar → Gestionar implementaciones → editar (lápiz) → Nueva versión**
para que los cambios se reflejen en la URL ya publicada.

## Uso

- **Registrar**: identificarse con código (escaneado o tipeado) o como
  invitado con nombre, elegir una etiqueta (`Asistencia`, `Importante`,
  `Anonimo`, `Seguridad`, `Terreno`) y escribir la descripción.
- **Revisar**: bitácora compartida, muestra los últimos 30 días con filtro por
  etiqueta. Si ponés tu código ahí también, vas a poder borrar tus propios
  registros durante la primera hora después de cargados.

## Limitaciones conocidas (a propósito, por simplicidad)

- El código no es un login real: cualquiera que lo conozca puede cargar a tu
  nombre. Es un control liviano pensado para uso interno de confianza, no
  autenticación fuerte.
- Los registros cargados como "invitado" no se pueden borrar (no hay código
  que pruebe que son tuyos).
- El escaneo de QR necesita permiso de cámara del navegador; si el usuario lo
  rechaza, siempre queda la opción de tipear el código a mano.

## Próximos pasos posibles

- Reportes/resúmenes mensuales automáticos.
- Adjuntar fotos a un registro.
- Exportar a PDF.

Estas ideas quedan afuera de esta primera versión a propósito, para no
sumar complejidad antes de validar el uso real.
