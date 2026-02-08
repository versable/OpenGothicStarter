# Changelog

All notable changes to this project will be documented in this file.

The format is based on Common Changelog,
and this project adheres to Semantic Versioning.

## [0.1.0] - 2026-02-08

- Adopted a drop-in runtime model rooted at `Gothic/system` with saves in
  `Gothic/Saves` and `Gothic/Saves/<MOD_NAME>`.
- Added automatic Gothic version detection with persisted install selection in
  `system/OpenGothicStarter.ini`.
- Added Windows-only DirectX 12 launcher toggle (`-dx12`).
- Added Unix-like PE icon extraction support for `.exe` mod icons via
  `pe-parse`.
- Improved launch diagnostics and streamlined UI event/launch flow internals.

## [0.0.1] - 2026-02-07

_First release._

[0.1.0]: https://github.com/versable/OpenGothicStarter/releases/tag/v0.1.0
[0.0.1]: https://github.com/versable/OpenGothicStarter/releases/tag/v0.0.1
