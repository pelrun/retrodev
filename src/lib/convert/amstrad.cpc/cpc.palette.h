// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Amstrad CPC palette converter -- hardware colour mapping and pen assignment.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <convert/convert.palette.h>
#include <convert/amstrad.cpc/amstrad.cpc.h>
#include <assets/image/image.color.h>
#include <array>
#include <string>
#include <vector>
#include <algorithm>

namespace RetrodevLib::ConverterAmstradCPC {
	//
	// Color selection mode constants
	//
	namespace CPCColorSelectionModes {
		const std::string EuclideanDistance = "Euclidean Distance (Balanced)";
		const std::string EuclideanPerceptual = "Euclidean Distance (Perceptual)";
		const std::string RGBClamping = "RGB Clamping";
	}
	//
	// Amstrad CPC Palette implementation
	// Supports both Hardware palette (27 colors) and Plus palette (4096 colors)
	//
	class CPCPalette : public IPaletteConverter {
	public:
		//
		// Constructor: Initialize CPC palette with default settings
		//
		CPCPalette();
		~CPCPalette() override = default;
		//
		// IPalette interface implementation
		//
		//
		// Returns the list of supported color selection modes
		//
		std::vector<std::string> GetColorSelectionModes() override;
		//
		// Returns the maximum number of colors in the system palette
		// Hardware: 27, Plus: 4096
		//
		int GetSystemMaxColors() override;
		//
		// Returns the RGB value for the given system palette index
		//
		RgbColor GetSystemColorByIndex(int index) override;
		//
		// Returns the system palette index for the given RGB color
		// Uses the selected color matching mode
		//
		int GetSystemIndexByColor(const RgbColor& color, const std::string& colorSelectionMode) override;
		//
		// Returns the maximum displayable colors for current mode
		// Mode 0: 16, Mode 1: 4, Mode 2: 2
		//
		int PaletteMaxColors() override;
		//
		// Returns the maximum displayable colors for a specific scanline
		// For standard modes, same as PaletteMaxColors()
		//
		int PaletteMaxColorsByLine(int line) override;
		//
		// Returns the RGB color at the given palette index (pen)
		//
		RgbColor PenGetColor(int index) override;
		//
		// Maps a palette slot (pen) to a system color index
		//
		void PenSetColorIndex(int pen, int index) override;
		//
		// Returns the system color index at the given palette slot (pen)
		//
		int PenGetColorIndex(int pen) override;
		//
		// Returns the pen index whose assigned color matches the given RGB color,
		// or -1 if no active pen holds that color.
		//
		int PenGetIndex(const RgbColor& color) override;
		//
		// Lock/unlock a palette slot (locked pens can be used but not changed)
		//
		void PenLock(int pen, bool lock) override;
		//
		// Enable/disable a palette slot (disabled pens cannot be used)
		//
		void PenEnable(int pen, bool enable) override;
		//
		// Returns whether a palette slot is locked
		//
		bool PenGetLock(int pen) override;
		//
		// Returns whether a palette slot is enabled
		//
		bool PenGetEnabled(int pen) override;
		//
		// Returns whether a palette slot has been explicitly assigned a color
		//
		bool PenGetUsed(int pen) override;
		//
		// Calculates color distance using the current matching mode
		//
		int ColorDistance(const RgbColor& c1, const RgbColor& c2) override;
		//
		// Additional methods specific to CPC
		//
		//
		// Sets the CPC video mode (Mode 0/1/2)
		//
		void SetCPCMode(const std::string& mode);
		//
		// Sets the palette type (Hardware or Plus)
		//
		void SetPaletteType(const std::string& paletteType);
		//
		// Sets pointers to the lock/enable state arrays from params
		// This avoids duplicate state - palette uses params arrays directly
		//
		void SetLockEnableArrays(std::vector<bool>& locked, std::vector<bool>& enabled);
		//
		// Applies stored pen color indices from params to locked, enabled pens
		// Must be called after SetPaletteType so the palette range is known
		// colors: vector of system color indices (-1 means not set)
		//
		void ApplyPenColors(std::vector<int>& colors);
		//
		// Lock or unlock all pens at once
		//
		void PenLockAll(bool lock) override;
		//
		// Enable or disable all pens at once
		//
		void PenEnableAll(bool enable) override;

	private:
		//
		// Amstrad CPC color values (3 discrete levels per channel)
		// CA0 = 0x00 (0), CA1 = 0x60 (96), CA2 = 0xFF (255)
		//
		static constexpr int CA0 = 0x00;
		static constexpr int CA1 = 0x60;
		static constexpr int CA2 = 0xFF;
		//
		// True Euclidean distance coefficients (equal RGB weights)
		// Used for balanced color matching (33% per channel)
		// Total: 32768 (1/3 each)
		//
		static constexpr int coefR_true = 10923; // ~33.3%
		static constexpr int coefG_true = 10923; // ~33.3%
		static constexpr int coefB_true = 10922; // ~33.3%
		//
		// Perceptual Euclidean distance coefficients
		// Weighted for human color perception (equal weights work best for CPC's discrete space)
		// Total: 32768
		//
		static constexpr int coefR_perceptual = 10923; // ~33%
		static constexpr int coefG_perceptual = 10923; // ~33%
		static constexpr int coefB_perceptual = 10922; // ~33%
		//
		// RGB clamping thresholds for direct hardware color cube mapping
		// Divides 0-255 range into thirds: 0-85, 86-170, 171-255
		// Maps to CA0 (0), CA1 (96), CA2 (255)
		//
		static constexpr int cstR1 = 85;
		static constexpr int cstR2 = 170;
		static constexpr int cstG1 = 85;
		static constexpr int cstG2 = 170;
		static constexpr int cstB1 = 85;
		static constexpr int cstB2 = 170;
		//
		// The CPC hardware palette (27 colors: 3x3x3 RGB cube)
		// Colors ordered by: R + B*3 + G*9
		//
		static const std::array<RgbColor, 27> RgbCPC;
		//
		// Available color selection modes
		//
		static const std::array<std::string, 3> ColorSelectionModesList;
		//
		// Active palette (maps pen index -> system color index)
		// Size: 17 (max 16 colors + 1 for border in some modes)
		//
		std::array<int, 17> m_palette;
		//
		// Tracks which pens have been explicitly assigned (by quantizer or PenSetColorIndex).
		// Needed because m_palette[i] == 0 is ambiguous: it may be black-assigned or never-touched.
		//
		std::array<bool, 17> m_paletteUsed;
		//
		// Pointers to lock/enable state vectors (owned by params, not copied)
		// This avoids duplicate state and sync issues
		//
		std::vector<bool>* m_paletteLocked;
		std::vector<bool>* m_paletteEnabled;
		//
		// Default vectors used when no params are set
		//
		std::vector<bool> m_defaultLocked;
		std::vector<bool> m_defaultEnabled;
		//
		// True after the first SetLockEnableArrays call; used to decide whether to
		// write-back internal state to params before re-reading (see SetLockEnableArrays).
		//
		bool m_syncedFromParams;
		//
		// Current palette type ("GA Palette" or "ASIC Palette")
		//
		std::string m_paletteType;
		//
		// Current video mode ("Mode 0", "Mode 1", "Mode 2")
		//
		std::string m_mode;
		//
		// Current color selection mode (stored for ColorDistance consistency)
		//
		std::string m_colorSelectionMode;
	};
}
