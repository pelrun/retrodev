
# Retrodev

<div align="center"><img src="doc/img/retrodev-logo.png" alt="Retrodev" width="400" /></div>

> **A development environment for retro 8-bit and 16-bit home computer software.**

<div align="center"><a href="https://github.com/tlotb/retrodev/releases/latest"><img src="https://img.shields.io/badge/⬇_Download_Latest_Release-2ea44f?style=for-the-badge" alt="Download Latest Release" /></a></div>

---

## Table of Contents

1. [What is Retrodev?](#1-what-is-retrodev)
2. [Features at a Glance](#2-features-at-a-glance)
3. [Installing and Getting Started](#3-installing-and-getting-started)
4. [Project Layout](#4-project-layout)
5. [Project Items](#5-project-items)
6. [Files Panel](#6-files-panel)
7. [Building a Project](#7-building-a-project)
8. [Emulators and Debugging](#8-emulators-and-debugging)
9. [Contributing and Technical Information](#9-contributing-and-technical-information)
10. [Thanks and Greetings](#10-thanks-and-greetings)
11. [Open Source Libraries and Licenses](#11-open-source-libraries-and-licenses)
12. [License and Copyright](#12-license-and-copyright)

> **Quick links to feature documentation:** [Bitmaps](doc/usage/bitmaps.md) · [Tiles](doc/usage/tiles.md) · [Sprites](doc/usage/sprites.md) · [Maps](doc/usage/maps.md) · [Raster effects](doc/usage/raster.md) · [Build](doc/usage/build.md) · [Palette](doc/usage/palette.md) · [Editor](doc/usage/editor.md) · [Export scripts](doc/usage/export-scripts.md)

---

## 1. What is Retrodev?

Developing software for retro home computers has always meant assembling a fragile toolchain from scattered pieces: a batch file here, a graphics converter there, a separate palette tool, an assembler invoked from the command line, and a hex editor for sanity-checking the output. Every machine has its own quirks, its own file formats, its own assembler dialect. Keeping all those moving parts in sync is tedious and error-prone.

**Retrodev** replaces that mess with a single, self-contained application that acts as the glue between all your existing tools. It does not replace your image editor or your version control system — it sits alongside them and brings together everything you need for the build and conversion side of the workflow in one place:

- **Graphics conversion** — import any modern image and convert it to the native pixel and palette format of the target machine, with live preview.
- **Tile and sprite extraction** — slice a converted image into tiles or sprites ready to be referenced by your code.
- **Tile map editor** — paint multi-layer scrolling maps using extracted tiles, with parallax preview and aspect-ratio-correct display per system and mode.
- **Palette solver** — describe which graphics appear on screen at the same time and let Retrodev find a palette assignment that satisfies all constraints simultaneously.
- **Raster effects generator** — define CRTC and Gate Array register changes at specific scanlines, validate timing in real time against hardware constraints, and export the NOP-accurate Z80 interrupt handler code with a single click. Currently targets the Amstrad CPC; generators for other systems will be added as support is extended.
- **Integrated code editor** — write Z80 assembly and AngelScript export scripts without leaving the application. The editor provides syntax highlighting, RASM directive and label auto-complete, code-lens timing annotations for Z80, and full Retrodev API auto-complete and hover documentation for AngelScript.
- **Scripted export** — write AngelScript export scripts for bitmaps, tile sets, sprites and maps that produce exactly the binary format your program expects, with full control over layout, headers and compression.
- **Project-based workflow** — everything lives in a `.retrodev` project file. Open it, build it, run it. No batch files.

<div align="center"><img src="doc/img/retrodev-overview.png" alt="Retrodev main window" width="800" /></div>

### Supported target systems

| System | Status |
|---|---|
| Amstrad CPC / CPC+ | ✅ Full support |
| ZX Spectrum | 🚧 Planned |
| Commodore 64 | 🚧 Planned |
| MSX | 🚧 Planned |

---

## 2. Features at a Glance

### Graphics conversion and extraction

Retrodev converts modern RGBA images into native hardware formats. The conversion pipeline supports resize, colour correction, quantization and dithering — all applied in the order that matches how real hardware renders pixels.

For the complete documentation on graphics conversion, click [here](doc/usage/bitmaps.md).

**Dithering methods**

**Tile extraction**: chop a converted image into a grid of tiles, deduplicate them, and produce a tile sheet the map editor and tile export scripts can reference.

For the complete documentation on tile extraction, click [here](doc/usage/tiles.md).

**Sprite extraction**: extract individual sprite frames from a sheet, with configurable frame size and ordering.

For the complete documentation on sprite extraction, click [here](doc/usage/sprites.md).

### Palette solver

The palette solver takes a list of graphics participants — bitmaps, tile sets and sprite sheets — and finds the smallest set of hardware colours that satisfies all of them simultaneously, respecting screen zones (horizontal scanline bands that share a palette) and participant roles (always-on, level-specific, raster-zone).

For the complete documentation on the palette solver, click [here](doc/usage/palette.md).

### Tile map editor

Paint multi-layer tile maps with configurable parallax scroll speeds, tile group stamps, viewable-area overlay and aspect-ratio-correct canvas preview per target system and screen mode.

For the complete documentation on the tile map editor, click [here](doc/usage/maps.md).

### Raster effects generator

Define a declarative sequence of CRTC and Gate Array register changes — one Frame command describing the full hardware timing, plus Effect commands targeting specific scanlines — then click **Export ASM** to produce the complete NOP-accurate Z80 interrupt handler code. A built-in CRTC simulator validates every write against hardware timing constraints in real time and reports violations before you export. A pixel-accurate monitor view shows the resulting frame layout as the hardware would produce it, with colour-coded interrupt slot boundaries.

Currently supports the **Amstrad CPC**. Raster generators for other systems will be added as system support is extended.

For the complete documentation on the raster effects editor, click [here](doc/usage/raster.md).

### Integrated code editor

A full-featured text editor with Z80 assembler and AngelScript support: syntax highlighting, RASM directive completion, label and symbol auto-complete, code-lens timing annotations per instruction, and a directive reference built in.

The same editor is used for AngelScript export scripts, where it exposes the full Retrodev export API — `IBitmapContext`, `ITilesetContext`, `ISpriteContext`, `IMapContext`, `IPaletteContext` and all their methods — as built-in auto-complete symbols and hover documentation, so you write export scripts with the same comfort as assembly code.

For the complete documentation on the integrated code editor, click [here](doc/usage/editor.md).

### Scripted export

Bitmap conversions, tile extractions, sprite extractions and maps each have an associated AngelScript export script that receives the fully converted data and writes it in any format you need. Scripts declare their parameters with `// @param` annotations and Retrodev builds the UI for them automatically.

For the complete documentation on scripted export, click [here](doc/usage/export-scripts.md).

---

## 3. Installing and Getting Started

### Download

The latest release is available on the [GitHub Releases page](https://github.com/tlotb/retrodev/releases/latest).

Download the `.zip` archive for your platform and follow the installation instructions below.

### Installation

Retrodev requires no installer. Unzip the release archive to any folder and run `retrodev.exe`. All dependencies are bundled.

```
retrodev-1.0.x/
├── retrodev.exe
├── sdk/              ← reusable macros and library code for your projects
└── examples/         ← ready-to-build example projects
```

### Creating a new project

1. Launch Retrodev.
2. Choose **File → New Project**.
3. Pick a folder. Retrodev creates a single `.retrodev` project file there and opens it — nothing else is added. Your folder and its files are yours to organise however you like.

From here you can add files to the **Files** panel, create project items in the **Project** panel, and start building.

<div align="center"><img src="doc/img/new-project-dialog.png" alt="New Project dialog" width="800" /></div>

### Opening an existing project

Choose **File → Open Project** and navigate to a `.retrodev` project file, or double-click a `.retrodev` file in Explorer. Recent projects appear in the **File → Recent** submenu.

---

## 4. Project Layout

When you open a project, Retrodev shows three sections in the **Project** panel. These are not filesystem folders — they are three distinct views into different parts of your workflow.

<div align="center"><img src="doc/img/project.-view.png" alt="Project panel" width="800" /></div>

### SDK

The **SDK** section shows the contents of the `sdk/` folder that ships alongside `retrodev.exe`. This is a read-only file view of the shared library: Z80 macros, reusable routines and AngelScript helpers that any project can draw from.

To use something from the SDK in your project, include it explicitly in your build configuration — the files are listed in the panel so it is just point-and-click. The paths are handled automatically; you just pick what you need.

The SDK folder is shared across all your projects. It never lives inside your project directory.

### Project

The **Project** section is where all your build configuration lives. This is entirely virtual — nothing here corresponds directly to files on disk. It is the list of things Retrodev should *do* with your files:

- Bitmap conversions
- Tile extractions
- Sprite extractions
- Maps
- Build pipelines
- Palette definitions

Each item stores its parameters in the `.retrodev` project file. You add, configure, reorder and remove them here. The Project section is the backbone of the workflow.

### Files

The **Files** section is a file view rooted at the folder where your `.retrodev` project file lives, showing that folder and all its subfolders. These are your actual input files — bitmaps, assembly sources, AngelScript export scripts, data files — and they can all be opened and edited directly from here:

| File type | Opens in |
|---|---|
| `.png`, `.bmp`, `.jpg`, `.tga` | Built-in image editor / viewer |
| `.asm`, `.z80` | Z80 assembly editor |
| `.as` | AngelScript editor |
| Anything else | Hex / data viewer |

Files in this section are the raw ingredients. The typical way to put them to work is to right-click a file, choose the conversion type from the context menu, and Retrodev creates the corresponding project item with that file already set as the source.

---

## 5. Project Items

Project items appear in the **Project** panel. You can organise them into virtual folders by drag-and-drop — for example, grouping all Amstrad CPC items under a CPC folder and all Spectrum items under a Spectrum folder. This organisation is cosmetic only; execution order is controlled by the dependency list inside each Build project item.

### Bitmap conversion

Converts an image file to the native bitmap format of the target system. Parameters include target mode, resolution, colour correction, quantization, dithering and palette constraints.

For the complete documentation on bitmap conversion, click [here](doc/usage/bitmaps.md).

<div align="center"><img src="doc/img/bitmap-build-item.png" alt="Bitmap conversion editor" width="800" /></div>

### Tiles

Extracts a grid of tiles from a converted image. Configure tile width, height and whether to deduplicate identical tiles. The resulting tile sheet is available to the map editor and to the tile export script.

For the complete documentation on tile extraction, click [here](doc/usage/tiles.md).

<div align="center"><img src="doc/img/tiles-build-item.png" alt="Tile extraction editor" width="800" /></div>

### Sprites

Extracts individual sprite frames from a converted image. Configure frame size, frame count and reading order (left-to-right, top-to-bottom).

For the complete documentation on sprite extraction, click [here](doc/usage/sprites.md).

<div align="center"><img src="doc/img/sprites-editor.png" alt="Sprite extraction editor" width="800" /></div>

### Maps

A tile map stores

Key concepts:

- **Layers** — painted independently; rendered bottom-to-top. Each layer has a **map speed** (parallax factor relative to the camera). A speed of `1.0` scrolls one tile per camera step; `0.5` moves at half the camera rate (background parallax); values ≥ the viewport width create a fixed/room layer.
- **Tileset slots** — a slot is a logical tileset position. Multiple tileset variants can share a slot; the active variant is selected in the editor and passed through to the map export script.
- **Groups** — multi-tile stamps captured by dragging a selection on the canvas and saved as a named group for quick reuse.
- **Viewport** — defines the visible tile area (width × height in tiles). Used for the viewable-area overlay and for scroll range calculations.
- **Aspect-ratio preview** — the canvas can display the map in the correct pixel aspect ratio for the selected target system and mode, so what you see matches what the hardware produces.

For the complete documentation on maps, click [here](doc/usage/maps.md).

<div align="center"><img src="doc/img/map-editor.png" alt="Map editor" width="800" /></div>

### Raster effects

A Raster project item defines a hardware raster effects sequence for the Amstrad CPC. You define a Frame command (complete hardware timing), add Effect commands at specific scanlines (colour changes, screen position shifts, mode changes), and optionally add Variable commands (runtime synchronization signals) and Subroutine commands (calls to your own code at scheduled scanlines). The CRTC simulator validates timing in real time as you edit; when the setup is correct, click **Export ASM** to write the complete Z80 handler to disk.

Currently supports the **Amstrad CPC**. Raster generators for other systems will be added as system support is extended.

For the complete documentation on the raster effects editor, click [here](doc/usage/raster.md).

<div align="center"><img src="doc/img/raster-editor.png" alt="Raster effects editor" width="800" /></div>

### Build

A Build project item ties everything together. It holds:

- **Assembly sources** — one or more `.asm` / `.z80` files to pass to RASM.
- **Includes** — the include search path list, automatically populated from the folders in your project and any SDK items you have included, so adding a new folder makes it available to all your sources without manual path management.
- **Defines** — preprocessor symbols passed to RASM at assembly time.
- **Dependencies** — an ordered list of other project items (bitmap conversions, tile extractions, etc.) that must be processed before the assembler runs. You add, remove and reorder them in the list.
- **Emulator launch configuration** — executable path and arguments, so you can run the result directly from Retrodev after a successful build.

For the complete documentation on the build project item, click [here](doc/usage/build.md).

### Palette

A Palette project item describes the colour constraints for a section of the screen.

- **Always** — present on every frame (HUD, status bar).
- **Level** — present only during a specific level (tagged with a level name).
- **Screen Zone** — present only while the raster beam is inside a defined scanline range.

The palette solver finds the smallest hardware palette that satisfies all participants simultaneously.

For the complete documentation on the palette project item, click [here](doc/usage/palette.md).

---

## 6. Files Panel

### Images

Images tracked in the Files panel are displayed in a lightweight built-in viewer. Double-clicking an image opens it; from there, or directly via right-click in the Files panel, you can create a project item — Bitmap conversion, Tiles or Sprites — with that image already set as the source. Basic per-pixel inspection (colour picker) is available in the viewer. If the image is a paletised format, the viewer also includes a simple pixel paint editor so you can make small corrections without leaving Retrodev.

Right-clicking anywhere in the Files panel lets you create a new blank image, a new blank text document (useful for starting a new `.asm`, `.z80` or `.as` file), or a new folder directly in the project directory.

### Sources

Source files (`.asm`, `.z80`, `.as`) open in the integrated code editor. The editor provides:

- Syntax highlighting for Z80 assembly (RASM dialect) and AngelScript.
- Auto-complete for labels, symbols and RASM directives in assembly files.
- Auto-complete for the full Retrodev export API (`IBitmapContext`, `ITilesetContext`, `ISpriteContext`, `IMapContext`, `IPaletteContext`) in AngelScript files.
- Code lens — Z80 instructions are annotated with their cycle timings inline, so you can reason about performance without leaving the editor.
- Hover tooltips showing directive documentation, symbol definitions and API method signatures.

For the complete editor documentation, click [here](doc/usage/editor.md).

### Other files

Any other file tracked in the project opens in the data viewer: a hex editor with an ASCII column, suitable for inspecting binary output, include files, and raw data blobs.

<div align="center"><img src="doc/img/hex-editor.png" alt="Hex / data viewer" width="800" /></div>

---

## 7. Building a Project

### Active build selection

You can have multiple Build project items in a project — one per target system, one per configuration, or however you like. A combobox next to the **Build** and **Debug** buttons selects which one is active. All build shortcuts and toolbar buttons operate on the active Build project item.

### Project item dependencies

Execution order within a build is defined by the dependency list inside the active Build project item. You add, remove and reorder entries there. Retrodev processes them in that order before handing off to the assembler.

### Running a build

Click **Build** (or press `F5`) to run the active Build project item:

1. Each dependency in the list is executed in order — bitmap conversions, tile extractions and any other project items — so their output files are ready on disk.
2. RASM assembles the source files defined in the Build project item, with the configured includes and defines. Multiple source files can be defined; each is assembled independently. A typical use has one source file taking a converted bitmap produced by a dependency and packaging it into a `.dsk` file, while another source file assembles the program code and adds it to the same `.dsk`.
3. If the Build project item has an emulator configured, it is launched automatically after a successful build.

Build output, errors and warnings are shown in the **Console** panel at the bottom of the window.

---

## 8. Emulators and Debugging

Retrodev supports launching external emulators directly after a successful build. The currently supported emulators are **WinAPE**, **ACEDL** and **RVM**.

Emulator configuration lives in the **Debug** tab of the Build project item — not in any settings file. You set the emulator executable path using the built-in file picker and configure the launch arguments in the same place. Because the emulator location is machine-specific rather than project-specific, the executable path is stored in the Retrodev application settings on your machine and shared across all your projects; the launch arguments are stored in the project file.

Emulators can be passed media files (`.dsk`, `.cdt`), snapshots, and debug symbol definitions, among other options. Some options are common across all supported emulators; others are emulator-specific — for example, selecting the exact hardware model (such as Amstrad CPC 464 vs 664). Refer to the documentation for the emulator you are using for the full list of supported options.

<div align="center"><img src="doc/img/debugging.png" alt="Emulator launched at breakpoint after Debug" width="800" /></div>

---

## 9. Contributing and Technical Information

Contributions of any kind are welcome. You can report bugs and suggestions, create and submit AngelScript export scripts or Z80 macros to grow the SDK, port Retrodev to another platform, or add support for a new target system. To learn how to contribute in any of these ways, read the [contributing guide](doc/tech/contributing.md).

For details on the source code structure, how to build from source, how to port to other platforms and how to add new systems, see the technical documentation:

- To understand the directory structure and module responsibilities, read the [source layout](doc/tech/source-layout.md).
- To build Retrodev from source using the Kombine build system, read the [building guide](doc/tech/building.md).
- To learn about cross-platform library choices and what is needed to port Retrodev, read the [porting notes](doc/tech/porting.md).
- To add support for a new target system, read the [adding systems guide](doc/tech/adding-systems.md).

### Source code layout (summary)

```
src/
├── cmd/              ← command-line application for launching builds without the GUI (planned)
├── gui/              ← all ImGui views, panels and document editors
│   ├── app/          ← application window and SDL lifecycle
│   ├── dialogs/      ← shared modal dialogs
│   ├── gen/          ← generated code (icons, embedded resources)
│   ├── os/           ← platform-specific GUI integration
│   ├── res/          ← embedded resources (fonts, images, exporters)
│   ├── widgets/      ← reusable ImGui widgets
│   └── views/
│       ├── bitmaps/  ← bitmap conversion editor
│       ├── build/    ← build pipeline editor and RASM integration
│       ├── data/     ← hex/data viewer
│       ├── image/    ← image viewer
│       ├── maps/     ← tile map editor
│       ├── palette/  ← palette solver editor
│       ├── sprites/  ← sprite editor
│       ├── text/     ← code editor
│       └── tiles/    ← tile extraction editor
└── lib/              ← platform-independent core library (no ImGui)
    ├── assets/       ← image, map, palette and source asset types
    ├── convert/      ← bitmap / tile / sprite converters per system
    ├── export/       ← AngelScript export engine and script bindings
    ├── log/          ← logging
    ├── process/      ← image processing (dithering, quantization, colour correction, resize)
    ├── project/      ← project load/save and build item management
    ├── system/       ← system-specific hardware definitions and aspect ratio
    └── utils/        ← shared utilities
```

---

## 10. Thanks and Greetings

Special thanks to the people who directly supported, inspired and shaped Retrodev:

**Roudoudou**, **DemoniakLudo**, **Bladerunner / TLOTB**, **Estrayk / Paradox**, **Mowgly**, and the **Amstrad Power Telegram Channel** — for feedback and ideas.

Greetings fly out to the groups and projects that keep the Amstrad CPC scene alive and thriving: **4Mhz**, **Capasoft**, **CNG**, **The Mojon Twins**, the **CPCTelera** authors, the **8BP** authors, **Batman Group**, **ESP Soft**, the **CPCWiki** team, **Logon System** and all the groups who contributed with information or public source code to keep the retro development alive.

And a big thank you to the wider retro computing community — the sceners, the forum members, the wiki editors, the emulator authors and the hardware hackers — who keep these machines alive and kickin' decades after they left the shelves.

Special thanks also to the authors and maintainers of the open source libraries that make Retrodev possible — see section 11 for the full list.

---

## 11. Open Source Libraries and Licenses

Retrodev is built on the following open source components:

| Library | Use | License |
|---|---|---|
| [Dear ImGui](https://github.com/ocornut/imgui) | Entire UI framework | MIT |
| [SDL3](https://libsdl.org/) | Window, renderer, input, cross-platform file I/O | Zlib |
| [RASM](http://rasm.wikidot.com/) | Embedded Z80 assembler | Custom (see `ext/rasm/`) |
| [AngelScript](https://www.angelcode.com/angelscript/) | Export scripting engine | Zlib |
| [Glaze](https://github.com/stephenberry/glaze) | JSON serialisation | MIT |
| [CTRE](https://github.com/hanickadot/compile-time-regular-expressions) | Compile-time regular expressions | Apache 2.0 |


### ImGui extensions

The following ImGui widgets were adapted from community sources and in several cases substantially rewritten:

| Component | Origin | Notes |
|---|---|---|
| `imgui.text.editor` | Based on [ImGuiColorTextEdit](https://github.com/santaclose/ImGuiColorTextEdit) / [pthom fork](https://github.com/pthom/ImGuiColorTextEdit) | Heavily extended: codelens, autocomplete, hover tooltips, language API injection |
| `imgui.hex.editor` | Based on [imgui_hex_editor](https://github.com/Teselka/imgui_hex_editor) | Adapted for Retrodev data viewer |
| `imgui.splitter` | Public domain ImGui snippet | Rewritten for consistent sizing behaviour |
| `imgui.filedialog` | Based on [ImFileDialog](https://github.com/dfranx/ImFileDialog) | Heavily adapted for Retrodev |
| `imgui.zoomable.image` | Based on [imgui_zoomable_image](https://github.com/danielm5/imgui_zoomable_image) | Mostly rewritten for Retrodev |
| `imgui.spinner` | Community snippet | Adapted |

### Fonts

| Font | License |
|---|---|
| [Fixedsys Excelsior](https://github.com/kika/fixedsys) | Public domain |
| [Material Design Icons](https://materialdesignicons.com/) | SIL Open Font License 1.1 |
| [Ubuntu Font](https://design.ubuntu.com/font/) | Ubuntu Font Licence 1.0 |

### Images

| Image | Author | Notes |
|---|---|---|
| Retrodev Logo & co | DALL-E (AI generated) | |
| Alien sprites and platform tiles | [Mr.Capa](https://capasoft.eu) & [Titan](https://amstradmuseum.emu-france.info) | Example graphics from the game Alien the Xenomorph|

---

## 12. License and Copyright

Copyright © 2026 TLOTB. See [license.md](license.md) for the full MIT license text.
