# Modified DXVK's D3D9 adding eyecandy of bigger cursor

This repository is a fork of [DXVK](https://github.com/doitsujin/dxvk/) with following modification:
- Adding a new config option: `d3d9.enlargeHardwareCursor = True or False`. When enabled, D3D9 hardware cursor would be enlarged into 2X size. So when running an elder D3D9 game on modern HiDPI display should be easier to see mouse cursor.
- Enable `d3d9.enlargeHardwareCursor = True` and `d3d9.deviceLossOnFocusLoss = True` for WoW.exe WoWFoV.exe and WoW_tweaked.exe. When running Vanilla WoW via [WINE-GE](https://github.com/GloriousEggroll/wine-ge-custom) we could enable [AMD FSR](https://www.amd.com/en/technologies/fidelityfx-super-resolution) just like Retail WoW. However when multiboxing, it seems Vanilla WoW need `d3d9.deviceLossOnFocusLoss` to switch between multiple full-screen windows.

## How to use

Build or download the modified `d3d9.dll` and put it into game's executable folder. (The folder holding `WoW.exe` etc.)

## DISCLAIMERS

This work follow DXVK's original Zlib license.

It is recommended to build your own .dlls from source code. Downloading arbitrary binary from Internet and apply them onto Azeroth could lead into Burning Legion invasion or lose of your account.

## Limitation

- Only works for **SOME** D3D9 games. No D3D11 nor D3D12.
- I tested it on Debian Linux and Windows 11 using [Turtle WoW](https://turtle-wow.org/)
- According to MS document, if the underlaying platform can not support cursor size bigger than 32x32, API should scale the size back. However current implementation do not have such fallback capability. You may end up with no cursor.
- Vanilla WoW has a hard-coded 32x32 cursor. So current code has a 64x64 buffer for it. If you try using this code for another game, beware that if that game's cursor is already bigger than 32x32, the enlargement code would be bypassed.

## Algorithm

To enlarge cursor into 2X size, this code employs a modified [Eagle](https://en.wikipedia.org/wiki/Pixel-art_scaling_algorithms#Eagle) algorithm.

Say, we have original cursor pixel P and its surrounding pixels A,B,C,D,E,F,G,H as:
A B C
D P E
F G H

we would try expanding P into 4 new pixels 1,2,3,4 as:
1 2
3 4

Pixel 1 would be a blend of Pixel A,B,D,P by averaging their ARGB color channels. However as Pixel P is the actual source pixel, it would hold a weight of 3 during averaging, while Pixel A,B,D only weighted 1 each.
Pixel 2 would be a bleand of Pixel B,C,P,E with the same averaging process.
Pixel 3 would be a bleand of Pixel D,P,F,G with the same averaging process.
Pixel 4 would be a bleand of Pixel P,E,G,H with the same averaging process.


# Following below is copy from original DXVK repository

A Vulkan-based translation layer for Direct3D 9/10/11 which allows running 3D applications on Linux using Wine.

For the current status of the project, please refer to the [project wiki](https://github.com/doitsujin/dxvk/wiki).

The most recent development builds can be found [here](https://github.com/doitsujin/dxvk/actions/workflows/artifacts.yml?query=branch%3Amaster).

Release builds can be found [here](https://github.com/doitsujin/dxvk/releases).

## How to use
In order to install a DXVK package obtained from the [release](https://github.com/doitsujin/dxvk/releases) page into a given wine prefix, copy or symlink the DLLs into the following directories as follows, then open `winecfg` and manually add DLL overrides for `d3d11`, `d3d10core`, `dxgi`, and `d3d9`.

In a default Wine prefix that would be as follows:
```
export WINEPREFIX=/path/to/wineprefix
cp x64/*.dll $WINEPREFIX/drive_c/windows/system32
cp x32/*.dll $WINEPREFIX/drive_c/windows/syswow64
winecfg
```

For a pure 32-bit Wine prefix (non default) the 32-bit DLLs instead go to the `system32` directory:
```
export WINEPREFIX=/path/to/wineprefix
cp x32/*.dll $WINEPREFIX/drive_c/windows/system32
winecfg
```

Verify that your application uses DXVK instead of wined3d by enabling the HUD (see notes below).

In order to remove DXVK from a prefix, remove the DLLs and DLL overrides, and run `wineboot -u` to restore the original DLL files.

Tools such as Steam Play, Lutris, Bottles, Heroic Launcher, etc will automatically handle setup of dxvk on their own when enabled.

### Notes on Vulkan drivers
Before reporting an issue, please check the [Wiki](https://github.com/doitsujin/dxvk/wiki/Driver-support) page on the current driver status and make sure you run a recent enough driver version for your hardware.

## Build instructions

In order to pull in all submodules that are needed for building, clone the repository using the following command:
```
git clone --recursive https://github.com/doitsujin/dxvk.git
```



### Requirements:
- [wine 7.1](https://www.winehq.org/) or newer
- [Meson](https://mesonbuild.com/) build system (at least version 0.49)
- [Mingw-w64](https://www.mingw-w64.org) compiler and headers (at least version 10.0)
- [glslang](https://github.com/KhronosGroup/glslang) compiler

### Building DLLs

#### The simple way
Inside the DXVK directory, run:
```
./package-release.sh master /your/target/directory --no-package
```

This will create a folder `dxvk-master` in `/your/target/directory`, which contains both 32-bit and 64-bit versions of DXVK, which can be set up in the same way as the release versions as noted above.

In order to preserve the build directories for development, pass `--dev-build` to the script. This option implies `--no-package`. After making changes to the source code, you can then do the following to rebuild DXVK:
```
# change to build.32 for 32-bit
cd /your/target/directory/build.64
ninja install
```

#### Compiling manually
```
# 64-bit build. For 32-bit builds, replace
# build-win64.txt with build-win32.txt
meson setup --cross-file build-win64.txt --buildtype release --prefix /your/dxvk/directory build.w64
cd build.w64
ninja install
```

The D3D9, D3D10, D3D11 and DXGI DLLs will be located in `/your/dxvk/directory/bin`. Setup has to be done manually in this case.

### Online multi-player games
Manipulation of Direct3D libraries in multi-player games may be considered cheating and can get your account **banned**. This may also apply to single-player games with an embedded or dedicated multiplayer portion. **Use at your own risk.**

### Logs
When used with Wine, DXVK will print log messages to `stderr`. Additionally, standalone log files can optionally be generated by setting the `DXVK_LOG_PATH` variable, where log files in the given directory will be called `app_d3d11.log`, `app_dxgi.log` etc., where `app` is the name of the game executable.

On Windows, log files will be created in the game's working directory by default, which is usually next to the game executable.

### HUD
The `DXVK_HUD` environment variable controls a HUD which can display the framerate and some stat counters. It accepts a comma-separated list of the following options:
- `devinfo`: Displays the name of the GPU and the driver version.
- `fps`: Shows the current frame rate.
- `frametimes`: Shows a frame time graph.
- `submissions`: Shows the number of command buffers submitted per frame.
- `drawcalls`: Shows the number of draw calls and render passes per frame.
- `pipelines`: Shows the total number of graphics and compute pipelines.
- `descriptors`: Shows the number of descriptor pools and descriptor sets.
- `memory`: Shows the amount of device memory allocated and used.
- `gpuload`: Shows estimated GPU load. May be inaccurate.
- `version`: Shows DXVK version.
- `api`: Shows the D3D feature level used by the application.
- `cs`: Shows worker thread statistics.
- `compiler`: Shows shader compiler activity
- `samplers`: Shows the current number of sampler pairs used *[D3D9 Only]*
- `scale=x`: Scales the HUD by a factor of `x` (e.g. `1.5`)
- `opacity=y`: Adjusts the HUD opacity by a factor of `y` (e.g. `0.5`, `1.0` being fully opaque).

Additionally, `DXVK_HUD=1` has the same effect as `DXVK_HUD=devinfo,fps`, and `DXVK_HUD=full` enables all available HUD elements.

### Frame rate limit
The `DXVK_FRAME_RATE` environment variable can be used to limit the frame rate. A value of `0` uncaps the frame rate, while any positive value will limit rendering to the given number of frames per second. Alternatively, the configuration file can be used.

### Device filter
Some applications do not provide a method to select a different GPU. In that case, DXVK can be forced to use a given device:
- `DXVK_FILTER_DEVICE_NAME="Device Name"` Selects devices with a matching Vulkan device name, which can be retrieved with tools such as `vulkaninfo`. Matches on substrings, so "VEGA" or "AMD RADV VEGA10" is supported if the full device name is "AMD RADV VEGA10 (LLVM 9.0.0)", for example. If the substring matches more than one device, the first device matched will be used.

**Note:** If the device filter is configured incorrectly, it may filter out all devices and applications will be unable to create a D3D device.

### Graphics Pipeline Library
On drivers which support `VK_EXT_graphics_pipeline_library` Vulkan shaders will be compiled at the time the game loads its D3D shaders, rather than at draw time. This reduces or eliminates shader compile stutter in many games when compared to the previous system.

In games that load their shaders during loading screens or in the menu, this can lead to prolonged periods of very high CPU utilization, especially on weaker CPUs. For affected games it is recommended to wait for shader compilation to finish before starting the game to avoid stutter and low performance. Shader compiler activity can be monitored with `DXVK_HUD=compiler`.

This feature largely replaces the state cache.

**Note:** Games which only load their D3D shaders at draw time (e.g. most Unreal Engine games) will still exhibit some stutter, although it should still be less severe than without this feature.

### State cache
DXVK caches pipeline state by default, so that shaders can be recompiled ahead of time on subsequent runs of an application, even if the driver's own shader cache got invalidated in the meantime. This cache is enabled by default, and generally reduces stuttering.

The following environment variables can be used to control the cache:
- `DXVK_STATE_CACHE`: Controls the state cache. The following values are supported:
  - `disable`: Disables the cache entirely.
  - `reset`: Clears the cache file.
- `DXVK_STATE_CACHE_PATH=/some/directory` Specifies a directory where to put the cache files. Defaults to the current working directory of the application.

This feature is mostly only relevant on systems without support for `VK_EXT_graphics_pipeline_library`

### Debugging
The following environment variables can be used for **debugging** purposes.
- `VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation` Enables Vulkan debug layers. Highly recommended for troubleshooting rendering issues and driver crashes. Requires the Vulkan SDK to be installed on the host system.
- `DXVK_LOG_LEVEL=none|error|warn|info|debug` Controls message logging.
- `DXVK_LOG_PATH=/some/directory` Changes path where log files are stored. Set to `none` to disable log file creation entirely, without disabling logging.
- `DXVK_DEBUG=markers|validation` Enables use of the `VK_EXT_debug_utils` extension for translating performance event markers, or to enable Vulkan validation, respecticely.
- `DXVK_CONFIG_FILE=/xxx/dxvk.conf` Sets path to the configuration file.
- `DXVK_CONFIG="dxgi.hideAmdGpu = True; dxgi.syncInterval = 0"` Can be used to set config variables through the environment instead of a configuration file using the same syntax. `;` is used as a seperator.

## Troubleshooting
DXVK requires threading support from your mingw-w64 build environment. If you
are missing this, you may see "error: ‘std::cv_status’ has not been declared"
or similar threading related errors.

On Debian and Ubuntu, this can be resolved by using the posix alternate, which
supports threading. For example, choose the posix alternate from these
commands:
```
update-alternatives --config x86_64-w64-mingw32-gcc
update-alternatives --config x86_64-w64-mingw32-g++
update-alternatives --config i686-w64-mingw32-gcc
update-alternatives --config i686-w64-mingw32-g++
```
For non debian based distros, make sure that your mingw-w64-gcc cross compiler 
does have `--enable-threads=posix` enabled during configure. If your distro does
ship its mingw-w64-gcc binary with `--enable-threads=win32` you might have to
recompile locally or open a bug at your distro's bugtracker to ask for it. 
