# Modified DXVK's D3D9 adding eyecandy of bigger cursor

This repository is a fork of [DXVK](https://github.com/doitsujin/dxvk/) with following modification:

- Adding a new config option: `d3d9.enlargeHardwareCursor = True or False`. When enabled, D3D9 hardware cursor would be enlarged into 2X size. So when running an elder D3D9 game on modern HiDPI display should be easier to see mouse cursor.

- Enable `d3d9.enlargeHardwareCursor = True` and `d3d9.deviceLossOnFocusLoss = True` for WoW.exe WoWFoV.exe and WoW_tweaked.exe. When running Vanilla WoW via [WINE-GE](https://github.com/GloriousEggroll/wine-ge-custom) we could enable [AMD FSR](https://www.amd.com/en/technologies/fidelityfx-super-resolution) just like Retail WoW. However when multiboxing, it seems Vanilla WoW need `d3d9.deviceLossOnFocusLoss` to switch between multiple full-screen windows.

## How to use

For [Turtle WoW](https://turtle-wow.org/):

- Enable Hardware Cursor in game Video Options

- Windows 11: Put d3d9.dll under the same folder with WoW.exe

- Debian Linux and WINE: Put d3d9.dll into WINEPREFIX/drive_c/windows/system32/

- Debian Linux and Proton: Put d3d9.dll into Proton_Directory/files/lib/wine/dxvk/

- WINE may need a DLL override via winecfg to mark d3d9 as Native. Proton don't need this.

For other games you might find more info from upstream DXVK(https://github.com/doitsujin/dxvk/)

## Limitation

- Only works for **SOME** D3D9 games. No D3D11 nor D3D12.
- Only works for hardware cursor, not software cursor.
- I tested it on Debian Linux and Windows 11 using [Turtle WoW](https://turtle-wow.org/)
- According to MS document, if the underlaying platform can not support cursor size bigger than 32x32, API should scale the size back. However current implementation do not have such fallback capability. You may end up with no cursor.
- Vanilla WoW has a hard-coded 32x32 cursor. So current code has a 64x64 buffer for it. If you try using this code for another game, beware that if that game's cursor is already bigger than 32x32, the enlargement code would be bypassed.

## Algorithm

This work employs hq2x-1.2 from https://github.com/grom358/hqx

## DISCLAIMERS

This work follow DXVK's original Zlib license.

Included hqx algorithm code is LGPL. Care, it's LGPL.

It is recommended to build your own .dlls from source code. Downloading arbitrary binary from Internet and apply them onto Azeroth could lead into Burning Legion invasion or lose of your account.


