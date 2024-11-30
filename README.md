# Chocolate Engine

## Required Software
- CMake
- Python 3.10
- py7zr Python Module
- git
- git-lfs (used in KTX)
- Vulkan SDK

### Windows Software
- Visual Studio 2022

### Linux Software
- Install these packages: `build-essential cmake libsdl2-dev libglm-dev libvorbis-dev libogg-dev`

## Build Instructions
- Make a folder to place this repo in
- Clone the "chocolate_output" repo first in that folder: https://github.com/ChocolateEngine/chocolate_output
- Clone this repo with `--recursive`
- Enter `chocolate/thirdparty` and run the `thirdparty.py` script to download and compile thirdparty stuff
- Compile shaders with `chocolate/src/render/shaders_glsl/_compile.bat` (or _compile.sh on linux)
- OPTIONAL - If you want to run render3, compile render3 shaders here as well: `chocolate/src/render3/shaders_glsl/_compile.bat`
- Make a folder called `build` in the `chocolate` folder (`chocolate/build`)
- Enter that folder and in cmd/terminal, run `cmake ..` on Windows, or `cmake -DCMAKE_BUILD_TYPE=Debug ..` on Linux (also Release is an option)
- Finish with standard cmake build stuff (VS2022 on Windows, `make -j 8` on linux)
- Run `output/sidurylauncher.exe` to run the engine


Current CMake Options:

| Option      | Default Value | Description |
| ----------- | ------------- | ------------- |
| RENDER3     | ON  | Build Render3 |
| RENDER2     | ON  | Build Render2 |
| STEAM       | OFF | Steam Game Abstraction, Requires the Steamworks SDK placed in the `apps/steam` folder |
| GAME        | ON | Build Sidury |
| TOOLKIT     | ON | Build the toolkit |
| RENDER_TEST | ON | Build Render 3 Test App |

