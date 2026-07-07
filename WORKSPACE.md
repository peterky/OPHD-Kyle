# Project layout

**Canonical home (Synology):** `C:\Users\h4don\SynologyDrive\Projects\OPHD`

This folder is synced to Synology NAS. It holds source code, git history, and Windows release packages.

| Path | Purpose |
| ---- | ------- |
| *(repo root)* | Source code, `OPHD.sln`, docs |
| `release/` | Pre-built Windows x64 zips and extracted folders (`OutpostHD-KylesColony-v0.8.10-kyle.*-win64`) |
| `data/` | Game assets submodule |
| `nas2d-core/` | NAS2D engine submodule |

**Local build workspace (optional):** `C:\Users\h4don\Games\OPHD`

Fast local SSD copy for Visual Studio builds. Sync from Synology before starting work; copy new `release/` builds back to Synology after packaging.

**GitHub:** [github.com/peterky/OPHD-Kyle](https://github.com/peterky/OPHD-Kyle) — branch `kyle-mods` (default `main` tracks the fork).

**Upstream:** [github.com/OutpostUniverse/OPHD](https://github.com/OutpostUniverse/OPHD) — original Outpost Universe project (remote `origin`).

## Sync checklist

After a dev session in `Games\OPHD`:

1. Commit and push from whichever tree has the latest `.git`
2. `robocopy` source (exclude `.build`, `.vs`, `vcpkg_installed`, `dist`) → Synology
3. `robocopy dist` → Synology `release\`
4. Push `kyle-mods` and `main` to GitHub