<p align="center">
  <img src="docs/logo.png" width="120" alt="yyzTools">
</p>

<h1 align="center">yyzTools</h1>

<p align="center">
  <strong>Yes Your Zen Tools</strong>
</p>

<p align="center"><a href="README.md">English</a> · <a href="README.zh-CN.md">简体中文</a> · <a href="README.zh-TW.md">繁體中文</a> · <a href="README.ja.md">日本語</a> · <a href="README.ko.md">한국어</a> · <strong>Français</strong> · <a href="README.de.md">Deutsch</a> · <a href="README.es.md">Español</a> · <a href="README.ru.md">Русский</a> · <a href="README.ar.md">العربية</a> · <a href="README.pt.md">Português</a> · <a href="README.it.md">Italiano</a></p>

<p align="center">
  <img alt="version" src="https://img.shields.io/badge/version-1.0.6.1600-blue">
  <img alt="platform" src="https://img.shields.io/badge/platform-Windows%2010%2F11%20x64-lightgrey">
  <img alt="languages" src="https://img.shields.io/badge/UI-12%20languages-green">
  <img alt="license" src="https://img.shields.io/badge/license-gratuit-red">
</p>

---

## Présentation

Rechercher, traduire, reconnaître, prévisualiser, traiter par lots, capturer, décorer — toutes ces petites tâches quotidiennes, sans devoir dénicher, installer et payer un outil différent pour chacune. Installez yyzTools une fois : ce dont vous avez besoin est déjà là.

## Points forts

- **Gratuit à vie** — Toutes les fonctions sont ouvertes : aucun déblocage payant, aucun abonnement, et jamais de version bridée destinée à vous faire payer les fonctions essentielles.
- **Priorité au local** — Presse-papiers, recherche de fichiers, statistiques d’usage, OCR : tout est traité et conservé sur votre propre ordinateur, rien n’est envoyé de lui-même.
- **Une seule installation** — Plus de 40 modules prêts à l’emploi, ce qui vous épargne le cycle habituel : chercher, comparer, télécharger, essayer, désinstaller.
- **379 modules de commandes** — Cinq catégories — IA, applications, jeux, système, sites web — disponibles immédiatement, et un simple fichier JSON suffit pour en ajouter.

## Fonctions

| | |
|---|---|
| **Palette de commandes** | Recherche d’applications · Recherche de fichiers · Gestion des processus · Gestion des fenêtres · Recherche de favoris · Historique du presse-papiers · Ligne de commande · Calcul instantané |
| **Lancement et productivité** | Dock · Historique du presse-papiers · Statistiques d’applications · Capture d’écran · Enregistrement d’écran · Assistant de rappels · Calendrier perpétuel · Indication des touches · Localisation de la souris |
| **Texte et langues** | Traduction avancée · Reconnaissance de texte OCR · Expressions régulières · Encodage et décodage · Calcul de hachage · Chiffrement et déchiffrement · Conversion de données · Génération de données |
| **Fichiers et médias** | Aperçu de fichiers · Renommage par lots · Traitement d’images par lots · Traitement vidéo par lots · Traitement PDF par lots · Téléchargement de fichiers |
| **Calcul et extensions** | Bloc de calcul · QR code · Modules de commandes · Navigateur intégré · Extensions personnalisées |
| **Personnalisation** | Fond d’écran animé · Effets de bureau · Mode jeu · Page de navigation · Défense de la Terre |

## Architecture

Architecture hybride : un **hôte C++ (Win32 + WebView2)** embarquant un **front end (Alpine.js + JS natif + Vite)**, avec les fonctionnalités isolées dans des processus séparés.

| Processus | Rôle |
|-----------|------|
| `yyzTools.exe` | Processus principal — héberge la palette de commandes, la traduction, l'OCR et la plupart des fonctions |
| `yyzWallpaper.exe` | Fond d'écran animé — compose pages web/vidéos sous la couche d'icônes du bureau via l'API Windows Composition |
| `yyzBrowser.exe` | Navigateur intégré sans cadre pour l'ouverture d'URL par les modules de commandes |
| `yyzCmd.exe` | Exécuteur de commandes en Win32 pur (arrêt / redémarrage / volume / écran / corbeille, etc.) |
| `yyzInputHint.exe` | Affichage en temps réel des touches clavier / souris (RawInput) |
| `yyzMouseFinder.exe` | Localisation rapide du curseur de la souris |
| `yyzUpdater.exe` | Metteur à jour automatique incrémental (voir `src/updater/`) |
| `yyzFileSearch.exe` | Moteur de recherche de fichiers interne — lecture directe de la MFT + index incrémental USN |

Le front end appelle les Manager C++ via le pont `window.Zen`, tous répondant selon le contrat `{ error, ... }`.

## Téléchargement

- **Installateur** (recommandé) : `yyzTools-setup-1.0.6.1600.exe` —— [GitHub Releases](https://github.com/jearry/yyzTools/releases)
- Connexion lente depuis la Chine ? Préfixez l'URL de l'installateur par un miroir : [ghfast.top](https://ghfast.top/) · [ghproxy.com](https://ghproxy.com/) · [gh-proxy.com](https://gh-proxy.com/)
- Vous pouvez aussi le récupérer sur la [page de téléchargement officielle](https://yyztools.com/fr/download.html).

L'assistant d'installation prend en charge 12 langues : chinois simplifié et traditionnel, anglais, japonais, coréen, français, allemand, espagnol, russe, arabe, portugais et italien.

## Configuration requise

- Windows 10 / 11 (x64)
- WebView2 Runtime (préinstallé sur les systèmes récents ; l'installateur le récupère s'il manque)

## Langues de l'interface

L'interface comme les textes des modules de commandes prennent en charge les 12 langues ci-dessus. Au premier lancement, la langue du système est détectée automatiquement ; elle se change à tout moment dans les paramètres.

## À propos de ce dépôt

Ce dépôt (`jearry/yyzTools`) est le **dépôt de publication et d'hébergement du site officiel** de yyzTools :

- [`releases/`](releases/) —— installateurs versionnés et archives fractionnées (`.7z`), plus le manifeste de mise à jour `update.json`
- [`docs/`](docs/) —— site statique de <https://yyztools.com> (hébergé via GitHub Pages)
- [`src/updater/`](src/updater/) —— code source du moteur de mise à jour automatique

Pour signaler un problème de fonctionnalité, utilisez [Issues](https://github.com/jearry/yyzTools/issues).

## Licence et remerciements

yyzTools est un logiciel propriétaire, utilisable gratuitement. Seul le module de mise à jour automatique est open source à l'heure actuelle, sous **licence MIT**. L'ouverture du code des autres parties, ou de l'ensemble, sera décidée le moment venu.

Ce logiciel intègre plusieurs bibliothèques et outils open source. Consultez [LICENSE](LICENSE) pour la liste complète des composants tiers et leurs licences respectives.

Nos plus sincères remerciements à tous les développeurs qui contribuent à ces projets open source !
