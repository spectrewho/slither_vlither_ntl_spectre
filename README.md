# Vlither (Spectre Edition)

## A High-Performance Desktop Client for Slither.io

Vlither is a desktop client for [Slither.io](https://slither.io) written in C and powered by Vulkan for rendering. By bypassing the browser, it delivers exceptionally smooth gameplay, higher frame rates, and sharp textures.

This version is based on [protocol version 19](./game1107241958.js) and features custom HUD modifications, NTL Team coordination, and detailed in-game overlays.

---

## Features

- **Vulkan-Accelerated Renderer**: Smooth, low-latency rendering.
- **Symmetric Centered UI**: Completely overhauled title screen and menu.
- **In-Game Chat Overlay**: Seamless chat integration with text wrapping, history, and status indicator.
- **NTL Teammate Syncing**: Synchronizes teammates' locations, scores, and custom teammate name highlighting (vibrant green/cyan with a `★` star).
- **Minimap Teammate Indicators**: Teammates are drawn as custom dots with clean names on the minimap HUD.
- **Viewport Zoom & steering**: Scroll wheel zoom and full mouse control even while typing in chat.
- **Auto-Update Pill**: Top-right version checker pill that appears when a newer version is available.
- **Skin Builder & Custom Skins**: Reverted to original working skin builder, supporting all 66 skins and 32 accessories.

---

## Running the Client

Vlither runs out of the box with zero runtime dependencies. Simply ensure your graphics card supports Vulkan 1.0 or newer.

1. Download the executable bundle for your system from the releases.
2. Run `app.exe` (Windows) or `./app` (Linux).

---

## Build Instructions

### Prerequisites
Make sure you have the following installed on your system:
- **C/C++ Compiler**: GCC (MinGW-w64 on Windows, native GCC on Linux).
- **Vulkan SDK**: Available from [LunarG](https://www.lunarg.com/vulkan-sdk).
- **Python 3**: For shader compile orchestration.
- **Premake 5**: For workspace configuration.
- **Slang**: Compiler for `.slang` shaders (available via `winget install shader-slang.slang` on Windows).

---

### Windows Build

1. **Configure Environment Paths**:
   Add compiler binaries and tools to your system PATH:
   - MinGW: `C:\msys64\mingw64\bin`
   - Premake5: `C:\Users\<User>\AppData\Local\Microsoft\WinGet\Links`
   - Slang: `C:\Users\<User>\AppData\Local\Microsoft\WinGet\Packages\shader-slang.slang_Microsoft.Winget.Source_8wekyb3d8bbwe\bin`

2. **Compile Shaders**:
   ```bash
   python build.py 2
   ```

3. **Configure Project & Makefiles**:
   ```bash
   premake5 --file=project.lua gmake
   ```

4. **Compile Builds**:
   - **Debug Build**:
     ```bash
     mingw32-make -C build/makefiles config=debug_windows -j4
     ```
   - **Release Build**:
     ```bash
     mingw32-make -C build/makefiles config=release_windows -j4
     ```

---

### Linux Build

1. **Install Dependencies (Ubuntu/Debian)**:
   ```bash
   sudo apt update
   sudo apt install build-essential libvulkan-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
   ```

2. **Compile Shaders**:
   ```bash
   python3 build.py 2
   ```

3. **Configure Workspace**:
   ```bash
   premake5 --file=project.lua gmake
   ```

4. **Compile Builds**:
   - **Debug Build**:
     ```bash
     make -C build/makefiles config=debug_linux -j4
     ```
   - **Release Build**:
     ```bash
     make -C build/makefiles config=release_linux -j4
     ```

---

## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](./LICENSE) file for details.

## Disclaimer & Copyright

All game assets, artwork, trademarks, and game content are the property of [Slither.io](https://slither.io). This project is an independent performance-focused client and does not claim ownership over any original Slither.io assets.
