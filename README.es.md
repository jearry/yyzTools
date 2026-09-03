<p align="center">
  <img src="docs/logo.png" width="120" alt="yyzTools">
</p>

<h1 align="center">yyzTools</h1>

<p align="center">
  <strong>Yes Your Zen Tools</strong>
</p>

<p align="center"><a href="README.md">English</a> · <a href="README.zh-CN.md">简体中文</a> · <a href="README.zh-TW.md">繁體中文</a> · <a href="README.ja.md">日本語</a> · <a href="README.ko.md">한국어</a> · <a href="README.fr.md">Français</a> · <a href="README.de.md">Deutsch</a> · <strong>Español</strong> · <a href="README.ru.md">Русский</a> · <a href="README.ar.md">العربية</a> · <a href="README.pt.md">Português</a> · <a href="README.it.md">Italiano</a></p>

<p align="center">
  <img alt="version" src="https://img.shields.io/badge/version-1.0.5.1500-blue">
  <img alt="platform" src="https://img.shields.io/badge/platform-Windows%2010%2F11%20x64-lightgrey">
  <img alt="languages" src="https://img.shields.io/badge/UI-12%20languages-green">
  <img alt="license" src="https://img.shields.io/badge/license-gratis-red">
</p>

---

## Introducción

Buscar, traducir, reconocer, previsualizar, procesar por lotes, capturar pantalla, personalizar el fondo: todas esas tareas cotidianas, frecuentes y menudas, sin tener que buscar, instalar y pagar una extensión distinta para cada una. Instale yyzTools una vez y tendrá a mano lo que necesite.

## Características

- **Gratis para siempre** — Todas las funciones están abiertas. Sin desbloqueos de pago, sin niveles de suscripción y sin recortar lo esencial para luego cobrárselo.
- **Prioridad local** — El portapapeles, la búsqueda de archivos, las estadísticas de uso y el OCR se procesan y guardan en su propio equipo. Nada se envía por iniciativa propia.
- **Una sola instalación** — Más de 40 módulos listos para usar, sin el habitual ciclo de buscar, comparar, descargar, probar y desinstalar.
- **379 módulos de comandos** — Comandos de IA, aplicaciones, juegos, sistema y sitios web siempre a mano, y basta un archivo JSON para añadir los suyos.

## Funciones

| | |
|---|---|
| **Paleta de comandos** | Búsqueda de aplicaciones · Búsqueda de archivos · Gestión de procesos · Gestión de ventanas · Búsqueda de marcadores · Historial del portapapeles · Línea de comandos · Cálculo instantáneo |
| **Inicio y eficiencia** | Dock · Historial del portapapeles · Estadísticas de uso · Captura de pantalla · Grabación de pantalla · Asistente de recordatorios · Calendario perpetuo · Indicaciones de teclado · Localizador del ratón |
| **Texto e idiomas** | Traducción avanzada · Reconocimiento de texto OCR · Expresiones regulares · Codificar y decodificar · Cálculo de hash · Cifrado y descifrado · Conversión de datos · Generación de datos |
| **Archivos y multimedia** | Vista previa de archivos · Renombrado por lotes · Procesamiento por lotes de imágenes · Procesamiento por lotes de vídeo · Procesamiento por lotes de PDF · Descarga de archivos |
| **Cálculo y extensiones** | Hoja de cálculo libre · Códigos QR · Módulos de comandos · Navegador integrado · Extensiones propias |
| **Personalización** | Fondo de pantalla dinámico · Efectos de escritorio · Modo juego · Página de navegación · Defensa de la Tierra |

## Arquitectura

Híbrida: un **host C++ (Win32 + WebView2)** que alberga un **front end (Alpine.js + JS nativo + Vite)**, con áreas funcionales aisladas en procesos separados.

| Proceso | Función |
|---------|---------|
| `yyzTools.exe` | Proceso principal — aloja la paleta de comandos, la traducción, el OCR y la mayoría de las funciones |
| `yyzWallpaper.exe` | Fondo dinámico — compone web/vídeo bajo la capa de iconos del escritorio mediante la API de Windows Composition |
| `yyzBrowser.exe` | Navegador integrado sin marco para que los módulos de comandos abran URL |
| `yyzCmd.exe` | Ejecutor de comandos en Win32 puro (apagado / reinicio / volumen / pantalla / papelera, etc.) |
| `yyzInputHint.exe` | Indicación en tiempo real de teclas de teclado / ratón (RawInput) |
| `yyzMouseFinder.exe` | Localización rápida del cursor del ratón |
| `yyzUpdater.exe` | Actualizador automático incremental (ver `src/updater/`) |
| `yyzFileSearch.exe` | Motor de búsqueda de archivos propio — lectura directa de la MFT + índice incremental USN |

El front end llama a cada Manager de C++ a través del puente `window.Zen`; todos responden con el contrato `{ error, ... }`.

## Descarga

- **Instalador** (recomendado): `yyzTools-setup-1.0.5.1500.exe` —— [GitHub Releases](https://github.com/jearry/yyzTools/releases)
- ¿Lento desde China? Anteponga al URL del instalador un espejo: [ghfast.top](https://ghfast.top/) · [ghproxy.com](https://ghproxy.com/) · [gh-proxy.com](https://gh-proxy.com/)
- También puede obtenerlo en la [página oficial de descargas](https://yyztools.com/es/download.html).

El asistente de instalación admite 12 idiomas: chino simplificado y tradicional, inglés, japonés, coreano, francés, alemán, español, ruso, árabe, portugués e italiano.

## Requisitos del sistema

- Windows 10 / 11 (x64)
- WebView2 Runtime (preinstalado en sistemas recientes; el instalador lo incorpora si falta)

## Idiomas de la interfaz

Tanto la interfaz como los textos de los módulos de comandos admiten los 12 idiomas mencionados. El primer inicio detecta automáticamente el idioma del sistema; puede cambiarlo en cualquier momento en la configuración.

## Acerca de este repositorio

Este repositorio (`jearry/yyzTools`) es el **repositorio de publicación y alojamiento del sitio oficial** de yyzTools:

- [`releases/`](releases/) —— instaladores versionados y archivos comprimidos divididos (`.7z`), además del manifiesto de actualización automática `update.json`
- [`docs/`](docs/) —— sitio estático de <https://yyztools.com> (alojado en GitHub Pages)
- [`src/updater/`](src/updater/) —— código fuente del actualizador automático

Para informar de problemas de funcionalidad, utilice [Issues](https://github.com/jearry/yyzTools/issues).

## Licencia y agradecimientos

yyzTools es software propietario de uso gratuito. Por ahora solo se ha publicado como código abierto el módulo de actualización automática, bajo la **licencia MIT**. La apertura del código de otras partes o de la aplicación completa se decidirá según las circunstancias.

Este software utiliza varias bibliotecas y herramientas de código abierto. Consulte [LICENSE](LICENSE) para ver la lista completa de componentes de terceros y sus respectivas licencias.

¡Agradecemos sinceramente a todos los desarrolladores que han contribuido a estos proyectos de código abierto!
