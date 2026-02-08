# Changelog

All notable changes to this project will be documented in this file.

The format is based on Common Changelog,
and this project adheres to Semantic Versioning.

## [0.2.1] - 2026-02-08

- Fixed `clang-tidy` CI failure in the settings dialog by using explicit sizer
  flag casts for mixed wx flag enums.
- Stopped shipping redundant `locale/` folders in release artifacts now that
  catalogs are embedded in the launcher binary.
- Updated `README.md` to remove outdated manual language override guidance and
  clarify developer build vs local runtime test requirements.

## [0.2.0] - 2026-02-08

- Added full launcher localization infrastructure with gettext catalogs,
  install-level language override support, and German translations.
- Added automated i18n quality gates in hooks and CI, including `.po` validation
  and POT/source sync checks.
- Added settings dialog support for selecting and persisting both Gothic version
  and launcher language.
- Applied language changes immediately without restarting the launcher process.
- Embedded locale catalogs into the launcher binary and load them from an
  XDG-compliant user cache, removing runtime dependency on external `locale/`
  deployment.
- Improved release artifact packaging to include locale assets across all
  platform jobs.

## [0.1.1] - 2026-02-08

- Improved startup and launch error messages with clearer, actionable paths.
- Aligned runtime executable detection with documented binary naming
  (`Gothic2Notr` / `Gothic2Notr.exe`).
- Restructured `README.md` to prioritize user setup and runtime usage flow.
- Strengthened CI quality gates with formatter checks, clang-tidy policy, strict
  warnings, and hardened build profiles.
- Improved Windows CI build performance and consistency with Ninja and pinned,
  cached vcpkg dependencies.
- Hardened release automation by deriving release SemVer from tag names and
  enforcing complete draft release assets.

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

[0.2.1]: https://github.com/versable/OpenGothicStarter/releases/tag/v0.2.1
[0.2.0]: https://github.com/versable/OpenGothicStarter/releases/tag/v0.2.0
[0.1.1]: https://github.com/versable/OpenGothicStarter/releases/tag/v0.1.1
[0.1.0]: https://github.com/versable/OpenGothicStarter/releases/tag/v0.1.0
[0.0.1]: https://github.com/versable/OpenGothicStarter/releases/tag/v0.0.1
