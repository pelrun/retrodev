// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Help dialog -- built-in documentation viewer with topic navigator.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#include "dialog.help.h"
#include <app/app.h>
#include <app/app.resources.h>
#include <app/app.icons.mdi.h>
#include <SDL3_image/SDL_image.h>

namespace RetrodevGui {

	//
	// Topic indices -- keep in sync with s_topics[] order below
	//
	enum TopicIndex {
		//
		// Getting Started
		//
		TOPIC_WELCOME = 0,
		TOPIC_PROJECTS,
		TOPIC_PROJECT_LAYOUT,
		TOPIC_FILES_PANEL,
		//
		// Project Items
		//
		TOPIC_BITMAPS,
		TOPIC_BITMAP_PIPELINE,
		TOPIC_BITMAP_PARAMS,
		TOPIC_BITMAP_PALETTE,
		TOPIC_BITMAP_RESIZE,
		TOPIC_BITMAP_QUANTIZE,
		TOPIC_BITMAP_EXPORT,
		TOPIC_TILES,
		TOPIC_TILE_EXTRACTION,
		TOPIC_TILE_EXPORT,
		TOPIC_SPRITES,
		TOPIC_SPRITE_EXTRACTION,
		TOPIC_SPRITE_EXPORT,
		TOPIC_MAPS,
		TOPIC_MAP_CONCEPTS,
		TOPIC_MAP_PAINTING,
		TOPIC_MAP_EXPORT,
		TOPIC_PALETTE,
		TOPIC_PALETTE_ZONES,
		TOPIC_BUILD,
		TOPIC_BUILD_SOURCE,
		TOPIC_BUILD_OUTPUT,
		TOPIC_BUILD_DEBUG,
		//
		// Editor
		//
		TOPIC_EDITOR,
		TOPIC_EDITOR_Z80,
		TOPIC_EDITOR_AS,
		TOPIC_EDITOR_SHORTCUTS,
		//
		// Export Scripts
		//
		TOPIC_EXPORTSCRIPTS,
		TOPIC_EXPORTSCRIPTS_ANATOMY,
		TOPIC_EXPORTSCRIPTS_API,
		TOPIC_EXPORTSCRIPTS_LOG,
		//
		// Emulators
		//
		TOPIC_EMULATORS,
		TOPIC_COUNT
	};

	//
	// Section IDs used by the treeview navigator
	//
	enum SectionIndex { SEC_GETTINGSTARTED = 0, SEC_PROJECTITEMS, SEC_EDITOR, SEC_EXPORTSCRIPTS, SEC_EMULATORS, SEC_COUNT };

	//
	// Per-topic record: section it belongs to, display title, body text
	//
	struct HelpTopic {
		int section;
		const char* title;
		const char* content;
	};

	static const char* s_sectionNames[SEC_COUNT] = {
		"Getting Started", "Project Items", "Code Editor", "Export Scripts", "Emulators & Debugging",
	};

	static const HelpTopic s_topics[TOPIC_COUNT] = {
		//
		// -- Getting Started ------------------------------------------------------
		//
		{SEC_GETTINGSTARTED, "Welcome",
		 "Retrodev is a self-contained development environment for retro 8-bit and 16-bit home "
		 "computer software. It replaces the classic fragile toolchain (batch files, scattered "
		 "converters, command-line assemblers) with a single application that handles the full "
		 "workflow:\n\n"
		 "  `Graphics conversion`   Import any modern image and convert it to the native pixel and palette format of the target machine.\n"
		 "  `Tile & sprite extract`  Slice converted images into tiles or sprites.\n"
		 "  `Tile map editor`        Paint multi-layer scrolling maps with parallax preview.\n"
		 "  `Palette solver`         Find a shared hardware palette for all your graphics.\n"
		 "  `Integrated editor`      Write Z80 assembly and AngelScript without leaving the app.\n"
		 "  `Scripted export`        Write AngelScript scripts that produce exact binary output.\n"
		 "  `Project workflow`       Everything lives in a single .retrodev project file.\n\n"
		 "`Supported target systems:`\n\n"
		 "  `Amstrad CPC / CPC+`   Full support\n"
		 "  `ZX Spectrum`          Planned\n"
		 "  `Commodore 64`         Planned\n"
		 "  `MSX`                  Planned"},

		{SEC_GETTINGSTARTED, "Projects",
		 "A Retrodev project (.retrodev) stores all build item configuration and asset references "
		 "in a single file. The project directory and its files are yours to organise freely -- "
		 "Retrodev adds nothing to your folder structure.\n\n"
		 "`File -> New Project`     Create a new project and choose a save path.\n"
		 "`File -> Open Project`    Load an existing .retrodev file from disk.\n"
		 "`File -> Save Project`    Save all changes (`Ctrl+S`).\n"
		 "`File -> Close Project`   Close the current project.\n\n"
		 "Recent projects appear in the `File -> Recent` submenu.\n\n"
		 "Installation requires no installer. Unzip the release archive and run retrodev.exe -- "
		 "all dependencies are bundled."},

		{SEC_GETTINGSTARTED, "Project Layout",
		 "When a project is open, the Project panel shows three independent sections:\n\n"
		 "`SDK`\n"
		 "  Read-only view of the sdk/ folder that ships alongside retrodev.exe. It contains "
		 "Z80 macros, reusable routines and AngelScript helpers shared across all projects. "
		 "Paths into the SDK are stored with a $(sdk)/ prefix and resolved at build time "
		 "regardless of where the SDK folder actually lives on your machine.\n\n"
		 "`Project`\n"
		 "  Entirely virtual -- stores what Retrodev should *do* with your files: bitmap "
		 "conversions, tile extractions, sprite extractions, maps, builds and palettes. "
		 "Nothing here corresponds directly to files on disk. Items can be organised into "
		 "virtual folders by drag-and-drop; the arrangement is cosmetic only.\n\n"
		 "`Files`\n"
		 "  File view rooted at the project folder. These are your raw input files. "
		 "Right-click any file to create a project item with that file already set as source. "
		 "Double-click to open files in the appropriate editor or viewer."},

		{SEC_GETTINGSTARTED, "Files Panel",
		 "The Files panel shows the project directory tree. Files open as follows:\n\n"
		 "  `.png / .bmp / .jpg / .tga`   Image viewer / pixel paint editor\n"
		 "  `.asm / .z80`                 Z80 assembly editor\n"
		 "  `.as`                         AngelScript editor\n"
		 "  Anything else               Hex / data viewer\n\n"
		 "Right-click anywhere in the panel to create a new blank image, a new text "
		 "document or a new folder directly in the project directory.\n\n"
		 "Right-click an image file to add a `Bitmap`, `Tiles` or `Sprites` build item with "
		 "that file already set as the source -- no manual path entry required."},

		//
		// -- Project Items / Bitmaps ----------------------------------------------
		//
		{SEC_PROJECTITEMS, "Bitmap Conversion",
		 "A Bitmap build item converts a source image into the native pixel and colour format "
		 "of the target system. The editor shows the original image on the left and a live "
		 "hardware-converted preview on the right. Every parameter change updates the preview "
		 "automatically.\n\n"
		 "To create: right-click an image in the Files panel and select `Add bitmap conversion`.\n"
		 "To remove: right-click the entry in the Build section and select `Remove`.\n\n"
		 "`Preview controls (both panels):`\n\n"
		 "  `Zoom`       Mouse wheel over the panel.\n"
		 "  `Pan`        Click and drag.\n"
		 "  `Reset`      Double-click to fit.\n"
		 "  `Pixel grid` Appears automatically at high zoom.\n"
		 "  `Info bar`   Shows image name, native resolution, zoom and cursor position.\n\n"
		 "`The converted panel has two extra display options (preview only, not in output):`\n\n"
		 "  `Aspect`     Applies hardware pixel aspect ratio correction.\n"
		 "  `Scanlines`  Overlays a CRT scanline effect (only when Aspect is enabled)."},

		{SEC_PROJECTITEMS, "Conversion Pipeline",
		 "The conversion runs in a fixed order:\n\n"
		 "  `1. Resize`          Scale the source image to the target resolution.\n"
		 "  `2. Colour Correction`  Per-pixel adjustments before palette matching.\n"
		 "  `3. Quantization`    Map each pixel to the closest hardware colour.\n"
		 "  `4. Dithering`       Distribute quantization error to neighbouring pixels.\n\n"
		 "Understanding this order matters when tuning: colour correction affects what "
		 "the quantizer sees, and both steps operate on the already-resized image at "
		 "native target resolution.\n\n"
		 "`Target system and mode:`\n\n"
		 "Selects the hardware to convert for. The choice of system determines which video "
		 "modes, resolutions and palette types are available. The screen mode controls both "
		 "the horizontal resolution and the maximum number of colours the hardware can "
		 "display at once.\n\n"
		 "`Resolution presets:`\n\n"
		 "  `Normal`    Standard screen area for the selected mode.\n"
		 "  `Overscan`  Extended hardware area beyond the standard boundary.\n"
		 "  `Custom`    Arbitrary target dimensions.\n"
		 "  `Original`  Uses the source image dimensions, no rescaling.\n\n"
		 "`Palette type` selects which hardware palette the converted image will target. "
		 "The available types depend on the selected system -- a system may expose more "
		 "than one when its hardware provides distinct colour sets for different purposes. "
		 "Changing the palette type invalidates the current palette configuration."},

		{SEC_PROJECTITEMS, "Conversion Parameters",
		 "Quantization settings:\n\n"
		 "  `Smoothness`          Blend each pixel with its four neighbours before matching "
		 "(reduces high-frequency noise, softens hard edges).\n"
		 "  `Sort palette`        Reorder pen slots for consistent palette ordering.\n"
		 "  `Reduction method`    `Higher Frequencies`: fills pens by most-common colour. "
		 "`Higher Distances`: spreads pens across colour space.\n"
		 "  `Reduction before dithering`  When on, colour reduction runs before dithering.\n"
		 "  `Use source palette`  Copy palette entries from a paletised PNG as-is.\n\n"
		 "Dithering methods (selection):\n\n"
		 "  `None`                No dithering -- sharpest result, most obvious banding.\n"
		 "  `Floyd-Steinberg`     Classic error diffusion, best for photographic sources.\n"
		 "  `Bayer 2 (4x4)`       Regular dot pattern, good general-purpose ordered dither.\n"
		 "  `ZigZag 2 (4x3)`      Non-square diagonal matrix, suits wide-pixel modes.\n"
		 "  `Checkerboard Heavy`  Strong alternating pattern, coarse texture.\n"
		 "  (+ 20 further methods -- see the full documentation)\n\n"
		 "Dithering strength: 0% to 400%%. Error diffusion can be layered on top of any "
		 "matrix method. `Pattern mode` alternates scanline pairs for hardware-style "
		 "interlaced palette mixing."},

		{SEC_PROJECTITEMS, "Palette Controls",
		 "The Palette panel (bottom of the left area) shows one slot per hardware pen.\n\n"
		 "`Lock`\n"
		 "  A locked pen keeps its assigned hardware colour. The quantizer may still use "
		 "it but will not reassign it. Use this to guarantee a fixed pen -- e.g. the "
		 "background colour.\n\n"
		 "`Enable / Disable`\n"
		 "  A disabled pen is completely excluded from quantization. Use this to reserve "
		 "pens for colours loaded at runtime by your program.\n\n"
		 "`Colour assignment`\n"
		 "  Click a pen to assign a specific hardware colour manually. A pen with a "
		 "manually assigned colour behaves as locked.\n\n"
		 "`Transparent colour`\n"
		  "  When enabled, pixels matching a chosen RGB colour (within a configurable "
		  "tolerance) are excluded from quantization entirely. The colour does not occupy "
		  "any palette pen -- it simply disappears from the converted output.\n\n"
		  "  This is suitable only when exporting as code or a format that does not carry "
		  "per-pixel colour data. For RGBA images with a real alpha channel, pixels with "
		  "`alpha=0` are automatically skipped without enabling this option.\n\n"
		  "`Transparent pen`\n"
		  "  For raw-data exports where a transparent pixel must map to a specific pen "
		  "index at runtime (e.g. sprites drawn with a mask colour), set this to the "
		  "desired pen number. Use this together with the `Transparent colour` option.\n\n"
		  "  The palette solver treats the selected slot specially across all three passes:\n"
		  "  - The quantizer never assigns any colour to the reserved slot.\n"
		  "  - After capping, the solver remaps the fixed palette so that colours needed "
		  "by this asset do not land in the reserved slot.\n"
		  "  - The participant result line shows the number of assigned pens plus "
		  "'+1 transparent (pen N)' when the option is active.\n\n"
		  "  All assets sharing screen space should ideally use the same transparent pen "
		  "index. If multiple assets declare different transparent pen indices, the solver "
		  "emits a warning in the solution summary.\n\n"
		  "  Set Transparent pen to None to disable (no pen is dedicated to transparency)."},

		{SEC_PROJECTITEMS, "Resize & Colour Correction",
		 "Resize scale modes:\n\n"
		 "  `Fit`          Stretch to exactly fill target (does not preserve aspect ratio).\n"
		 "  `Smallest`     Scale uniformly to fit inside the target rectangle (letterbox).\n"
		 "  `Largest`      Scale uniformly to fill the target rectangle, crop the overflow.\n"
		 "  `Custom`       Sample a user-defined rectangle from the source.\n"
		 "  `Original`     Copy at 1:1 pixel scale with no resampling.\n\n"
		 "Interpolation modes: `Nearest Neighbor`, `Low`, `Bilinear`, `High Bilinear`, `Bicubic`, "
		 "`High Bicubic`, `High`. Nearest Neighbor is best for indexed-colour pixel art.\n\n"
		 "Colour correction (applied in order after resize):\n\n"
		 "  `Red / Green / Blue`    Per-channel multipliers (100 = unchanged).\n"
		 "  `Contrast`              Scales luminance around the midpoint.\n"
		 "  `Brightness`            Scales overall luminosity.\n"
		 "  `Saturation`            0% = greyscale, 100% = unchanged, >100% = vivid.\n"
		 "  `Colour bits`           Reduces bit depth per channel (24/12/9/6 bits total).\n"
		 "  `Palette reduction lower limit`  OR mask raising the minimum per-channel value.\n"
		 "  `Palette reduction upper limit`  AND mask clipping the maximum per-channel value."},

		{SEC_PROJECTITEMS, "Quantization & Dithering",
		 "Quantization maps each pixel to the closest hardware colour and builds a frequency "
		 "histogram for the colour reduction step.\n\n"
		 "Reduction methods:\n\n"
		 "  `Higher Frequencies`   Fills each pen with the most-common colour in the image "
		 "histogram, in descending order of frequency.\n"
		 "  `Higher Distances`     Assigns first by frequency, then alternates frequency and "
		 "maximum colour distance. Spreads pens across the colour space.\n\n"
		 "Dithering distributes quantization error to neighbouring pixels. Methods:\n\n"
		 "  `None`                 No dithering.\n"
		 "  `Floyd-Steinberg`      Error diffusion (7/16, 3/16, 5/16, 1/16 weights).\n"
		 "  `Bayer 1/2/3`          Ordered Bayer matrices (2x2, 4x4 and variant 4x4).\n"
		 "  `Ordered 1-4`          Sequential ordered matrices (2x2, 3x3, 4x4, 8x8).\n"
		 "  `ZigZag 1/2/3`         Diagonal scatter matrices (3x3, 4x3, 5x4).\n"
		 "  `Checkerboard Heavy / Light / Alternate`\n"
		 "  `Diagonal Wave`        Horizontal stripe with diagonal offset.\n"
		 "  `Sparse Vertical / Horizontal`\n"
		 "  `Cross Pattern / Cluster Dots`\n"
		 "  `Gradient Horizontal / Diagonal`\n\n"
		 "Strength: 0% to 400%. Error diffusion can be layered on any matrix method."},

		{SEC_PROJECTITEMS, "Bitmap Export",
		 "To write converted data to disk, attach an AngelScript export script via the "
		 "Export section of the build item. The script receives:\n\n"
		 "  `Image@`                  The converted image (pixel access via `GetPixelColor`).\n"
		 "  `outputPath`              The configured output file path.\n"
		 "  `BitmapExportContext@`    Native width/height, palette, transparency settings, target system/mode, and all declared `@param` values.\n\n"
		 "Transparency:\n\n"
		 "  `GetUseTransparentColor()`  True when transparent colour handling is enabled.\n"
		 "  `GetTransparentPen()`       Pen index for transparent pixels (-1 if none set).\n\n"
		 "The script has full control over binary layout, headers and compression."},

		//
		// -- Project Items / Tiles ------------------------------------------------
		//
		{SEC_PROJECTITEMS, "Tile Extraction",
		 "A Tiles build item converts a source image and then slices the converted result "
		 "into a regular grid of tiles. Each tile can be individually previewed and "
		 "selectively excluded from export.\n\n"
		 "To create: right-click an image in the Files panel and select `Add tileset conversion`.\n\n"
		 "`Document tabs:`\n\n"
		 "  `Conversion`        Identical to the Bitmap editor -- same parameters and preview.\n"
		 "  `Tile Extraction`   Define the grid and manage individual tiles.\n\n"
		 "`Tile Extraction tab layout:`\n\n"
		 "  `Left top`     Dual viewer -- full converted image (left), selected tile (right).\n"
		 "  `Left bottom`  Tile list -- 64x64 px thumbnails in a scrollable grid.\n"
		 "  `Right`        Grid parameters and export widget."},

		{SEC_PROJECTITEMS, "Tile Extraction Parameters",
		 "All parameters are in the Tile Extraction collapsible section:\n\n"
		 "  `Tile Width / Height`    Size of each tile in pixels (minimum 1).\n"
		 "  `Offset X / Y`           Pixel offset from the image edge before the grid starts.\n"
		 "  `Padding X / Y`          Gap in pixels between adjacent tiles.\n\n"
		 "`Below the parameters the panel shows:`\n\n"
		 "  `Tiles Extracted`   Current tile count.\n"
		 "  `Grid Size`         Computed grid dimensions (columns x rows).\n\n"
		 "Click `Extract Tiles` to re-run conversion and re-slice the image. Extraction "
		 "must be triggered manually after changing parameters.\n\n"
		 "`Remove Duplicates` (below Extract Tiles):\n\n"
		 "Compares raw pixel data of every non-deleted tile against all earlier tiles. "
		 "Any tile identical to a previous one is marked as deleted. "
		 "The count removed is logged to the Console. "
		 "Disabled until at least one tile has been extracted.\n\n"
		 "`Undelete All` (below Remove Duplicates):\n\n"
		 "Clears the entire deleted-tiles list, restoring all deleted tiles to active in one step. "
		 "The count restored is logged to the Console. "
		 "Disabled when there are no deleted tiles.\n\n"
		 "`Pack to Grid:`\n\n"
		 "Use this when the source image has tile graphics scattered irregularly across a solid "
		 "background (e.g. a magenta fill) rather than already arranged in a regular grid. "
		 "Pack to Grid detects every content region automatically and finds the smallest detected "
		 "region's dimensions as the base tile unit. Any region larger than that unit is "
		 "subdivided into cells of that size -- this handles composite background graphics made "
		 "of several tiles placed side by side. All resulting cells are rearranged into a "
		 "uniform grid image. The tile width, height, offset, and padding parameters are then "
		 "set automatically to match the detected cell size. Tiles are extracted immediately "
		 "from the packed result.\n\n"
		 "  `Background`   The solid separator colour (e.g. magenta). Use `Sample` to read\n"
		 "                 it from pixel (0,0) of the converted image.\n"
		 "  `Merge gap`    Pixel gap at which nearby sub-regions are merged into one bounding\n"
		 "                 box. Merged boxes larger than the smallest tile unit are subdivided.\n"
		 "  `Cell padding` Gap in pixels between cells in the packed output image.\n"
		 "  `Columns`      Grid columns in the packed output (0 = automatic).\n\n"
		 "Pressing `Extract Tiles` after packing reverts to the normal converter output.\n\n"
		 "`Tile list multiselection:`\n\n"
		 "Plain click selects a single tile and clears the previous selection. "
		 "Ctrl+click toggles a tile in or out of the selection. "
		 "Shift+click range-selects from the last clicked tile to the current one. "
		 "The primary tile (last clicked) is shown with a gold border; other selected tiles have a white border.\n\n"
		 "`Deleting and restoring tiles:`\n\n"
		 "Right-click a tile to open the context menu. If the right-clicked tile is not already "
		 "selected it becomes the sole selection first. The menu operates on the full selection and "
		 "adapts to the mix: all-active shows Delete / Delete All; all-deleted shows Undelete / "
		 "Undelete All; mixed shows Delete Active and Undelete Deleted as separate entries. "
		 "Deleted tiles appear as a dark red placeholder with a red cross. They remain in the list "
		 "but are excluded from export. Changing any grid parameter clears the deleted-tiles list.\n\n"
		 "Tile extraction always operates on the native-resolution converted image."},

		{SEC_PROJECTITEMS, "Tile Export",
		 "Extracted tiles and the deleted-tiles list are available to export scripts "
		 "via the TilesetExportContext API:\n\n"
		 "  `GetTileCount()`          Number of extracted (non-deleted) tiles.\n"
		 "  `GetTileWidth()` / `GetTileHeight()`  Tile dimensions in pixels.\n"
		 "  `GetTile(tileIdx)`        Returns an `Image@` for the tile.\n"
		 "  `GetPalette()`            Returns the `Palette@` for per-pixel pen lookup.\n"
		 "  `GetUseTransparentColor()`  True when transparent colour handling is enabled.\n"
		 "  `GetTransparentPen()`       Pen index for transparent pixels (-1 if none set).\n"
		 "  `GetParam(key)`           Value of a declared `@param`.\n\n"
		 "Once extracted, the tileset is also available as a source in the Map editor."},

		//
		// -- Project Items / Sprites ----------------------------------------------
		//
		{SEC_PROJECTITEMS, "Sprite Extraction",
		 "A Sprites build item combines a full bitmap conversion with a sprite region "
		 "definition step. `The editor has two tabs:`\n\n"
		 "  `Conversion`         Identical to the Bitmap editor.\n"
		 "  `Sprite Extraction`  Define, preview and manage individual sprite regions.\n\n"
		 "To create: right-click an image in the Files panel and select `Add sprites conversion`.\n\n"
		 "`Sprite Extraction tab layout:`\n\n"
		 "  `Left top`     Dual viewer -- full converted image (left), selected sprite (right). Each panel has independent `Aspect` and `Scanlines` controls.\n"
		 "  `Left bottom`  Sprite list -- thumbnail grid of all defined sprites.\n"
		 "  `Right`        Sprite Extraction section and Export section.\n\n"
		 "`Sprite list multiselection:`\n\n"
		 "Plain click selects a single sprite and clears the previous selection. "
		 "Ctrl+click toggles a sprite in or out of the selection. "
		 "Shift+click range-selects from the last clicked sprite to the current one. "
		 "The primary sprite (last clicked) is shown with a gold border; other selected sprites have a white border. "
		 "When multiple sprites are selected the hover tooltip shows the total count."},

		{SEC_PROJECTITEMS, "Defining & Editing Sprites",
		 "`Adding a sprite:`\n\n"
		 "  1. Click `Add Sprite` in the tooling panel to enter selection mode.\n"
		 "  2. Click and drag on the converted image to draw a rectangular region.\n"
		 "  3. Enter a name in the `Sprite Name` field (auto-generated by default).\n"
		 "  4. Click `Done` to confirm. Click `Cancel` to discard.\n\n"
		 "All sprite regions are cut from the native-resolution converted image, not from "
		 "the original source or the aspect-corrected preview.\n\n"
		 "`Editing a selected sprite:`\n\n"
		 "  `X / Y Position`   Top-left corner of the region (minimum 0).\n"
		 "  `Width / Height`   Region size in pixels (minimum 1).\n"
		 "  `Name`             Label used to identify the sprite in export scripts.\n\n"
		 "Any change immediately re-runs extraction. Click `Delete Sprite` to remove the "
		 "selected sprite.\n\n"
		 "`Context menu on sprite thumbnails:`\n\n"
		 "Right-click any thumbnail to open the context menu. If the right-clicked sprite is not "
		 "already selected it first becomes the sole selection. The menu operates on the full "
		 "selection; a header shows the count when multiple sprites are selected. It has four groups:\n\n"
		 "`Duplicate` submenu — appends a copy to the list, extracts immediately and selects it:\n"
		 "  `Duplicate`                    Plain copy. Name gets `_copy` suffix.\n"
		 "  `Duplicate + Flip Horizontal`  Copy mirrored left-to-right. Name gets `_fh` suffix.\n"
		 "  `Duplicate + Flip Vertical`    Copy mirrored top-to-bottom. Name gets `_fv` suffix.\n"
		 "  `Duplicate + Shift Left/Right/Up/Down`\n"
		 "                                 Copy shifted one pixel in the chosen direction (cyclic wrap).\n"
		 "                                 Name gets `_sl`, `_sr`, `_su` or `_sd` suffix.\n\n"
		 "`Flip` submenu — transforms the sprite in place (toggles; re-applying restores original):\n"
		 "  `Flip Horizontal`  Mirrors left-to-right.\n"
		 "  `Flip Vertical`    Mirrors top-to-bottom.\n\n"
		 "`Shift` submenu — moves pixel content one step in place with cyclic wrap:\n"
		 "  `Shift Left / Right / Up / Down`\n\n"
		 "`Remove` — removes all selected sprites from the list.\n\n"
		 "Duplicate operations create one copy per selected sprite, append each copy to the end of "
		 "the list, extract immediately, and select the new entries. In-place flip and shift "
		 "operations re-run extraction immediately on every selected sprite.\n\n"
		 "`Pixel painting:`\n\n"
		 "Select a colour from the palette list in the painting widget, then click a pixel "
		 "in the sprite preview to paint it. Changes persist with the project."},

		{SEC_PROJECTITEMS, "Sprite Export",
		 "Sprites are available to export scripts via the SpriteExportContext API:\n\n"
		 "  `GetSpriteCount()`          Number of extracted sprites.\n"
		 "  `GetSpriteWidth(idx)`       Width in pixels.\n"
		 "  `GetSpriteHeight(idx)`      Height in pixels.\n"
		 "  `GetSpriteName(idx)`        Name label.\n"
		 "  `GetSprite(idx)`            Returns an `Image@` for the sprite.\n"
		 "  `GetPalette()`              Returns the `Palette@` for pen lookup.\n"
		 "  `GetUseTransparentColor()`  True when transparent colour handling is enabled.\n"
		 "  `GetTransparentPen()`       Pen index for transparent pixels (-1 if none set).\n"
		 "  `GetParam(key)`             Value of a declared `@param`."},

		//
		// -- Project Items / Maps -------------------------------------------------
		//
		{SEC_PROJECTITEMS, "Map Editor",
		 "The Map editor lets you paint tile maps across one or more layers with "
		 "independent parallax scroll speeds. Maps store only tile slot and index "
		 "references -- they have no dependency on any particular hardware or colour mode. "
		 "Aspect ratio preview is a display aid only.\n\n"
		 "To create: right-click inside the Project section and select `New Map...`\n\n"
		 "`Document layout:`\n\n"
		 "  `Left -- canvas area`    Toolbar strip, tile canvas, X/Y scroll controls.\n"
		 "  `Right -- tooling panel` Name field, export widget, and collapsible sections: Layers, Dimensions, Viewport, Tilesets, Groups, Tiles.\n\n"
		 "`Toolbar controls:`\n\n"
		 "  `Show viewable area`   Toggle viewport overlay (blue shading outside viewport).\n"
		 "  `Show grid`            Toggle tile cell border lines.\n"
		 "  `System / Mode`        Select target for aspect-ratio-correct canvas preview.\n\n"
		 "Painting: left-click / drag to paint, right-click to erase. All operations "
		 "target the editing layer only."},

		{SEC_PROJECTITEMS, "Map Concepts",
		 "`Layers`\n\n"
		 "A map can have any number of layers rendered bottom-to-top. Each layer has its "
		 "own tile matrix and dimensions -- layers do not need to share the same size.\n\n"
		 "  `Name`           Label shown in the Layers panel.\n"
		 "  `Width / Height` Dimensions in tiles.\n"
		 "  `Speed`          Scroll speed in tiles per camera step. 1.0 = one tile/step (foreground).\n"
		 "                   Parallax scroller: assign each layer a different speed (e.g. 0.25\n"
		 "                   sky, 0.5 background, 1.0 foreground) to preview depth separation.\n"
		 "                   Screen-by-screen game: set speed = viewport width (or height) so\n"
		 "                   each camera step advances a full screen (room-to-room transition).\n"
		 "  `Offset X / Y`   Positional offset in tiles (fractional values allowed).\n"
		 "  `Visible`        Toggle visibility without deleting the layer data.\n\n"
		 "`Tileset slots`\n\n"
		 "A slot index (1-15) is what gets encoded in map cells. Each slot holds one or "
		 "more variants (different tileset build items) that can be swapped without "
		 "changing any tile index in the map. The active variant is used on the canvas "
		 "and in export.\n\n"
		 "`Groups`\n\n"
		 "A group is a rectangular multi-tile stamp captured from the editing layer for "
		 "quick reuse. Click `Add Group`, drag a rectangle on the canvas, release to save.\n\n"
		 "`Viewport`\n\n"
		 "Defines the visible tile area (width x height). Used for the viewable-area "
		 "overlay and for scroll range calculations.\n\n"
		 "Cell encoding: each cell is a 16-bit word. Bits 15-12 = slot index+1 (0=empty),"
		 " bits 11-0 = tile index within the slot (0-4095)."},

		{SEC_PROJECTITEMS, "Map Painting & Scrollbars",
		 "Painting tools:\n\n"
		 "  `Single tile`    Select a tile in the Tiles panel; left-click places it.\n"
		 "  `Group stamp`    Select a group in the Groups panel; left-click stamps it.\n"
		 "                 A semi-transparent preview follows the cursor before clicking.\n"
		 "  `Erase`          Right-click with any tool selected.\n\n"
		 "`Scrollbars:`\n\n"
		 "Each axis has arrow buttons and a slider. Each step moves the camera by one "
		 "unit. The actual tile offset per layer is step x layerSpeed, clamped so no "
		 "layer scrolls past its own edge. The slider range is computed from the widest "
		 "and tallest layer.\n\n"
		 "Arrow keys scroll the canvas when it is focused or hovered.\n\n"
		 "`Dimensions panel (editing layer only):`\n\n"
		 "  `Width / Height` + `Apply`   Resize the layer, preserving data in the overlap.\n"
		 "  `Row / Col` buttons        Add or remove rows/columns at any edge."},

		{SEC_PROJECTITEMS, "Map Export",
		 "Map data is available to export scripts via the MapExportContext API:\n\n"
		 "  `GetLayerCount()`                Number of layers.\n"
		 "  `GetLayerWidth(layerIdx)`        Width in tiles.\n"
		 "  `GetLayerHeight(layerIdx)`       Height in tiles.\n"
		 "  `GetLayerName(layerIdx)`         Layer name.\n"
		 "  `GetCell(layerIdx, col, row)`    Encoded 16-bit cell word.\n"
		 "  `GetParam(key)`                  Value of a declared `@param`.\n\n"
		 "`Cell word format:`\n\n"
		 "  `Bits 15-12`   tilesetSlotIndex + 1  (0 = empty, 1-15 = slot)\n"
		 "  `Bits 11-0`    tile index within that slot (0-4095)"},

		//
		// -- Project Items / Palette ----------------------------------------------
		//
		{SEC_PROJECTITEMS, "Palette Solver",
		 "The Palette build item solves colour assignment constraints across multiple "
		 "graphics that share screen space. It computes a shared palette and writes the "
		 "result back into participating build items (bitmaps, tilesets, sprites) so "
		 "their subsequent conversions use the solved palette automatically.\n\n"
		 "To create: right-click inside the Project section and select `New Palette...`\n\n"
		 "`Document layout:`\n\n"
		 "  `Left panel`         Target system and palette type, zone list, thumbnails.\n"
		 "  `Right top panel`    Zone properties and participant list.\n"
		 "  `Right bottom panel` Solve / Validate controls, result list, solved palette.\n\n"
		 "`Participants:`\n\n"
		 "  `Always`       Present in every zone and every level. Colors pooled globally.\n"
		 "  `Zone Always`  Always present in this zone across all levels, not shared across zones.\n"
		 "  `Level`        Present only in this zone when a named level is active (tagged).\n\n"
		 "Level participants also require a `Tag` -- a free-text string (e.g. `Level1`) grouping "
		 "graphics that appear together. Only participants sharing the same tag are solved together.\n\n"
		 "Only build items configured for the same target system and palette type are "
		 "available as participants.\n\n"
		 "`Original palette panel:`\n\n"
		 "When a participant is selected in the list, the right sub-panel shows its original "
		 "palette -- the palette that item's own conversion produced before the solver overrides it. "
		 "This is read-only and provided as a reference.\n\n"
		 "`Pre-loaded palette:`\n\n"
		 "Before solving you can lock specific pen slots to fixed hardware colours. Locked slots "
		 "are injected at the head of the global Always union in Pass 1, guaranteeing they occupy "
		 "the same pen positions in every zone and every level solution. Click a colour swatch in "
		 "the pre-loaded palette widget to lock or unlock it. Changing any locked slot immediately "
		 "invalidates the current solution. Pre-loaded slots persist in the project file and are "
		 "cleared only when the target system or palette type is changed.\n\n"
		 "`Thumbnails:`\n\n"
		 "Below the zone list, participant thumbnails are shown for the selected zone. After a "
		 "successful solve each thumbnail displays the image as converted with the solved palette. "
		 "A `Level tag` combo above the area filters which Level participants are shown. "
		 "Clicking a thumbnail selects it and shows its solved preview in the right bottom panel.\n\n"
		 "`Overflow method:`\n\n"
		 "Controls how the solver handles colour counts that exceed the pen budget:\n"
		 "  `Hard Cap`       Truncate the union list at the pen limit.\n"
		 "  `Soft Cap`       Replace the nearest accepted entry with the RGB midpoint.\n"
		 "  `Weighted Blend` Like Soft Cap but blends 67%% accepted + 33%% overflow.\n"
		 "  `Median`         Replace accepted entry with the RGB centroid of the cluster.\n\n"
		 "`Transparent pen and the solver:`\n\n"
		 "If a participant has a Transparent pen configured (see Bitmap Palette Controls), "
		 "the solver handles that pen slot specially in all three passes:\n"
		 "  - The free-quantization pass never assigns any colour to the reserved slot.\n"
		 "  - After capping, the fixed palette is remapped so that colours needed by "
		 "transparency-using assets are moved out of the reserved slot if possible.\n"
		 "  - The participant result line shows the normal assigned pen count plus "
		 "'+1 transparent (pen N)' as an annotation.\n\n"
		 "All participants sharing screen space should ideally declare the same transparent "
		 "pen index. If different indices are detected the solver emits a warning in the "
		 "solution summary."},

		{SEC_PROJECTITEMS, "Zones & Solving",
		 "`Screen zones`\n\n"
		 "A screen zone is a horizontal scanline band with its own hardware palette. "
		 "Raster interrupts fire at a specific scanline and swap palette registers "
		 "mid-frame -- this lets the top and bottom halves of the screen use completely "
		 "different colours.\n\n"
		 "Each zone has a Name, first/last scanline (Lines) and a screen Mode. Different "
		 "zones may use different modes. A new palette item starts with one default zone "
		 "named \"Main\".\n\n"
		 "`Three-pass solve:`\n\n"
		 "  `Pass 1`  Global Always base -- all Always participants from every zone quantized "
		 "together. These receive priority in the pen budget.\n"
		 "  `Pass 2`  Zone base -- Zone Always participants fitted on top of the global base.\n"
		 "  `Pass 3`  Level tags -- Level participants for each (zone x tag) combination "
		 "fitted on top of the zone base.\n\n"
		 "The solve does not modify the project; it produces a result in memory only.\n\n"
		 "`Solution result list:`\n\n"
		 "After solving, each (zone x tag) entry is colour-coded green (fits) or red (overflow), "
		 "with pen usage shown (e.g. `4/16`). Per-participant result icons:\n\n"
		 "  `OK`       All colours fit.\n"
		 "  `Overflow` More unique colours than free pens available.\n"
		 "  `Missing`  Build item not found or source image could not be loaded.\n"
		 "  `Skipped`  Participant does not apply to this context.\n\n"
		 "For level solutions, inherited base participants (Always and Zone Always) are listed "
		 "first, then the participants specific to that level tag.\n\n"
		 "`Overflow remap strip:`\n\n"
		 "When colours exceed the pen budget, coloured swatches appear below the participant "
		 "results -- one per overflow colour. Hover a swatch to see the remap details: which "
		 "accepted slot it was nearest to, and whether that slot was updated, absorbed unchanged, "
		 "or dropped.\n\n"
		 "`Solved palette strip:`\n\n"
		 "A compact row of pen swatches shows the full solved palette. Occupied slots show "
		 "their hardware colour; free slots appear dark grey. Hover a swatch to see the "
		 "pen number, system colour index and RGB values.\n\n"
		 "`Solution preview:`\n\n"
		 "Click a thumbnail in the left panel to preview it in the right bottom panel, "
		 "rendered with the solved palette exactly as the solver produced it.\n\n"
		 "`Validate` writes solved palette assignments back into each participant's stored "
		 "configuration. Validate is available after any solve, including imperfect ones where "
		 "colours were remapped due to pen overflow. Clicking Validate on an imperfect solution "
		 "records a user-validated flag in the project -- the build pipeline will then re-run the "
		 "solver and apply assignments even when the solution is not perfect, logging a warning to "
		 "the Build output channel. The flag is automatically cleared whenever anything affecting "
		 "the solve outcome changes (participants, zones, mode, pre-loaded palette, or an "
		 "externally modified participating item). All open documents for affected items refresh "
		 "their previews immediately. Save the project to persist the changes."},

		//
		// -- Project Items / Build ------------------------------------------------
		//
		{SEC_PROJECTITEMS, "Build Pipeline",
		 "A Build item ties assembler sources, project dependencies and an emulator "
		 "launch configuration into a single reproducible pipeline.\n\n"
		 "To create: click `Add -> Build` in the Project panel.\n\n"
		 "`Triggering a build:`\n\n"
		 "  `Toolbar combo box`   Selects the active Build item.\n"
		 "  `Build button`        Runs the active Build item.\n"
		 "  `Debug button (F5)`   Builds and launches the configured emulator on success.\n"
		 "  `Amber floppy icon`   Appears on unsaved changes -- click to save immediately.\n\n"
		 "Before invoking the assembler, Retrodev saves open documents and processes "
		 "all declared dependencies in order. Build output appears in the Console panel."},

		{SEC_PROJECTITEMS, "Build -- Source & Includes",
		 "`Source tab layout (two columns):`\n\n"
		 "`Left -- Dependencies`\n\n"
		 "  An ordered list of other project items (Bitmap conversions, Tilesets, "
		 "Sprites, Maps, Palettes) processed before the assembler runs. Use `+` to add, "
		 "`-` to remove, arrows to reorder. Each entry is shown as name [Type].\n\n"
		 "`Right -- Sources`\n\n"
		 "  List of `.asm` / `.z80` source files passed to RASM. Each is assembled "
		 "independently. Use `+` to pick from files tracked in the project.\n\n"
		 "`Right -- Include Directories`\n\n"
		 "  Include search path passed to the assembler. Use `+` to pick from folders "
		 "already tracked in the project. Retrodev formats the list into the appropriate "
		 "flags automatically.\n\n"
		 "`Right -- Defines`\n\n"
		 "  Preprocessor symbols injected before assembly. Type `KEY=value` or a bare flag "
		 "name and press `Add`."},

		{SEC_PROJECTITEMS, "Build -- Output & RASM Options",
		 "`Build tab` -- selects the assembler (currently RASM) and its flags:\n\n"
		 "  `-w`    Suppress all warnings.\n"
		 "  `-twe`  Treat warnings as errors.\n"
		 "  `-wu`   Warn on unused symbols.\n"
		 "  `-me N` Stop after N errors.\n"
		 "  Various compatibility flags: `-ass`, `-uz`, `-pasmo`, `-dams`, `-m`, `-amper`, `-fq`, `-utf8`\n"
		 "  Macro flags: `-void`, `-mml`\n"
		 "  Diagnostic flags: `-v`, `-map`, `-cprquiet`\n\n"
		 "A read-only `Generated options` field shows the exact flags passed to RASM.\n\n"
		 "`Output tab` -- configures output file paths:\n\n"
		 "  Binary, ROM, Cartridge, Snapshot, Tape, Symbol, Breakpoint files.\n"
		 "  Output radix: automatic from source filename (`-oa`) or explicit (`-o`).\n"
		 "  Symbol export formats: `Default`, `Pasmo`, `WinAPE`, `Custom`."},

		{SEC_PROJECTITEMS, "Build -- Debug & Emulator",
		 "`Debug tab`:\n\n"
		 "  RASM snapshot options: `-ss` (embed symbols), `-sb` (embed breakpoints), `-v2` "
		 "(version 2 snapshot).\n\n"
		 "`Emulator Launch:`\n\n"
		 "  Select emulator: `None`, `WinAPE`, `RVM` or `ACE-DL`.\n\n"
		 "The executable path is stored in application settings on your machine (shared "
		 "across all projects). All other settings are stored in the project file.\n\n"
		 "Media: disc / tape / cartridge image (`.dsk`, `.hfe`, `.cdt`, `.xpr`, `.cpr`, `.sna`) "
		 "and snapshot file.\n\n"
		 "Startup command varies by emulator:\n\n"
		 "  `WinAPE`    Program to run (`/A:`).\n"
		 "  `RVM`       BASIC command (`-command=`). Use `\\n` for Enter key.\n"
		 "  `ACE-DL`    Auto-run file (`-autoRunFile`).\n\n"
		 "Emulator-specific options -- RVM: machine type. ACE-DL: CRTC type, RAM, "
		 "firmware, speed, and additional launch flags."},

		//
		// -- Code Editor ---------------------------------------------------------
		//
		{SEC_EDITOR, "Code Editor Overview",
		 "The integrated code editor supports Z80 assembly (RASM dialect) and AngelScript. "
		 "It is used for both assembler sources and export scripts without leaving Retrodev.\n\n"
		 "Supported languages:\n\n"
		 "  `Z80 Assembly` (`.asm`, `.z80`)\n"
		 "    Syntax highlighting, directive completion, label/symbol autocomplete,\n"
		 "    codelens timing annotations, hover documentation.\n\n"
		 "  `AngelScript` (`.as`)\n"
		 "    Syntax highlighting, Retrodev export API autocomplete, hover documentation.\n\n"
		 "`Toolbar status line (left side):`\n\n"
		 "  line/column  total lines  Ins  *  language  filename\n\n"
		 "  `*` appears when the document has unsaved changes.\n\n"
		 "`Font scale slider (right side):` 0.5x - 3.0x, affects only editor text."},

		{SEC_EDITOR, "Z80 Assembly Features",
		 "`Syntax highlighting`\n\n"
		 "Keywords, directives, registers, opcodes, labels, strings, comments and numeric "
		 "literals are all coloured independently.\n\n"
		 "`Codelens -- instruction timing annotations`\n\n"
		 "Every Z80 instruction line is annotated with its timing inline in the margin. "
		 "On the Amstrad CPC the relevant unit is NOPs: 1 NOP = 4 T-states (1 us at "
		 "4 MHz). Three display modes:\n\n"
		 "  `Cycles`       T-state count per instruction.\n"
		 "  `Cycles+M1`    T-states with M1 opcode-fetch cycle counted separately.\n"
		 "  `Instructions` NOPs equivalent per instruction.\n\n"
		 "`Auto-complete`\n\n"
		 "Completions come from: RASM directives, labels and symbols from all open source "
		 "files in the project, and Z80 register names and opcodes. Suppressed inside "
		 "comments.\n\n"
		 "`Hover documentation`\n\n"
		 "Hover over a label or directive to see its definition or documentation. "
		 "Suppressed over comment tokens.\n\n"
		 "`Find and Replace`\n\n"
		 "`Ctrl+F` opens Find. `Ctrl+H` opens Replace. `Find All` logs all matches to the Find "
		 "channel in the Console. `Replace All` snapshots occurrences before replacing to "
		 "prevent newly introduced matches from being caught.\n\n"
		 "`Context menu`\n\n"
		 "Right-click a word to open a context menu. If the word resolves to a known "
		 "symbol, `Go to Definition` jumps to the file and line where it is defined."},

		{SEC_EDITOR, "AngelScript Features",
		 "The AngelScript editor provides full auto-complete and hover documentation for "
		 "the Retrodev export API:\n\n"
		 "  `IBitmapContext`    Bitmap conversion data and palette.\n"
		 "  `ITilesetContext`   Extracted tiles and palette.\n"
		 "  `ISpriteContext`    Extracted sprites and palette.\n"
		 "  `IMapContext`       Map layers and cell data.\n"
		 "  `IPaletteContext`   Solved palette result.\n\n"
		 "All their methods -- `GetPixelColor`, `GetTile`, `GetCell`, `GetPalette`, `PenGetIndex`, "
		 "`GetParam`, `GetTargetSystem`, `GetTargetMode`, etc. -- appear in the autocomplete "
		 "popup as you type and show signatures on hover.\n\n"
		 "Format menu:\n\n"
		 "  `Tabbify`    Convert leading spaces to tabs (4 spaces = 1 tab).\n"
		 "  `Untabify`   Convert leading tabs to spaces. Trailing whitespace is stripped.\n\n"
		 "Options menu:\n\n"
		 "  `Line Numbers`, `Timing`, `Timing Type`, `Bytecode`, `Palette` (colour theme).\n"
		 "  Available themes: `Dark`, `Light`, `Mariana`, `RetroBlue` (default)."},

		{SEC_EDITOR, "Keyboard Shortcuts",
		 "  `Ctrl+S`         Save\n"
		 "  `Ctrl+Z`         Undo\n"
		 "  `Ctrl+Y`         Redo\n"
		 "  `Ctrl+A`         Select all\n"
		 "  `Ctrl+C`         Copy\n"
		 "  `Ctrl+X`         Cut\n"
		 "  `Ctrl+V`         Paste\n"
		 "  `Ctrl+F`         Find\n"
		 "  `Ctrl+H`         Replace\n"
		 "  `Tab`            Indent selection\n"
		 "  `Shift+Tab`      Unindent selection"},

		//
		// -- Export Scripts -------------------------------------------------------
		//
		{SEC_EXPORTSCRIPTS, "Export Scripts Overview",
		 "Export scripts are AngelScript files that receive fully converted data and write "
		 "it in any binary format. They have full control over layout, headers and "
		 "compression.\n\n"
		 "Scripts are attached to a build item via the Export section of the item's editor. "
		 "Click `Select...` to open the script picker, which shows only scripts whose `@exporter` "
		 "and `@target` tags match the current build item type and target system.\n\n"
		 "Scripts tagged `@exporter util` are helper includes -- they contain shared functions "
		 "and are never shown in the picker. Use a standard `#include` directive to pull them "
		 "in from another script.\n\n"
		 "The SDK ships ready-to-use export scripts under `sdk/amstrad.cpc/exporters/`."},

		{SEC_EXPORTSCRIPTS, "Script Anatomy & Tags",
		 "Every script begins with a metadata block of // @tag value comment lines:\n\n"
		 "  `// @description`  One-line description shown in the picker.\n"
		 "  `// @exporter`     `bitmap` | `tiles` | `sprites` | `map` | `util`\n"
		 "  `// @target`       `amstrad.cpc`  (omit for system-agnostic scripts)\n"
		 "  `// @param`        key  type  default  label\n\n"
		 "`@param` types and their UI controls:\n\n"
		 "  `bool`    Checkbox      true or false\n"
		 "  `int`     Spinner       Any decimal integer\n"
		 "  `string`  Text field    Single word\n"
		 "  `combo`   Drop-down     default|opt2|opt3  (options are pipe-separated)\n\n"
		 "Retrodev builds the parameter UI from these tags automatically. Scripts read "
		 "values at run time via `ctx.GetParam(\"key\")`.\n\n"
		 "Entry point -- the function is always named `Export`:\n\n"
		 "  `void Export(Image@ image, const string& in outputPath, BitmapExportContext@ ctx)`\n\n"
		 "  `void Export(const string& in outputPath, TilesetExportContext@ ctx)`\n\n"
		 "  `void Export(const string& in outputPath, SpriteExportContext@ ctx)`\n\n"
		 "  `void Export(const string& in outputPath, MapExportContext@ ctx)`"},

		{SEC_EXPORTSCRIPTS, "Export API Reference",
		 "Common methods on all context types:\n\n"
		 "  `GetParam(key)`         Value chosen by the user in the Export panel.\n"
		 "  `GetTargetSystem()`     e.g. \"amstrad.cpc\"\n"
		 "  `GetTargetMode()`       e.g. \"Mode 0\", \"Mode 1\"\n\n"
		 "Transparency (bitmap, tileset, sprite contexts):\n\n"
		 "  `GetUseTransparentColor()`  True when transparent colour handling is enabled.\n"
		 "  `GetTransparentPen()`       Pen index for transparent pixels (-1 if none set).\n\n"
		 "Palette API (bitmap, tileset, sprite contexts):\n\n"
		 "  `ctx.GetPalette()`              Returns a `Palette@` handle.\n"
		 "  `palette.PaletteMaxColors()`    Number of active pens.\n"
		 "  `palette.PenGetIndex(rgb)`      Pen index for a pixel colour (-1 if not found).\n"
		 "  `palette.PenGetColorIndex(pen)` Firmware colour index for the pen.\n\n"
		 "`Image@` (bitmap / per-tile / per-sprite):\n\n"
		 "  `image.GetPixelColor(x, y)`     Returns `RgbColor` for a pixel.\n\n"
		 "`RgbColor` value (returned by `GetPixelColor`):\n\n"
		 "  `.r` / `.g` / `.b` / `.a`       Per-channel byte values (0-255).\n"
		 "  `IsTransparent()`               Returns true when alpha == 0.\n"
		 "  `IsOpaque()`                    Returns true when alpha == 255.\n\n"
		 "`BitmapExportContext`:\n"
		 "  `GetNativeWidth()`, `GetNativeHeight()`, `GetPalette()`,\n"
		 "  `GetUseTransparentColor()`, `GetTransparentPen()`\n\n"
		 "`TilesetExportContext`:\n"
		 "  `GetTileCount()`, `GetTileWidth()`, `GetTileHeight()`, `GetTile(idx)`, `GetPalette()`,\n"
		 "  `GetUseTransparentColor()`, `GetTransparentPen()`\n\n"
		 "`SpriteExportContext`:\n"
		 "  `GetSpriteCount()`, `GetSpriteWidth(idx)`, `GetSpriteHeight(idx)`,\n"
		 "  `GetSpriteName(idx)`, `GetSprite(idx)`, `GetPalette()`,\n"
		 "  `GetUseTransparentColor()`, `GetTransparentPen()`\n\n"
		 "`MapExportContext`:\n"
		 "  `GetLayerCount()`, `GetLayerWidth(i)`, `GetLayerHeight(i)`, `GetLayerName(i)`,\n"
		 "  `GetCell(layer, col, row)`"},

		{SEC_EXPORTSCRIPTS, "Logging & Includes",
		 "Scripts report progress via built-in log functions that write to the Script "
		 "channel in the Console panel:\n\n"
		 "  `Log_Info(\"Export started - \" + tileCount + \" tiles\");`\n"
		 "  `Log_Warning(\"No matching pen - using pen 0.\");`\n"
		 "  `Log_Error(\"Could not open: \" + outputPath);`\n\n"
		 "Including shared scripts:\n\n"
		 "  `#include \"cpc.utils.as\"`\n\n"
		 "The path is resolved relative to the including script. For SDK scripts no "
		 "additional setup is needed -- just add the `#include` and the functions it "
		 "provides become available immediately.\n\n"
		 "SDK utility: `cpc.utils.as` provides `EncodePixels` which appends encoded bytes for "
		 "a horizontal run of pen indices directly to an output buffer, and `EncodeByte` for "
		 "arbitrary byte layouts."},

		//
		// -- Emulators ------------------------------------------------------------
		//
		{SEC_EMULATORS, "Emulator Launch",
		 "Retrodev can launch external emulators directly after a successful build. "
		 "Configure the emulator in the Debug tab of the Build item.\n\n"
		 "Supported emulators:\n\n"
		 "  `WinAPE`    Amstrad CPC emulator.\n"
		 "  `RVM`       Cross-platform CPC emulator.\n"
		 "  `ACE-DL`    Advanced CPC emulator with integrated debugger.\n\n"
		 "The executable path is stored in application settings and shared across all "
		 "projects. All other options (media, command, flags) are stored per project.\n\n"
		 "Press `F5` or click `Debug` to build and launch in one step."},
	};
	static_assert(sizeof(s_topics) / sizeof(s_topics[0]) == TOPIC_COUNT, "s_topics array size does not match TOPIC_COUNT -- update the array or the enum");

	//
	// Per-section: first topic index and count
	//
	static const int s_sectionStart[SEC_COUNT] = {
		TOPIC_WELCOME, TOPIC_BITMAPS, TOPIC_EDITOR, TOPIC_EXPORTSCRIPTS, TOPIC_EMULATORS,
	};
	static const int s_sectionCount[SEC_COUNT] = {
		TOPIC_BITMAPS - TOPIC_WELCOME, TOPIC_EDITOR - TOPIC_BITMAPS, TOPIC_EXPORTSCRIPTS - TOPIC_EDITOR, TOPIC_EMULATORS - TOPIC_EXPORTSCRIPTS, TOPIC_COUNT - TOPIC_EMULATORS,
	};

	//
	// Cross-reference links shown at the bottom of each topic
	//
	struct TopicLink {
		int fromTopic;
		int toTopic;
		const char* label;
	};
	static const TopicLink s_links[] = {
		//
		// Getting Started
		//
		{TOPIC_WELCOME, TOPIC_PROJECTS, "Creating and opening projects"},
		{TOPIC_WELCOME, TOPIC_PROJECT_LAYOUT, "Project panel layout"},
		{TOPIC_PROJECTS, TOPIC_PROJECT_LAYOUT, "Project panel layout"},
		{TOPIC_PROJECTS, TOPIC_FILES_PANEL, "Files panel"},
		{TOPIC_PROJECT_LAYOUT, TOPIC_FILES_PANEL, "Files panel"},
		{TOPIC_PROJECT_LAYOUT, TOPIC_BITMAPS, "Bitmap conversion"},
		{TOPIC_FILES_PANEL, TOPIC_BITMAPS, "Bitmap conversion"},
		{TOPIC_FILES_PANEL, TOPIC_TILES, "Tile extraction"},
		{TOPIC_FILES_PANEL, TOPIC_SPRITES, "Sprite extraction"},
		//
		// Bitmaps
		//
		{TOPIC_BITMAPS, TOPIC_BITMAP_PIPELINE, "Conversion pipeline"},
		{TOPIC_BITMAPS, TOPIC_BITMAP_PALETTE, "Palette controls"},
		{TOPIC_BITMAPS, TOPIC_BITMAP_EXPORT, "Bitmap export"},
		{TOPIC_BITMAP_PIPELINE, TOPIC_BITMAP_PARAMS, "Quantization & dithering parameters"},
		{TOPIC_BITMAP_PIPELINE, TOPIC_BITMAP_RESIZE, "Resize & colour correction"},
		{TOPIC_BITMAP_PIPELINE, TOPIC_BITMAP_PALETTE, "Palette controls"},
		{TOPIC_BITMAP_PARAMS, TOPIC_BITMAP_QUANTIZE, "Quantization & dithering"},
		{TOPIC_BITMAP_PARAMS, TOPIC_BITMAP_RESIZE, "Resize & colour correction"},
		{TOPIC_BITMAP_PALETTE, TOPIC_PALETTE, "Palette Solver"},
		{TOPIC_BITMAP_EXPORT, TOPIC_EXPORTSCRIPTS, "Export scripts overview"},
		{TOPIC_BITMAP_EXPORT, TOPIC_EXPORTSCRIPTS_API, "Export API reference"},
		//
		// Tiles
		//
		{TOPIC_TILES, TOPIC_BITMAPS, "Bitmap conversion (shared Conversion tab)"},
		{TOPIC_TILES, TOPIC_TILE_EXTRACTION, "Tile extraction parameters"},
		{TOPIC_TILES, TOPIC_TILE_EXPORT, "Tile export"},
		{TOPIC_TILE_EXTRACTION, TOPIC_TILE_EXPORT, "Tile export API"},
		{TOPIC_TILE_EXTRACTION, TOPIC_MAPS, "Using tiles in a map"},
		{TOPIC_TILE_EXPORT, TOPIC_EXPORTSCRIPTS, "Export scripts overview"},
		{TOPIC_TILE_EXPORT, TOPIC_EXPORTSCRIPTS_API, "Export API reference"},
		//
		// Sprites
		//
		{TOPIC_SPRITES, TOPIC_BITMAPS, "Bitmap conversion (shared Conversion tab)"},
		{TOPIC_SPRITES, TOPIC_SPRITE_EXTRACTION, "Defining & editing sprites"},
		{TOPIC_SPRITES, TOPIC_SPRITE_EXPORT, "Sprite export"},
		{TOPIC_SPRITE_EXTRACTION, TOPIC_SPRITE_EXPORT, "Sprite export API"},
		{TOPIC_SPRITE_EXPORT, TOPIC_EXPORTSCRIPTS, "Export scripts overview"},
		{TOPIC_SPRITE_EXPORT, TOPIC_EXPORTSCRIPTS_API, "Export API reference"},
		//
		// Maps
		//
		{TOPIC_MAPS, TOPIC_MAP_CONCEPTS, "Map concepts: layers, slots, groups"},
		{TOPIC_MAPS, TOPIC_MAP_PAINTING, "Painting & scrollbars"},
		{TOPIC_MAPS, TOPIC_MAP_EXPORT, "Map export"},
		{TOPIC_MAPS, TOPIC_TILES, "Tile extraction (tileset source)"},
		{TOPIC_MAP_CONCEPTS, TOPIC_MAPS, "Map editor overview"},
		{TOPIC_MAP_CONCEPTS, TOPIC_MAP_PAINTING, "Painting & scrollbars"},
		{TOPIC_MAP_PAINTING, TOPIC_MAP_EXPORT, "Map export API"},
		{TOPIC_MAP_EXPORT, TOPIC_EXPORTSCRIPTS, "Export scripts overview"},
		{TOPIC_MAP_EXPORT, TOPIC_EXPORTSCRIPTS_API, "Export API reference"},
		//
		// Palette
		//
		{TOPIC_PALETTE, TOPIC_PALETTE_ZONES, "Zones & solving"},
		{TOPIC_PALETTE, TOPIC_BITMAPS, "Bitmap conversion"},
		{TOPIC_PALETTE_ZONES, TOPIC_PALETTE, "Palette solver overview"},
		//
		// Build
		//
		{TOPIC_BUILD, TOPIC_BUILD_SOURCE, "Sources & includes"},
		{TOPIC_BUILD, TOPIC_BUILD_OUTPUT, "Output & RASM options"},
		{TOPIC_BUILD, TOPIC_BUILD_DEBUG, "Debug & emulator"},
		{TOPIC_BUILD, TOPIC_EMULATORS, "Emulator launch"},
		{TOPIC_BUILD_SOURCE, TOPIC_BUILD, "Build pipeline overview"},
		{TOPIC_BUILD_OUTPUT, TOPIC_BUILD_DEBUG, "Debug tab"},
		{TOPIC_BUILD_DEBUG, TOPIC_EMULATORS, "Emulator launch"},
		//
		// Editor
		//
		{TOPIC_EDITOR, TOPIC_EDITOR_Z80, "Z80 assembly features"},
		{TOPIC_EDITOR, TOPIC_EDITOR_AS, "AngelScript features"},
		{TOPIC_EDITOR, TOPIC_EDITOR_SHORTCUTS, "Keyboard shortcuts"},
		{TOPIC_EDITOR_Z80, TOPIC_EDITOR_AS, "AngelScript features"},
		{TOPIC_EDITOR_AS, TOPIC_EXPORTSCRIPTS, "Export scripts overview"},
		{TOPIC_EDITOR_AS, TOPIC_EXPORTSCRIPTS_API, "Export API reference"},
		//
		// Export Scripts
		//
		{TOPIC_EXPORTSCRIPTS, TOPIC_EXPORTSCRIPTS_ANATOMY, "Script anatomy & tags"},
		{TOPIC_EXPORTSCRIPTS, TOPIC_EXPORTSCRIPTS_API, "Export API reference"},
		{TOPIC_EXPORTSCRIPTS, TOPIC_EXPORTSCRIPTS_LOG, "Logging & includes"},
		{TOPIC_EXPORTSCRIPTS_ANATOMY, TOPIC_EXPORTSCRIPTS_API, "Export API reference"},
		{TOPIC_EXPORTSCRIPTS_ANATOMY, TOPIC_EXPORTSCRIPTS_LOG, "Logging & includes"},
		{TOPIC_EXPORTSCRIPTS_API, TOPIC_BITMAP_EXPORT, "Bitmap export"},
		{TOPIC_EXPORTSCRIPTS_API, TOPIC_TILE_EXPORT, "Tile export"},
		{TOPIC_EXPORTSCRIPTS_API, TOPIC_SPRITE_EXPORT, "Sprite export"},
		{TOPIC_EXPORTSCRIPTS_API, TOPIC_MAP_EXPORT, "Map export"},
		//
		// Emulators
		//
		{TOPIC_EMULATORS, TOPIC_BUILD_DEBUG, "Build debug tab & emulator options"},
	};
	static const int s_linkCount = (int)(sizeof(s_links) / sizeof(s_links[0]));

	//
	// Section open/close state for the treeview navigator
	//
	static bool s_sectionOpen[SEC_COUNT] = {true, false, false, false, false};

	//
	// -------------------------------------------------------------------------
	//
	void HelpDialog::Show() {
		m_open = true;
	}

	//
	// Render a single logical line of text with inline backtick emphasis support.
	// The line is rendered word-wrapped at the available panel width. Backtick-wrapped
	// spans are drawn in bright white; plain text is drawn in a dimmed colour.
	// Uses ImDrawList directly so the full line wraps as a unit rather than as
	// disconnected SameLine fragments that cannot wrap past the first segment.
	//
	static void RenderLineSpans(const char* lineStart, const char* lineEnd, float indentPx) {
		static const ImU32 kDim = IM_COL32(140, 140, 140, 255);
		static const ImU32 kBright = IM_COL32(255, 255, 255, 255);
		ImFont* font = ImGui::GetFont();
		float fontSize = ImGui::GetFontSize();
		float wrapWidth = ImGui::GetContentRegionAvail().x - indentPx;
		if (wrapWidth < 1.0f)
			wrapWidth = 1.0f;
		//
		// Walk spans, building a flat token list: {text ptr, len, color}
		//
		struct Span {
			const char* text;
			int len;
			ImU32 col;
		};
		ImVector<Span> spans;
		const char* p = lineStart;
		while (p < lineEnd) {
			const char* tick = p;
			while (tick < lineEnd && *tick != '`')
				tick++;
			if (tick > p)
				spans.push_back({p, (int)(tick - p), kDim});
			if (tick >= lineEnd)
				break;
			const char* sStart = tick + 1;
			const char* sEnd = sStart;
			while (sEnd < lineEnd && *sEnd != '`')
				sEnd++;
			if (sEnd > sStart)
				spans.push_back({sStart, (int)(sEnd - sStart), kBright});
			p = (sEnd < lineEnd) ? sEnd + 1 : lineEnd;
		}
		if (spans.empty())
			return;
		//
		// Lay out spans word-wrapped across lines, drawing each glyph run
		// directly onto the window draw list at the correct screen position.
		//
		ImDrawList* dl = ImGui::GetWindowDrawList();
		ImVec2 cursor = ImGui::GetCursorScreenPos();
		float lineH = ImGui::GetTextLineHeightWithSpacing();
		float startX = cursor.x;
		float x = startX;
		float y = cursor.y;
		float maxX = startX + wrapWidth;
		for (int si = 0; si < spans.Size; si++) {
			const Span& sp = spans[si];
			const char* wp = sp.text;
			const char* wend = sp.text + sp.len;
			while (wp < wend) {
				//
				// Find end of next word (break at spaces)
				//
				const char* wordEnd = wp;
				while (wordEnd < wend && *wordEnd != ' ')
					wordEnd++;
				if (wordEnd < wend)
					wordEnd++;
				float wordW = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, wp, wordEnd).x;
				//
				// Wrap to next line if word doesn't fit (unless at line start)
				//
				if (x + wordW > maxX && x > startX) {
					x = startX;
					y += lineH;
				}
				dl->AddText(font, fontSize, ImVec2(x, y), sp.col, wp, wordEnd);
				x += wordW;
				wp = wordEnd;
			}
		}
		//
		// Advance ImGui cursor past the rendered block so layout continues correctly
		//
		float totalH = (y - cursor.y) + lineH;
		ImGui::Dummy(ImVec2(wrapWidth, totalH));
	}

	//
	// Render topic body text with proper wrapping, hanging-indent and inline keyword emphasis.
	// Non-indented lines wrap at the available width. Indented lines get a real ImGui indent
	// and also wrap. Backtick-wrapped spans inside any line are rendered in bright white while
	// surrounding text is slightly dimmed.
	//
	void HelpDialog::RenderContent(const char* text) {
		const float charW = ImGui::CalcTextSize(" ").x;
		const char* p = text;
		while (*p) {
			//
			// Find end of current line
			//
			const char* lineEnd = p;
			while (*lineEnd && *lineEnd != '\n')
				lineEnd++;
			//
			// Empty line: add vertical spacing
			//
			if (lineEnd == p) {
				ImGui::Spacing();
				p = (*lineEnd == '\n') ? lineEnd + 1 : lineEnd;
				continue;
			}
			//
			// Count leading spaces to determine indent level
			//
			int leadSpaces = 0;
			const char* contentStart = p;
			while (contentStart < lineEnd && *contentStart == ' ') {
				leadSpaces++;
				contentStart++;
			}
			//
			// Check whether this line contains any backtick emphasis spans.
			// Lines with no backticks use the simpler TextUnformatted path.
			//
			bool hasEmphasis = false;
			for (const char* s = contentStart; s < lineEnd; s++) {
				if (*s == '`') {
					hasEmphasis = true;
					break;
				}
			}
			if (leadSpaces == 0) {
				//
				// Non-indented line: wrap normally at the available width
				//
				if (hasEmphasis) {
					RenderLineSpans(contentStart, lineEnd, 0.0f);
				} else {
					ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(140, 140, 140, 255));
					ImGui::PushTextWrapPos(0.0f);
					ImGui::TextUnformatted(p, lineEnd);
					ImGui::PopTextWrapPos();
					ImGui::PopStyleColor();
				}
			} else {
				//
				// Indented line: push a real indent and wrap at the available width
				//
				float indentPx = leadSpaces * charW;
				ImGui::Indent(indentPx);
				if (hasEmphasis)
					RenderLineSpans(contentStart, lineEnd, indentPx);
				else {
					ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(140, 140, 140, 255));
					ImGui::PushTextWrapPos(0.0f);
					ImGui::TextUnformatted(contentStart, lineEnd);
					ImGui::PopTextWrapPos();
					ImGui::PopStyleColor();
				}
				ImGui::Unindent(indentPx);
			}
			p = (*lineEnd == '\n') ? lineEnd + 1 : lineEnd;
		}
	}

	//
	// Render a clickable cross-reference link that sets the selected topic
	//
	void HelpDialog::RenderLink(const char* label, int topicIndex) {
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(80, 200, 120, 255));
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32(80, 200, 120, 40));
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, IM_COL32(80, 200, 120, 60));
		ImGui::PushID(topicIndex);
		//
		// Render as a small selectable with no background so it reads as inline text
		//
		std::string linkLabel = std::string(ICON_CHEVRON_RIGHT " ") + label + "##link";
		if (ImGui::Selectable(linkLabel.c_str(), false, ImGuiSelectableFlags_None, ImVec2(0.0f, 0.0f))) {
			m_selectedTopic = topicIndex;
			m_scrollToTop = true;
			//
			// Ensure the section containing the target topic is open in the treeview
			//
			for (int sec = 0; sec < SEC_COUNT; sec++) {
				if (s_topics[topicIndex].section == sec) {
					s_sectionOpen[sec] = true;
					break;
				}
			}
		}
		ImGui::PopID();
		ImGui::PopStyleColor(3);
	}

	//
	// Render the Help modal; must be called every frame
	//
	void HelpDialog::Render() {
		//
		// Open the popup on the frame after Show() was called
		//
		if (m_open) {
			ImGui::OpenPopup("Retrodev Documentation");
			m_open = false;
		}
		//
		// Lazy-load logo from embedded resource on first render attempt
		//
		if (!m_logoLoaded) {
			m_logoLoaded = true;
			const Resource& res = Resources::GetResource("gui.res.images.doc-logo.png");
			if (res._ptr != nullptr) {
				SDL_IOStream* io = SDL_IOFromConstMem(res._ptr, (int)res._size);
				if (io)
					m_logoTexture = IMG_LoadTexture_IO(Application::GetRenderer(), io, true);
			}
		}
		//
		// Center the popup on the main viewport
		//
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(1440.0f, 860.0f), ImGuiCond_Appearing);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 10.0f));
		bool modalOpen = ImGui::BeginPopupModal("Retrodev Documentation", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
		ImGui::PopStyleVar();
		if (!modalOpen)
			return;
		//
		// Reserve vertical space for the Close button row at the bottom
		//
		float buttonRowHeight = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;
		float panelHeight = ImGui::GetContentRegionAvail().y - buttonRowHeight;
		//
		// Left panel: treeview navigator
		//
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 4.0f));
		if (ImGui::BeginChild("##topics", ImVec2(240.0f, panelHeight), ImGuiChildFlags_Borders)) {
			ImGui::PopStyleVar();
			//
			// Logo at the top of the navigator panel
			//
			if (m_logoTexture) {
				float panelW = ImGui::GetContentRegionAvail().x;
				float texW = 1.0f, texH = 1.0f;
				SDL_GetTextureSize(m_logoTexture, &texW, &texH);
				float logoH = panelW * (texH / texW);
				if (logoH > 80.0f)
					logoH = 80.0f;
				ImGui::Image(ImTextureRef(m_logoTexture), ImVec2(panelW, logoH));
				ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), IM_COL32(0, 150, 0, 200), 4.0f);
				ImGui::Spacing();
			}
			//
			// Treeview: one collapsible node per section, topics as leaf selectables inside
			//
			for (int sec = 0; sec < SEC_COUNT; sec++) {
				ImGui::SetNextItemOpen(s_sectionOpen[sec], ImGuiCond_Always);
				ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_SpanAvailWidth;
				bool sectionExpanded = ImGui::TreeNodeEx(s_sectionNames[sec], nodeFlags);
				s_sectionOpen[sec] = sectionExpanded;
				if (sectionExpanded) {
					int first = s_sectionStart[sec];
					int count = s_sectionCount[sec];
					for (int ti = first; ti < first + count; ti++) {
						ImGui::Indent(8.0f);
						bool selected = (m_selectedTopic == ti);
						char selId[128];
						snprintf(selId, sizeof(selId), "%s##ti%d", s_topics[ti].title, ti);
						if (ImGui::Selectable(selId, selected, ImGuiSelectableFlags_None, ImVec2(0.0f, 0.0f))) {
							m_selectedTopic = ti;
							m_scrollToTop = true;
						}
						ImGui::Unindent(8.0f);
					}
					ImGui::TreePop();
				}
			}
		} else {
			ImGui::PopStyleVar();
		}
		ImGui::EndChild();
		ImGui::SameLine();
		//
		// Right panel: outer bordered container (non-scrolling)
		// The body scrolls inside it; the "See also" footer is pinned at the bottom.
		//
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 8.0f));
		if (ImGui::BeginChild("##content", ImVec2(0.0f, panelHeight), ImGuiChildFlags_Borders)) {
			ImGui::PopStyleVar();
			//
			// Pre-count links for this topic so the footer height can be computed
			//
			int linkCount = 0;
			for (int li = 0; li < s_linkCount; li++) {
				if (s_links[li].fromTopic == m_selectedTopic)
					linkCount++;
			}
			//
			// Footer height: separator + "See also:" label + one selectable row per link
			// Add spacing and separator only when there are links
			//
			float footerH = 0.0f;
			if (linkCount > 0) {
				float lineH = ImGui::GetTextLineHeightWithSpacing();
				float spacingY = ImGui::GetStyle().ItemSpacing.y;
				footerH = spacingY			  // Spacing() before separator
						  + 1.0f			  // Separator line
						  + spacingY		  // Spacing() after separator
						  + lineH			  // "See also:" text
						  + spacingY		  // Spacing() after label
						  + linkCount * lineH // one selectable per link
						  + spacingY;		  // bottom breathing room
			}
			//
			// Scrollable body child: fills the outer container minus the footer strip
			//
			float bodyH = ImGui::GetContentRegionAvail().y - footerH;
			if (bodyH < 1.0f)
				bodyH = 1.0f;
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			if (ImGui::BeginChild("##body", ImVec2(-1.0f, bodyH), ImGuiChildFlags_None)) {
				ImGui::PopStyleVar();
				//
				// Scroll to top when navigating to a new topic via a link
				//
				if (m_scrollToTop) {
					ImGui::SetScrollHereY(0.0f);
					m_scrollToTop = false;
				}
				//
				// Topic title
				//
				ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 220, 0, 255));
				ImGui::Text("%s", s_topics[m_selectedTopic].title);
				ImGui::PopStyleColor();
				ImGui::Separator();
				ImGui::Spacing();
				//
				// Topic body text -- rendered with hanging-indent support
				//
				RenderContent(s_topics[m_selectedTopic].content);
				ImGui::Spacing();
			} else {
				ImGui::PopStyleVar();
			}
			ImGui::EndChild();
			//
			// "See also" footer -- rendered outside the scrollable child so it is
			// always visible regardless of how far the body has been scrolled
			//
			if (linkCount > 0) {
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();
				ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(160, 160, 160, 255));
				ImGui::TextUnformatted("See also:");
				ImGui::PopStyleColor();
				ImGui::Spacing();
				for (int li = 0; li < s_linkCount; li++) {
					if (s_links[li].fromTopic != m_selectedTopic)
						continue;
					RenderLink(s_links[li].label, s_links[li].toTopic);
				}
			}
		} else {
			ImGui::PopStyleVar();
		}
		ImGui::EndChild();
		//
		// Previous / Next navigation -- outside the scrollable child, always at the bottom
		//
		ImGui::BeginDisabled(m_selectedTopic == 0);
		if (ImGui::Button(ICON_CHEVRON_LEFT " Previous")) {
			--m_selectedTopic;
			m_scrollToTop = true;
			s_sectionOpen[s_topics[m_selectedTopic].section] = true;
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::BeginDisabled(m_selectedTopic == TOPIC_COUNT - 1);
		if (ImGui::Button("Next " ICON_CHEVRON_RIGHT)) {
			++m_selectedTopic;
			m_scrollToTop = true;
			s_sectionOpen[s_topics[m_selectedTopic].section] = true;
		}
		ImGui::EndDisabled();
		//
		// Close button -- right-aligned on the same row
		//
		ImGui::SameLine(ImGui::GetWindowSize().x - 120.0f - ImGui::GetStyle().WindowPadding.x);
		if (ImGui::Button("Close", ImVec2(120.0f, 0.0f)))
			ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}

}
