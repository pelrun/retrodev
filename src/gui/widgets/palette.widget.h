// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Palette widget -- hardware colour palette display and selection.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <retrodev.gui.h>
#include <retrodev.lib.h>

namespace RetrodevGui {

	//
	// Instance-based system color picker for palette selection
	// Used by raster editor, sprite picker, etc. - anywhere we need to select from system palette
	// Not static: maintains state across frames for multiple independent picker instances
	//
	class SystemColorPicker {
	public:
		//
		// Render the color picker popup (call each frame)
		// Returns true when a color has been selected (popup closes)
		// outColorIndex: receives the selected color system index (0-based)
		// popupId: unique identifier for ImGui popup
		// title: popup window title
		//
		bool RenderPickerPopup(
			std::shared_ptr<RetrodevLib::IPaletteConverter> palette,
			int& outColorIndex,
			const char* popupId,
			const char* title = "Select Color"
		);
		//
		// Open the color picker popup
		//
		void Open();
		//
		// Check if picker is currently open
		//
		bool IsOpen() const { return m_isOpen; }
		//
		// Cancel the current selection and close picker
		//
		void Cancel() { m_isOpen = false; }

	private:
		bool m_isOpen = false;
		bool m_shouldOpenThisFrame = false;  // One-shot flag to call ImGui::OpenPopup
		int m_selectedColorIndex = 0;  // Persistent selection while open
	};

	//
	// Palette widget for displaying and editing palette colors
	//
	class PaletteWidget {
	public:
		//
		// Render the palette widget contents (to be called from within an existing child window)
		// It receives the conversion params struct and a palette converter for accessing palette data
		// Returns true if any parameter was changed, false otherwise
		//
		static bool Render(RetrodevLib::GFXParams* params, std::shared_ptr<RetrodevLib::IPaletteConverter> palette, bool showTransparency = true);
		//
		// Check if currently picking a color from the source image
		//
		static bool IsPickingFromImage() { return m_pickingFromImage; }
		//
		// Set the picked color from the source image
		//
		static void SetPickedColor(int r, int g, int b, RetrodevLib::GFXParams* params);
		//
		// Cancel color picking
		//
		static void CancelColorPicking() { m_pickingFromImage = false; }

	private:
		//
		// Per-pen state
		//
		struct PenState {
			bool disabled;
			bool locked;
		};
		//
		// Collapsible section state
		//
		static bool m_paletteOpen;
		//
		// Pen states (disabled/locked flags)
		//
		static std::vector<PenState> m_penStates;
		//
		// Color picker popup state
		//
		static int m_selectedPen;
		static int m_selectedColorIndex; // Persistent across frames
		static bool m_showColorPicker;
		static bool m_openColorPickerThisFrame;
		//
		// Transparency color picker state
		//
		static bool m_showTransparencyPicker;
		static bool m_pickingFromImage;
		//
		// "All" checkbox states for mass lock/disable operations
		//
		static bool m_allLocked;
		static bool m_allDisabled;
		//
		// Render individual sections
		//
		static bool RenderPaletteColors(RetrodevLib::GFXParams* params, std::shared_ptr<RetrodevLib::IPaletteConverter> palette);
		static bool RenderColorPickerPopup(RetrodevLib::GFXParams* params, std::shared_ptr<RetrodevLib::IPaletteConverter> palette);
		static bool RenderTransparencySection(RetrodevLib::GFXParams* params, std::shared_ptr<RetrodevLib::IPaletteConverter> palette);
		//
		// Helper to ensure pen states match current palette size
		//
		static void EnsurePenStates(int penCount);
	};

}
