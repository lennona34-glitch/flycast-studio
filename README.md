# Flycast-XR Studio

[![Windows Build](https://github.com/lennona34-glitch/flycast-studio/actions/workflows/windows.yml/badge.svg)](https://github.com/lennona34-glitch/flycast-studio/actions/workflows/windows.yml)
[![Platform](https://img.shields.io/badge/platform-Windows%20x64-blue.svg)](https://github.com/lennona34-glitch/flycast-studio)
[![DirectX 11](https://img.shields.io/badge/Renderer-DirectX%2011%20%7C%20Vulkan-green.svg)](https://github.com/lennona34-glitch/flycast-studio)
[![License: GPL v2](https://img.shields.io/badge/License-GPL%20v2-yellow.svg)](LICENSE)

<p align="center">
  <img src="shell/linux/flycast.png" alt="Flycast Studio Logo" width="160"/>
  <br>
  <strong>Next-Generation Sega Dreamcast, NAOMI, NAOMI 2 & Atomiswave Arcade Emulator</strong>
</p>

---

## 🌟 Overview

**Flycast-XR Studio** is an advanced fork of Flycast designed for modern gaming environments. It pairs bit-accurate Sega Dreamcast, NAOMI, NAOMI 2, and Atomiswave arcade emulation with modern display innovations, wireless VR streaming, live community modding, and authentic CRT simulation.

---

## 🚀 Key Features

### 🥽 Meta Quest VR & Mixed Reality Broadcast Studio
- **Low-Latency Wireless VR Streaming**: Stream any Dreamcast or arcade title directly to your Meta Quest browser at silky-smooth 72 FPS via high-efficiency WebSockets/WebXR.
- **Stereoscopic 3D & Virtual Arcade**: Experience games floating in mixed reality or inside a life-sized virtual arcade cabinet.
- **Split-Screen Isolation**: Render VR stereo pairs cleanly without cluttering desktop controls or UI windows.

### 🍌 GameBanana Dreamcast Modding Hub & Cheats Studio
- **Integrated Mod Browser**: Search, preview, and explore thousands of skins, custom textures, music packs, and total conversions directly inside Flycast.
- **1-Click Download & Disc Patching**: Download mods directly into game directories with zero manual file moving.
- **GameShark / Action Replay Memory Injector**: Live cheat injector with memory search, health/lives locks, and widescreen hack patches.

### 📺 Native Hardware-Accelerated CRT Shader (DirectX 11)
- **Zero UI Blur**: Dedicated dual-pass quad pipeline renders the 3D/2D game viewport with authentic phosphor glow, curvature, and scanlines while keeping all Flycast UI menus, text, and debug bars tack-sharp.
- **4 Built-In Hardware Presets**:
  1. **Arcade CRT (15 kHz)**: Authentic RGB arcade monitor with subtle scanlines and phosphor bloom.
  2. **Dreamcast VGA (480p)**: Razor-sharp progressive scanlines mimicking the official Sega Dreamcast VGA Box on high-grade CRT monitors.
  3. **Curved Tube (Trinitron)**: Barrel curvature, aperture grille phosphors, and subtle glass corner vignetting.
  4. **Clean Crisp Scanlines**: Modern scanline overlay without geometry distortion for pure pixel art.

### 🎨 Integrated ReShade Post-Processing Suite
- **1-Click ReShade Setup**: Built-in downloader installs ReShade and over 70 vintage CRT, NTSC, and retro color-grading shaders into your Flycast folder.
- **In-Game Overlay**: Accessible via the `Home` key or through the in-game `Tab` menu.

---

## 🎮 Controls & Shortcuts

| Action | Shortcut / Input |
|---|---|
| **Toggle True Fullscreen** | `Alt + Enter` |
| **Exit Fullscreen / Quick Menu** | `Esc` |
| **Open In-Game Studio Menu** | `Tab` |
| **Toggle ReShade Overlay** | `Home` |
| **Take Screenshot** | `F12` |
| **Quick Save State** | `F5` |
| **Quick Load State** | `F7` |

---

## 🛠️ Building from Source (Windows x64)

### Prerequisites

Before compiling, make sure you have the following installed on Windows 10/11:

1. **Visual Studio 2022** (Community, Professional, or Enterprise)
   - Ensure the **"Desktop development with C++"** workload is selected in the Visual Studio Installer.
   - Ensure **"C++ CMake tools for Windows"** and **"MSVC v143 - VS 2022 C++ x64/x86 build tools"** are checked.
2. **Git for Windows** (available at [git-scm.com](https://git-scm.com/))
3. **CMake 3.15+** (available at [cmake.org](https://cmake.org/download/) or via Visual Studio)

---

### Option A: 1-Click Automated Build (Recommended)

1. Clone the repository with all submodules:
   ```cmd
   git clone --recursive https://github.com/lennona34-glitch/flycast-studio.git
   cd flycast-studio
   ```

2. Run the automated build script:
   ```cmd
   build_windows.bat
   ```

The script will automatically:
- Verify and update all recursive submodules (`SDL`, `glslang`, `libchdr`, `xxHash`, etc.).
- Generate the Visual Studio 2022 x64 project with modern DirectX 11 and Vulkan enabled.
- Compile in `Release` configuration using all available CPU threads.
- Output the binary to `build\Release\flycast.exe` and copy it to `dist\flycast.exe`.

---

### Option B: Manual Command Line Build

1. **Clone with Submodules**:
   ```cmd
   git clone --recursive https://github.com/lennona34-glitch/flycast-studio.git
   cd flycast-studio
   ```

   *(If already cloned without `--recursive`, run this first:)*
   ```cmd
   git submodule update --init --recursive
   ```

2. **Configure CMake**:
   ```cmd
   cmake -B build -A x64 -DCMAKE_BUILD_TYPE=Release -DUSE_DX9=OFF
   ```
   > **Note**: `-DUSE_DX9=OFF` ensures compatibility with modern Windows 10/11 systems without needing the legacy 2010 DirectX SDK. Flycast uses DirectX 11, Vulkan, and OpenGL for maximum performance and shader support.

3. **Build Release Binary**:
   ```cmd
   cmake --build build --config Release --parallel
   ```

4. **Run Flycast**:
   ```cmd
   build\Release\flycast.exe
   ```

---

## 📦 Continuous Integration & Releases

Automated Windows x64 binaries are built on every push via **GitHub Actions**.
Download the latest pre-compiled artifacts directly from the **[Actions tab](https://github.com/lennona34-glitch/flycast-studio/actions)**.

---

## 🕹️ Supported ROM & Disc Formats

- **Sega Dreamcast**: `.gdi`, `.cdi`, `.chd`, `.cue`, `.iso`
- **Sega NAOMI / NAOMI 2**: `.zip`, `.7z`, `.bin`, `.dat`
- **Sammy Atomiswave**: `.zip`, `.7z`

Place your BIOS and system files in your configured `data` directory (`dc_boot.bin`, `dc_flash.bin`, `naomi.zip`, `awbios.zip`).

---

## 📜 Credits & License

- Based on [Flycast](https://github.com/flyinghead/flycast) by flyinghead and the [reicast](https://github.com/skmp/reicast-emulator) team.
- Licensed under the **GNU General Public License v2** (GPL-2.0). See [LICENSE](LICENSE) for details.
