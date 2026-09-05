<p align="center">
  <img src="docs/logo.png" width="120" alt="yyzTools">
</p>

<h1 align="center">yyzTools</h1>

<p align="center">
  <strong>Yes Your Zen Tools</strong>
</p>

<p align="center"><a href="README.md">English</a> · <a href="README.zh-CN.md">简体中文</a> · <a href="README.zh-TW.md">繁體中文</a> · <a href="README.ja.md">日本語</a> · <a href="README.ko.md">한국어</a> · <a href="README.fr.md">Français</a> · <a href="README.de.md">Deutsch</a> · <a href="README.es.md">Español</a> · <a href="README.ru.md">Русский</a> · <a href="README.ar.md">العربية</a> · <strong>Português</strong> · <a href="README.it.md">Italiano</a></p>

<p align="center">
  <img alt="version" src="https://img.shields.io/badge/version-1.0.6.1650-blue">
  <img alt="platform" src="https://img.shields.io/badge/platform-Windows%2010%2F11%20x64-lightgrey">
  <img alt="languages" src="https://img.shields.io/badge/UI-12%20languages-green">
  <img alt="license" src="https://img.shields.io/badge/license-Grátis-red">
</p>

---

## Introdução

Pesquisar, traduzir, reconhecer texto, pré-visualizar, processar em lote, capturar ecrã, papéis de parede — as pequenas tarefas frequentes do dia a dia, sem ter de procurar, instalar e pagar por uma ferramenta separada para cada uma. Instale o yyzTools uma vez e o que precisar já está à mão.

## Destaques

- **Grátis para sempre** — Todas as funcionalidades abertas: sem desbloqueios pagos, sem sistema de membros e sem cobranças disfarçadas através de limitações nas capacidades essenciais.
- **Prioridade ao local** — Área de transferência, pesquisa de ficheiros, estatísticas de utilização e OCR são processados e guardados no seu próprio computador; nada é enviado automaticamente.
- **Uma única instalação** — Mais de 40 módulos prontos a usar, poupando todo o ciclo de pesquisar, comparar, descarregar, testar e desinstalar.
- **379 módulos de comandos** — Comandos de cinco categorias (IA, aplicações, jogos, sistema e sites) prontos a usar; basta um ficheiro JSON para adicionar os seus.

## Visão Geral das Funcionalidades

| | |
|---|---|
| **Painel de comandos** | Pesquisa de aplicações · Pesquisa de ficheiros · Gestão de processos · Gestão de janelas · Pesquisa de marcadores · Histórico da área de transferência · Linha de comandos · Calculadora instantânea |
| **Arranque e produtividade** | Dock · Histórico da área de transferência · Estatísticas de aplicações · Captura de ecrã · Gravação de ecrã · Assistente de lembretes · Calendário permanente · Indicação de teclas · Localização do rato |
| **Texto e idiomas** | Super tradução · Reconhecimento de texto OCR · Expressões regulares · Codificar/decodificar · Cálculo de hash · Encriptar/desencriptar · Conversão de dados · Geração de dados |
| **Ficheiros e multimédia** | Pré-visualização de ficheiros · Renomear em lote · Processamento de imagens em lote · Processamento de vídeo em lote · Processamento de PDF em lote · Descarga de ficheiros |
| **Cálculo e extensões** | Rascunho de cálculo · Código QR · Módulos de comandos · Browser integrado · Extensões personalizadas |
| **Personalização** | Papéis de parede dinâmicos · Efeitos de ambiente de trabalho · Modo de jogo · Página de navegação · Defesa da Terra |

## Arquitetura

Arquitetura híbrida: um **host C++ (Win32 + WebView2)** que aloja o **frontend (Alpine.js + JS nativo + Vite)**, com isolamento por processos.

| Processo | Responsabilidade |
|----------|------------------|
| `yyzTools.exe` | Processo principal — aloja o painel de comandos, a tradução, o OCR e a maioria das funcionalidades |
| `yyzWallpaper.exe` | Papel de parede dinâmico, fixado sob a camada de ícones do ambiente de trabalho através da Windows Composition API |
| `yyzBrowser.exe` | Browser integrado sem moldura, usado pelos módulos de comandos para abrir URLs |
| `yyzCmd.exe` | Executor de comandos em Win32 puro (encerrar / reiniciar / volume / monitor / reciclagem, etc.) |
| `yyzInputHint.exe` | Indicação em tempo real de teclas do teclado / rato (RawInput) |
| `yyzMouseFinder.exe` | Localização rápida do cursor do rato |
| `yyzUpdater.exe` | Atualizador automático incremental (ver `src/updater/`) |
| `yyzFileSearch.exe` | Motor de busca de ficheiros próprio — leitura direta da MFT + índice incremental USN |

O frontend chama os vários Managers do C++ através da ponte `window.Zen`, devolvendo sempre o contrato unificado `{ error, ... }`.

## Transferência e Instalação

- **Instalador** (recomendado): `yyzTools-setup-1.0.6.1650.exe` —— [GitHub Releases](https://github.com/jearry/yyzTools/releases)
- Se o acesso estiver lento a partir da China, utilize um espelho: [ghfast.top](https://ghfast.top/) · [ghproxy.com](https://ghproxy.com/) · [gh-proxy.com](https://gh-proxy.com/) (adicione o prefixo correspondente antes do link de descarga)
- Também disponível na [página de transferências oficial](https://yyztools.com/pt/download.html).

O assistente de instalação suporta 12 idiomas: chinês simplificado e tradicional, inglês, japonês, coreano, francês, alemão, espanhol, russo, árabe, português e italiano.

## Requisitos de Sistema

- Windows 10 / 11 (x64)
- WebView2 Runtime (já incluído nos sistemas recentes; o instalador instala automaticamente se faltar)

## Idioma da Interface

A interface e os textos dos módulos de comandos suportam os 12 idiomas acima; no primeiro arranque o idioma é atribuído automaticamente conforme o sistema e pode ser alterado nas definições.

## Sobre Este Repositório

Este repositório (`jearry/yyzTools`) é o **repositório de publicação e de alojamento do site oficial** do yyzTools:

- [`releases/`](releases/) —— Instaladores versionados e arquivos divididos (`.7z`), além do manifesto de atualização automática `update.json`
- [`docs/`](docs/) —— Site estático de <https://yyztools.com> (alojado no GitHub Pages)
- [`src/updater/`](src/updater/) —— Código-fonte do atualizador automático

Para reportar problemas de funcionalidades, utilize [Issues](https://github.com/jearry/yyzTools/issues).

## Licença e Agradecimentos

O yyzTools é software proprietário, de utilização gratuita. Atualmente apenas o componente de atualização automática é open source, licenciado nos termos da **Licença MIT**. A decisão de abrir o código de outras partes ou da totalidade da aplicação será tomada conforme as circunstâncias.

Este software utiliza várias bibliotecas e ferramentas open source; a lista completa de componentes de terceiros e respetivas licenças está em [LICENSE](LICENSE).

Um sincero obrigado a todos os programadores que contribuíram para estes projetos open source!
