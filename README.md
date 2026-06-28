# Outpost HD: Kyle's Colony

An unofficial personal fork of [OutpostHD](https://github.com/OutpostUniverse/OPHD), based on upstream **v0.8.10**. This repo contains Kyle's gameplay tweaks, UI polish, robodozer fixes, and colony-management additions.

**This is not an official [Outpost Universe](http://forum.outpost2.net) release.** For the original project, its maintainers, and official builds, see [OutpostUniverse/OPHD](https://github.com/OutpostUniverse/OPHD).

Current fork version: **v0.8.10-kyle.17** (shown on the main menu in-game).

## Download & play

Pre-built **Windows x64** builds are on the **[Releases](https://github.com/peterky/OPHD-Kyle/releases)** page for this repository.

1. Download the latest `OutpostHD-KylesColony-v0.8.10-kyle.*-win64.zip`
2. Extract anywhere
3. Run `appOPHD.exe` (keep the `data/` folder next to the exe)
4. Confirm the main menu shows a `kyle.*` version string

**Requirements:** Windows 10/11 x64, and the [Microsoft VC++ Redistributable (x64)](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist) if the game fails to start.

Save games: `%AppData%\LairWorks\OutpostHD\savegames\`

## What's different in this fork

See **[FORK_CHANGELOG.md](FORK_CHANGELOG.md)** for a version-by-version list. Highlights:

- Workforce, maintenance, and production reporting (F10 reports)
- Explorer robot deposit discovery and related research tweaks
- Robodozer overhaul: multi-turn terrain clearing, placement queue, save repair
- UI fixes: status panel icons, robot toolbar layout, save/load timestamps and sorting
- Bug fixes for turn-end crashes, mine-under-tube discovery, and dozer queue spam

Discussion for this fork: [Outpost2 forum thread (topic 6455)](https://forum.outpost2.net/index.php/topic,6455.0.html)

## Building from source

Clone **this** repository (not upstream), with submodules:

```sh
git clone --recursive https://github.com/peterky/OPHD-Kyle.git
cd OPHD-Kyle
git checkout kyle-mods
git submodule update --init
```

### Windows

Open `OPHD.sln` in Visual Studio 2022 and build **Release | x64**. The executable is written to `.build/Release_x64_appOPHD/appOPHD.exe`.

### Linux / macOS

Upstream build instructions still apply. Install dependencies (`make install-dependencies`), then `make` and `make run`. Non-Windows packages are not published for this fork.

## Configuration

Same as upstream: `config.xml` in the LairWorks user folder (on Windows: `%AppData%\Roaming\LairWorks\OutpostHD\`). The file is created on first run.

## Player guide

[PlayerGuide.md](PlayerGuide.md) covers base OutpostHD gameplay. Workforce and other fork reports are available from the reports panel (F1); Ctrl+Shift+F10 still opens the cheat menu as in upstream.

## Upstream project (original team's work)

OutpostHD is a reimplementation of Sierra's *Outpost* (1994) — a ground-up redesign, not a direct clone.

| Resource | Link |
| -------- | ---- |
| Source code | [github.com/OutpostUniverse/OPHD](https://github.com/OutpostUniverse/OPHD) |
| Official releases | [OutpostUniverse/OPHD Releases](https://github.com/OutpostUniverse/OPHD/releases) |
| Forum | [forum.outpost2.net](http://forum.outpost2.net) |
| OutpostHD discussion | [Forum topic 5718](http://forum.outpost2.net/index.php/topic,5718.0.html) |
| Discord | [Outpost Universe Discord](https://discord.gg/kDz5Q3t) |
| Upstream changelog | [CHANGELOG.md](CHANGELOG.md) |

### Upstream maintainers

OutpostHD is developed and maintained by the Outpost Universe team, including:

- **Leeor Dicker** (leeor_net) — design & programming
- **Hooman** — programming & system engineering
- **White Claw** — graphics

This fork builds on their work and the broader community contributions documented in the upstream project.

## License

Same license as upstream OutpostHD (BSD 3-clause). See [LICENSE.md](LICENSE.md).

Fork-specific changes in this repository are offered under the same terms. OutpostHD itself remains the work of the Outpost Universe project and its contributors.