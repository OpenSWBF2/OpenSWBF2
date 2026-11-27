# OpenSWBF2 Link

A Rider plugin that shows the log of a running OpenSWBF2 game and drives it, like UnrealLink's Unreal Editor Log panel.

## Build

Needs Rider 2026.1 (with its bundled C++ support) and a JDK 21 installed. The first build must be online, so Gradle can fetch itself and the build plugins; later builds work with `--offline`.

    tools\rider-link\gradlew.bat -p tools\rider-link buildPlugin

If Rider is not found under `%ProgramFiles%\JetBrains`, add `-PriderHome=<Rider folder>`.

## Install

In Rider: Settings, Plugins, the gear menu, Install Plugin from Disk, then pick the zip under `tools\rider-link\build\distributions`.

## Use

- Run a Debug or Test game with `-nofocuspause`. Without it the game pauses itself whenever Rider has focus.
- Open the BattlefrontII solution. The plugin connects to a running game by itself; the OpenSWBF2 Link tool window appears when it connects and stays until the solution closes. With several games running, pick one in the Pandemic menu on the main toolbar.
- The tool window filters by verbosity and category; its gear menu and Settings, Tools, OpenSWBF2 Link hold the log options.
- Play, Pause, Resume, Step Frame and Stop sit on the main toolbar next to the run configuration. Play starts the mission named under Settings, Tools, OpenSWBF2 Link.
- Lines written while the game crashes are not sent to the panel; the log file has them.
