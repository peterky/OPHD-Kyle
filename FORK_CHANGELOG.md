# Kyle's Colony — Fork Changelog

Changes in this fork relative to upstream [OutpostHD v0.8.10](https://github.com/OutpostUniverse/OPHD/releases). For the base game's history, see [CHANGELOG.md](CHANGELOG.md).

Version strings appear on the main menu as `v0.8.10-kyle.N`.

## [kyle.16] — 2026-06-24

### Added
- **Remote Construction Links** research (Hot Lab / Physics, cost 40, no prerequisites) — structures can be built within Communications Tower range, not only Command Center range

## [kyle.15] — 2026-06-24

### Fixed
- Removed F10 workforce-report hotkey that blocked Ctrl+Shift+F10 cheat menu

### Changed
- Disabled automatic GitHub Actions builds on push (upstream CI matrix; manual trigger only)

## [kyle.14] — 2026-06-24

### Fixed
- Explorer no longer reveals ore deposits on tiles already occupied by structures or robots
- Dozer bulldozes structures before checking ore on the same tile (fixes tube-over-deposit case)
- Undeveloped ore beacons (dig depth 0) can be cleared with a dozer
- Stuck dozer queue entries that can never succeed no longer spam alerts every turn

## [kyle.13] — 2026-06-24

### Fixed
- Tube and robominer toolbar icons clickable again (zero-height IconGrid hit area)
- Tooltips restored for single-row robot icon grids

### Changed
- Save/load dialog shows last-modified date/time per save file
- Sort saves by name or date (newest first)

## [kyle.12] — 2026-06-24

### Fixed
- Restored robominer toolbar button (separate icon grid below tube)
- Fixed explorer icon in rebuilt `robots.png` sprite sheet

## [kyle.11] — 2026-06-24

### Fixed
- Colony status panel unit icon alignment (unified 20×20 drawing)

## [kyle.10] — 2026-06-24

### Fixed
- Turn-end crash after placing dozers (`onRobotTaskComplete` called `mapCoordinate()` after `detachFromTile()`)

## [kyle.9] — 2026-06-24

### Fixed
- Additional turn-end crash cases with deployed dozers

## [kyle.8] — 2026-06-24

### Changed
- Restored turn-based dozer deployment with terrain-dependent task times

| Terrain | Turns |
| ------- | ----- |
| Clear | 1 |
| Rough | 2 |
| Difficult / Impassable | 3 |

## [kyle.7] — 2026-06-24

### Fixed
- Dozer multi-tile placement queue; build mode stays active while queue pending

## [kyle.6] — 2026-06-24

### Fixed
- Robodozer completion and save-game repair for stuck/incomplete dozer state

## [kyle.4] — 2026-06-24

### Fixed
- Hard cleanup for dozers stuck on map after task completion

## [kyle.3] — 2026-06-24

### Fixed
- Stuck dozers in active saves; map cleanup on load

## Earlier fork work (pre-kyle.3 versioning)

### Added / changed
- Workforce report, maintenance HUD, production history
- Explorer deposit discovery; smelting and ore-haul research tweaks
- Kubuntu-era research modifications restored on `kyle-mods` branch
- Robodozer reliability fixes (dangling research-effects pointer, stuck-on-map cases)