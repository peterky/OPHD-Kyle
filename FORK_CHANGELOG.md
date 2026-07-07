# Kyle's Colony — Fork Changelog

Changes in this fork relative to upstream [OutpostHD v0.8.10](https://github.com/OutpostUniverse/OPHD/releases). For the base game's history, see [CHANGELOG.md](CHANGELOG.md).

Version strings appear on the main menu as `v0.8.10-kyle.N`.

## [kyle.29] — 2026-06-29

### Fixed
- **Deep Scan satellite crash** — extending map depth no longer reallocates the tile vector and invalidates structure tile references
- **Report tab bar** — removed decorative tab icons that overlapped tab labels; exit tab shows a centered “X”
- **Report panels** — removed header/structure preview icons from Ports, Sats, Workers, Maint., Stores, Factory, and Mines so text starts at the left margin

## [kyle.28] — 2026-06-29

### Fixed
- **Workers report** — unified panel layout coordinates, increased control spacing, Take Me There no longer overlaps the shortage list, selection hint centers in the list area, detail strip pinned to panel bottom
- **Spaceports report** — launch buttons now sit below each satellite description instead of overlapping status/cost text

## [kyle.27] — 2026-06-29

### Fixed
- **Fusion Reactor** and other research-gated structures now unlock correctly when loading a save (completed tech structure unlocks are replayed into the build menu)
- **Biowaste recycling** runs before maintenance each turn; maintenance facilities pull freshly recycled warehouse parts before repairs; overflow residences with the largest backlog are processed first
- Recycling now reports parts stored vs lost when warehouses are full (HUD waste line + turn notification)
- **Report UI overlaps** — Maintenance, Stores, Factory, Mines, Production tooltip, and Research control areas now lay out relative to the report panel instead of full-screen coordinates

## [kyle.26] — 2026-06-29

### Fixed
- Minimap crash when iterating command centers or ore deposits (null/stale pointer guards; skip bad ore-deposit entries instead of aborting the whole overlay)
- Mine report no longer dereferences missing ore-route data
- Report layout pass: Research (progress label vs buttons), Factory (left/right columns), Maintenance, Stores, Production controls, Spaceports launch buttons
- F10/F11/F12 switch to Workers, Sats, and Ports tabs while reports are open (plain F10 only; Ctrl+Shift+F10 still opens the cheat menu on the map)

## [kyle.25] — 2026-06-28

### Fixed
- **Storage tanks** now receive smelted ore — refined metal from smelters fills dedicated tanks first (Command Center is backup only; smelter buffers no longer absorb colony hauls)
- **Police effectiveness** — connected colonies on the same depth are patrolled by operational police stations; protected structures shed crime faster (network patrol and direct coverage)
- **Report UI overlap** — Mines (dynamic ore panel placement), Maintenance, Stores/Warehouse, and Factory detail panes reserve space for headers, lists, and action buttons

## [kyle.24] — 2026-06-28

### Added
- **Food net forecast** in the top bar — food storage shows net production per turn `(+N)` / `(-N)` with detailed prod/consume breakdown in the tooltip
- **Free workers/scientists** shown inline as `W# S#` next to population count (green when available, red when over-committed)
- **Skip-turn food warning** — Alt+Enter dialog warns when skipping turns would risk starvation or when food runs a deficit
- **Map hover tooltips** on structures showing idle/disabled reasons without opening the inspector
- **New-game guidance tips** — rotating colony tips for the first 40 turns when no research is active
- **Mine report haul clarity** — shows smelter destination, raw ore at mine vs raw ore waiting at smelter

### Changed
- **Warehouses** now accept **raw ore buffers** (100 per type): mines may haul overflow ore to warehouses; warehouses auto-feed smelters each turn
- **Recycling facilities** convert processed biowaste into **maintenance parts** (1 part per 20 waste) stored in warehouses
- **Maintenance facilities** pull maintenance parts from warehouses before consuming refined metal for supplies
- Tooltips and starter tips updated for ore routing, biowaste recycling, and maintenance parts flow
- Structure status descriptions unified across inspector, mine report, and map hover

### Fixed
- Linker error for orbital satellite launches (`ColonyResearchEffects` forward declaration)

## [kyle.23] — 2026-06-28

### Added
- **Automation file bridge** — `logs/automation.cmd` commands (`turn`, `cheat`, `research_queue`, `escape`) for scripted play without keyboard/mouse automation

## [kyle.22] — 2026-06-28

### Fixed
- Workers report crash hardening: skip destroyed structures in shortage list, guard detail panel drawing, avoid re-entrant list layout during refresh
- Report panels are now owned per `ReportsState` instance (no shared static panel storage)
- Crash logs now include a Windows stack trace for access violations
- Structure list rows tolerate missing/destroyed structures instead of dereferencing stale pointers

## [kyle.21] — 2026-06-28

### Fixed
- Workers report layout overlap (labels, buttons, and hints now use a single stacked layout)
- Empty shortage list no longer renders on top of the right panel (no more green garbage when fully staffed)
- Workforce conversion buttons no longer crash when population data is not ready
- kyle.21 build now compiles and packages correctly (rectangle `position`/`size` API; per-button handlers for radio-style toggles)

## [kyle.20] — 2026-06-28

### Fixed
- Windows zip packaging again includes all runtime DLLs (SDL2, GLEW, image/mixer/audio deps); use `scripts/package-win64.ps1` when building releases

### Added
- **Research Tech Index** — button on the Research report lists every technology with benefits, prerequisites, and completion status
- Research topic details now show human-readable **benefits**, **unlocks**, and **enables research** chains (not just raw XML tokens)

### Changed
- **Workforce conversion** replaces one-way "re-education": default is **Status quo** (no role changes); optionally convert scientists→workers or workers→scientists at 5/10/20 per turn

## [kyle.19] — 2026-06-28

### Added
- **Orbital satellite program** — complete Orbital Mechanics (6120) to unlock the Orbital Survey Satellite; launch from the Spaceports report to reveal hidden ore deposits colony-wide
- **Geological Penetrator Array** research (6130) — unlocks the Deep Scan satellite, which reveals additional deposits and extends planet digging depth by one level
- **Satellites report** — tracks launched satellites, revealed mine locations, and remaining hidden deposit pool
- Fixed upstream typo: "Oribtal Mechanics" → "Orbital Mechanics"

## [kyle.18] — 2026-06-27

### Fixed
- Autosave and quit crash when deployed dozers were in a detached or stale state (`Robot must be placed`)
- Robodozer `abortTask()` and breakdown handling no longer require a map tile after work completes
- Save serialization reads robot coordinates directly instead of throwing on edge-case placement state

## [kyle.17] — 2026-06-27

### Fixed
- Turn-end and quit crash when canceling robodozer tasks (`abortTask()` called after `detachFromTile()`)
- Session teardown no longer assumes every deployed robot still has a map tile

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