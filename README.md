# Star Wars: Battlefront II (2005) Decompiled

## Prerequisites

- Visual Studio 2022 (17.x) with the "Desktop development with C++" workload and the v143 x86 tools (the project builds as C++17 with `/permissive`)
  https://visualstudio.microsoft.com/vs/
- Windows SDK 10.0.26100.0
- DirectX SDK (June 2010), which sets DXSDK_DIR (only d3dx9 is used from it)
  https://www.microsoft.com/en-us/download/details.aspx?id=6812

## Runtime requirements

- Windows 10 or later (Windows XP and Windows 7 are no longer supported)
- Microsoft Visual C++ 2015-2022 Redistributable (x86)
  https://aka.ms/vs/17/release/vc_redist.x86.exe
- Visual C++ Redistributable for Visual Studio 2013 (x86), which provides `msvcr120.dll` and `msvcp120.dll` for the Galaxy DLLs
  https://support.microsoft.com/help/3179560
- DirectX End-User Runtime (June 2010), for `d3dx9_43.dll`

The CRT is not shipped beside the exe; install the official redistributables.

## Build

```
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" BattlefrontII.sln /p:Configuration={Config} /p:Platform={Platform}
```

Use the MSBuild of whichever VS2022 edition is installed; `tests\vstools.ps1` finds it through `vswhere`.

The exe is written to `bin\{Platform}\{Config}\BattlefrontII.exe`.

## Run

The build automatically finds the Steam install's GameData through Steam's registry entry, or set `SWBF2_GAMEDATA` to the desired GameData directory to use another copy.
