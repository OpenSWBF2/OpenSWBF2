# OpenSWBF2
Repo where I'm playing around with new features and modernization. Nothing here is release quality.

# Star Wars: Battlefront II (2005) Decompiled

## Prerequisites

- Visual Studio 2022 (17.x) with the "Desktop development with C++" workload and the v143 x86 tools (the project builds as C++17 with `/permissive`)
  https://visualstudio.microsoft.com/vs/
- Windows SDK 10.0.26100.0
- DirectX SDK (June 2010), which sets DXSDK_DIR (only d3dx9 is used from it)
  https://www.microsoft.com/en-us/download/details.aspx?id=6812

## Runtime requirements

- Windows 10 or later (Windows XP and Windows 7 are no longer supported)
- Microsoft Visual C++ 2015-2022 Redistributable (x86) for the x86 (`Win32`) build
  https://aka.ms/vs/17/release/vc_redist.x86.exe
- Microsoft Visual C++ 2015-2022 Redistributable (x64) for the x64 build
- Visual C++ Redistributable for Visual Studio 2013 (x86), which provides `msvcr120.dll` and `msvcp120.dll` for the x86 Galaxy DLLs; only x86 Galaxy online play needs it, and the x64 build does not use it
  https://support.microsoft.com/help/3179560

`openswbf2-ffmpeg.dll`, which decodes the movies, is built from `src/FFmpegFL` under the LGPL v2.1 or later and ships beside the exe with `NOTICE.txt` and `COPYING.LGPLv2.1`. The CRT is not shipped beside the exe; install the official redistributables. The DirectX End-User Runtime is not needed: `d3dx9_43.dll` ships beside the exe. The build downloads `Microsoft.DXSDK.D3DX` 9.29.952.8 from nuget.org once into `%LOCALAPPDATA%\OpenSWBF2\vendor`, checks it against the hashes in `lib\vendor-pins.props`, and copies its `D3DX9_43.dll` for the build's platform. Later builds use the cached copy and work offline.

## Build

```
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" BattlefrontII.sln /p:Configuration={Config} /p:Platform={Platform}
```

`{Platform}` is `Win32` or `x64`. Use the MSBuild of whichever VS2022 edition is installed; `vswhere` finds it.

The exe is written to `bin\{Platform}\{Config}\BattlefrontII.exe`.

## Run

The build automatically finds the Steam install's GameData through Steam's registry entry, or set `SWBF2_GAMEDATA` to the desired GameData directory to use another copy.

## x64 vendor DLLs

The x64 build takes its vendor DLLs from a folder outside the repo, named by the `SWBF2_VENDOR_X64` environment variable or `/p:SWBF2_VENDOR_X64=<folder>`. Put `steam_api64.dll` in it, taken from `redistributable_bin\win64\` of the Steamworks SDK release whose `redistributable_bin\steam_api.dll` is byte-identical to the retail `steam_api.dll` (file version 03.75.32.07). The build copies it only when its SHA-256 is the `SteamApi64Sha256` pin in `lib\vendor-pins.props` (`38f6a1dd1e738e142160322638dae7b88dabcc30eea3f15c58eb08c5341f9b29`) and fails otherwise. Without the folder the x64 build still works, with Steam off.

The x64 build runs without GOG Galaxy, so it has no online lobbies; LAN and direct IP games work.

## Tests

The headless test harness, its gate and the gate tiers are described in `tests\README.md`.
