// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Amstrad CPC palette converter -- hardware colour mapping and pen assignment.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#include "cpc.palette.h"
#include <retrodev.lib.h>
#include <algorithm>

namespace RetrodevLib::ConverterAmstradCPC {

	//
	// Static CPC palette definition (27 colors)
	//
	const std::array<RgbColor, 27> CPCPalette::RgbCPC = {
		{RgbColor(CA0, CA0, CA0), RgbColor(CA0, CA0, CA1), RgbColor(CA0, CA0, CA2), RgbColor(CA1, CA0, CA0), RgbColor(CA1, CA0, CA1), RgbColor(CA1, CA0, CA2),
		 RgbColor(CA2, CA0, CA0), RgbColor(CA2, CA0, CA1), RgbColor(CA2, CA0, CA2), RgbColor(CA0, CA1, CA0), RgbColor(CA0, CA1, CA1), RgbColor(CA0, CA1, CA2),
		 RgbColor(CA1, CA1, CA0), RgbColor(CA1, CA1, CA1), RgbColor(CA1, CA1, CA2), RgbColor(CA2, CA1, CA0), RgbColor(CA2, CA1, CA1), RgbColor(CA2, CA1, CA2),
		 RgbColor(CA0, CA2, CA0), RgbColor(CA0, CA2, CA1), RgbColor(CA0, CA2, CA2), RgbColor(CA1, CA2, CA0), RgbColor(CA1, CA2, CA1), RgbColor(CA1, CA2, CA2),
		 RgbColor(CA2, CA2, CA0), RgbColor(CA2, CA2, CA1), RgbColor(CA2, CA2, CA2)}};

	//
	// Color selection modes
	//
	const std::array<std::string, 3> CPCPalette::ColorSelectionModesList = {
		{CPCColorSelectionModes::RGBClamping, CPCColorSelectionModes::EuclideanPerceptual, CPCColorSelectionModes::EuclideanDistance}};

	//
	// Constructor: Initialize CPC palette with default settings
	// Defaults: GA Palette (27 colors), Mode 0 (16 colors), Perceptual color matching
	//
	CPCPalette::CPCPalette() : m_paletteType(CPCPaletteTypes::Hardware), m_mode(CPCModes::Mode0), m_colorSelectionMode(CPCColorSelectionModes::EuclideanPerceptual) {
		m_palette.fill(0);
		m_paletteUsed.fill(false);
		//
		// Initialize default vectors sized for the CPC maximum (17 pens) and point to them
		//
		m_defaultLocked.assign(17, false);
		m_defaultEnabled.assign(17, true);
		m_paletteLocked = &m_defaultLocked;
		m_paletteEnabled = &m_defaultEnabled;
		m_syncedFromParams = false;
	}

	//
	// Set the palette type (GA Palette: 27 colors, ASIC Palette: 4096 colors)
	// Clears unlocked pens to black when type changes
	// PRESERVES locked pen colors (but clamps indices to new palette range)
	//
	void CPCPalette::SetPaletteType(const std::string& paletteType) {
		m_paletteType = paletteType;
		int maxColors = GetSystemMaxColors();
		//
		// Clear only unlocked pens, and clamp locked pen indices to valid range
		// Iterate only the active pen range so we never read past the lock/enable vectors
		//
		int penCount = PaletteMaxColors();
		for (int i = 0; i < penCount; i++) {
			if ((*m_paletteLocked)[i]) {
				//
				// Locked pen: clamp color index to new palette's valid range
				// This prevents crashes when switching from Plus (4096) to Hardware (27)
				//
				if (m_palette[i] >= maxColors) {
					m_palette[i] = 0; // Reset to black if out of range
				}
			} else {
				//
				// Unlocked pen: clear to black and mark unused
				//
				m_palette[i] = 0;
				m_paletteUsed[i] = false;
			}
		}
	}

	//
	// Returns the list of supported color selection modes
	// Available modes: RGB Clamping, Euclidean Perceptual, Euclidean Balanced
	//
	std::vector<std::string> CPCPalette::GetColorSelectionModes() {
		return std::vector<std::string>(ColorSelectionModesList.begin(), ColorSelectionModesList.end());
	}

	//
	// Returns the maximum number of colors in the system palette
	// GA Palette: 27 colors (3x3x3 RGB cube)
	// ASIC Palette: 4096 colors (12-bit RGB)
	//
	int CPCPalette::GetSystemMaxColors() {
		if (m_paletteType == CPCPaletteTypes::Plus)
			return 4096;
		else if (m_paletteType == CPCPaletteTypes::Hardware)
			return 27;
		return 27;
	}

	//
	// Returns the RGB value for the given index in the system palette
	// index: system palette index (0-26 for Hardware, 0-4095 for Plus)
	// Returns: RGB color (black if index out of range)
	//
	RgbColor CPCPalette::GetSystemColorByIndex(int index) {
		//
		// Clamp index to valid range
		//
		if (index < 0 || index >= GetSystemMaxColors())
			return RgbColor(0, 0, 0);
		if (m_paletteType == CPCPaletteTypes::Hardware) {
			return RgbCPC[index];
		}
		if (m_paletteType == CPCPaletteTypes::Plus) {
			//
			// Index will be in range of 0..4096
			// Extract RGB from 12-bit index: 0000GGGGBBBBRRR
			//
			uint8_t g = static_cast<uint8_t>((index & 0x0F00) >> 4);
			uint8_t b = static_cast<uint8_t>(index & 0x00F0);
			uint8_t r = static_cast<uint8_t>((index & 0x000F) << 4);
			return RgbColor(r, g, b);
		}
		//
		// Return black for unsupported palette type
		//
		return RgbColor(0, 0, 0);
	}

	//
	// Returns an index to the complete system palette using the given RGB values
	// Uses the selected color matching mode (Euclidean, Perceptual, or RGB Clamping)
	// inputColor: RGB color to match
	// colorSelectionMode: Color selection mode string (empty = use default)
	// Returns: index to the system palette (0-26 for Hardware, 0-4095 for Plus)
	//
	int CPCPalette::GetSystemIndexByColor(const RgbColor& inputColor, const std::string& colorSelectionMode) {
		int selectedIndex = 0;
		//
		// Store the color selection mode for use in ColorDistance
		//
		if (!colorSelectionMode.empty()) {
			m_colorSelectionMode = colorSelectionMode;
		}
		//
		// CPC Maps into 27 color fixed palette
		//
		if (m_paletteType == CPCPaletteTypes::Hardware) {
			//
			// Euclidean distance (Balanced) - true Euclidean with equal RGB weights
			// Better for frequency-based reduction
			//
			if (colorSelectionMode == CPCColorSelectionModes::EuclideanDistance) {
				int minDistance = 0x7FFFFFFF;
				for (int colorIndex = 0; colorIndex < 27; colorIndex++) {
					const RgbColor& systemColor = RgbCPC[colorIndex];
					//
					// Calculate color distance with equal weights (33% each channel)
					//
					int redDiff = systemColor.r - inputColor.r;
					int greenDiff = systemColor.g - inputColor.g;
					int blueDiff = systemColor.b - inputColor.b;
					int distance = redDiff * redDiff * coefR_true + greenDiff * greenDiff * coefG_true + blueDiff * blueDiff * coefB_true;
					if (distance < minDistance) {
						minDistance = distance;
						selectedIndex = colorIndex;
						//
						// Early exit if exact match found
						//
						if (distance == 0)
							break;
					}
				}
				return selectedIndex;
			}
			//
			// Euclidean distance (Perceptual) - weighted for human color perception
			// Better for distance-based reduction and avoiding color shifts
			//
			if (colorSelectionMode == CPCColorSelectionModes::EuclideanPerceptual || colorSelectionMode.empty()) {
				int minDistance = 0x7FFFFFFF;
				for (int colorIndex = 0; colorIndex < 27; colorIndex++) {
					const RgbColor& systemColor = RgbCPC[colorIndex];
					//
					// Calculate color distance with perceptual weights (balanced for human vision)
					//
					int redDiff = systemColor.r - inputColor.r;
					int greenDiff = systemColor.g - inputColor.g;
					int blueDiff = systemColor.b - inputColor.b;
					int distance = redDiff * redDiff * coefR_perceptual + greenDiff * greenDiff * coefG_perceptual + blueDiff * blueDiff * coefB_perceptual;
					if (distance < minDistance) {
						minDistance = distance;
						selectedIndex = colorIndex;
						//
						// Early exit if exact match found
						//
						if (distance == 0)
							break;
					}
				}
				return selectedIndex;
			}
			//
			// Direct RGB conversion by values (hardware-specific 3x3x3 cube)
			// Maps RGB values directly to CPC's discrete color space (0, 96, 255)
			//
			if (colorSelectionMode == CPCColorSelectionModes::RGBClamping) {
				//
				// Calculate index based on RGB thresholds (85, 170)
				// Red component: 0-85 -> 0, 86-170 -> 3, 171-255 -> 6
				// Blue component: 0-85 -> 0, 86-170 -> 1, 171-255 -> 2
				// Green component: 0-85 -> 0, 86-170 -> 9, 171-255 -> 18
				//
				selectedIndex = (inputColor.r > cstR2	? 6
								 : inputColor.r > cstR1 ? 3
														: 0) +
								(inputColor.b > cstB2	? 2
								 : inputColor.b > cstB1 ? 1
														: 0) +
								(inputColor.g > cstG2	? 18
								 : inputColor.g > cstG1 ? 9
														: 0);
			}
		}
		//
		// CPC+ Maps into 4096 RGB values (12-bit color: 4 bits per channel)
		//
		if (m_paletteType == CPCPaletteTypes::Plus) {
			//
			// Extract 4 bits per channel and pack into index
			// Format: 0000GGGGBBBBRRR (12-bit)
			//
			int redIndex = ((inputColor.r >> 4) & 0x000F);
			int greenIndex = ((inputColor.g >> 4) & 0x000F) << 8;
			int blueIndex = ((inputColor.b >> 4) & 0x000F) << 4;
			selectedIndex = redIndex + greenIndex + blueIndex;
		}
		return selectedIndex;
	}

	//
	// Returns the maximum number of colors that can be displayed simultaneously
	// Mode 0: 16 colors
	// Mode 1: 4 colors
	// Mode 2: 2 colors
	//
	int CPCPalette::PaletteMaxColors() {
		if (m_mode == CPCModes::Mode0)
			return 16;
		else if (m_mode == CPCModes::Mode1)
			return 4;
		else if (m_mode == CPCModes::Mode2)
			return 2;
		else
			return 16;
	}

	//
	// Returns the maximum number of colors for a specific scanline
	// For standard CPC modes, this is the same as PaletteMaxColors()
	// line: scanline index (for future EGX support where palette changes per line)
	//
	int CPCPalette::PaletteMaxColorsByLine(int y) {
		UNUSED(y);
		if (m_mode == CPCModes::Mode0)
			return 16;
		else if (m_mode == CPCModes::Mode1)
			return 4;
		else if (m_mode == CPCModes::Mode2)
			return 2;
		else
			return 16;
	}

	//
	// Returns the RGB color pointed by the given palette index (pen)
	// paletteIndex: palette index (0-15 for Mode 0, 0-3 for Mode 1, 0-1 for Mode 2)
	// Returns: RGB color (black if index out of range)
	//
	RgbColor CPCPalette::PenGetColor(int paletteIndex) {
		//
		// Return black for out-of-range index
		//
		if (paletteIndex < 0 || paletteIndex >= static_cast<int>(m_palette.size()))
			return RgbColor(0, 0, 0);
		int colorIndex = m_palette[paletteIndex];
		return GetSystemColorByIndex(colorIndex);
	}

	//
	// Returns the system color index stored at the given palette index (pen)
	// paletteIndex: palette index (0-15 for Mode 0, 0-3 for Mode 1, 0-1 for Mode 2)
	// Returns: system color index (0 if out of range)
	//
	int CPCPalette::PenGetColorIndex(int paletteIndex) {
		//
		// Return 0 for out-of-range index
		//
		if (paletteIndex < 0 || paletteIndex >= static_cast<int>(m_palette.size()))
			return 0;
		return m_palette[paletteIndex];
	}
	//
	// Returns the pen index whose assigned color matches the given RGB, or -1 if absent.
	// Only searches within the active pen count for the current mode.
	// Disabled pens are skipped: a disabled pen is explicitly excluded from use
	// (e.g. pen 0 reserved as the transparent pen) and must never be returned
	// as a match for a non-transparent pixel, even if its color happens to match.
	//
	int CPCPalette::PenGetIndex(const RgbColor& color) {
		int penCount = PaletteMaxColors();
		for (int pen = 0; pen < penCount; pen++) {
			if (!PenGetEnabled(pen))
				continue;
			RgbColor pc = PenGetColor(pen);
			if (pc.r == color.r && pc.g == color.g && pc.b == color.b)
				return pen;
		}
		return -1;
	}

	//
	// Sets the system color index into the given palette index (pen)
	// This maps a palette slot (pen) to a specific system color
	// paletteIndex: palette index (0-15 for Mode 0, 0-3 for Mode 1, 0-1 for Mode 2)
	// colorIndex: system color index (0-26 for Hardware, 0-4095 for Plus)
	//
	void CPCPalette::PenSetColorIndex(int paletteIndex, int colorIndex) {
		//
		// Ignore out-of-range writes
		//
		if (paletteIndex < 0 || paletteIndex >= static_cast<int>(m_palette.size()))
			return;
		m_palette[paletteIndex] = colorIndex;
		m_paletteUsed[paletteIndex] = true;
	}

	//
	// Lock or unlock a palette slot
	// Locked pens can be used during rendering but their color cannot be changed by quantization
	// paletteIndex: palette index (0-15 for Mode 0, 0-3 for Mode 1, 0-1 for Mode 2)
	// lock: true to lock, false to unlock
	//
	void CPCPalette::PenLock(int paletteIndex, bool lock) {
		//
		// Ignore out-of-range writes
		//
		if (paletteIndex < 0 || paletteIndex >= (int)m_paletteLocked->size())
			return;
		(*m_paletteLocked)[paletteIndex] = lock;
	}

	//
	// Enable or disable a palette slot
	// Disabled pens cannot be used at all (not selectable during quantization)
	// paletteIndex: palette index (0-15 for Mode 0, 0-3 for Mode 1, 0-1 for Mode 2)
	// enable: true to enable, false to disable
	//
	void CPCPalette::PenEnable(int paletteIndex, bool enable) {
		//
		// Ignore out-of-range writes
		//
		if (paletteIndex < 0 || paletteIndex >= (int)m_paletteEnabled->size())
			return;
		(*m_paletteEnabled)[paletteIndex] = enable;
	}

	//
	// Returns whether a palette slot is locked
	// paletteIndex: palette index (0-15 for Mode 0, 0-3 for Mode 1, 0-1 for Mode 2)
	// Returns: true if locked, false otherwise
	//
	bool CPCPalette::PenGetLock(int paletteIndex) {
		//
		// Return false for out-of-range index
		//
		if (paletteIndex < 0 || paletteIndex >= (int)m_paletteLocked->size())
			return false;
		return (*m_paletteLocked)[paletteIndex];
	}

	//
	// Returns whether a palette slot is enabled
	// paletteIndex: palette index (0-15 for Mode 0, 0-3 for Mode 1, 0-1 for Mode 2)
	// Returns: true if enabled, false if disabled
	//
	bool CPCPalette::PenGetEnabled(int paletteIndex) {
		//
		// Return true for out-of-range index (default to enabled)
		//
		if (paletteIndex < 0 || paletteIndex >= (int)m_paletteEnabled->size())
			return true;
		return (*m_paletteEnabled)[paletteIndex];
	}

	//
	// Lock or unlock all palette slots at once
	// lock: true to lock all, false to unlock all
	//
	void CPCPalette::PenLockAll(bool lock) {
		int count = PaletteMaxColors();
		for (int i = 0; i < count; i++)
			PenLock(i, lock);
	}

	//
	// Enable or disable all palette slots at once
	// enable: true to enable all, false to disable all
	//
	void CPCPalette::PenEnableAll(bool enable) {
		int count = PaletteMaxColors();
		for (int i = 0; i < count; i++)
			PenEnable(i, enable);
	}

	//
	// Returns the color distance between two colors
	// Uses the same method as color matching to ensure consistency
	// color1: first color
	// color2: second color
	// Returns: distance value (lower = more similar)
	//
	int CPCPalette::ColorDistance(const RgbColor& color1, const RgbColor& color2) {
		//
		// Use the same method that was used during color matching
		// This ensures consistency between histogram building and palette selection
		//
		if (m_colorSelectionMode == CPCColorSelectionModes::EuclideanDistance) {
			//
			// Balanced (true Euclidean) with equal RGB weights
			//
			int redDiff = color1.r - color2.r;
			int greenDiff = color1.g - color2.g;
			int blueDiff = color1.b - color2.b;
			return redDiff * redDiff * coefR_true + greenDiff * greenDiff * coefG_true + blueDiff * blueDiff * coefB_true;
		} else if (m_colorSelectionMode == CPCColorSelectionModes::RGBClamping) {
			//
			// For RGB Clamping, use perceptual distance
			// (RGB Clamping is used for matching, but distance still needs to be calculated)
			//
			int redDiff = color1.r - color2.r;
			int greenDiff = color1.g - color2.g;
			int blueDiff = color1.b - color2.b;
			return redDiff * redDiff * coefR_perceptual + greenDiff * greenDiff * coefG_perceptual + blueDiff * blueDiff * coefB_perceptual;
		} else {
			//
			// Default: Perceptual Euclidean distance
			//
			int redDiff = color1.r - color2.r;
			int greenDiff = color1.g - color2.g;
			int blueDiff = color1.b - color2.b;
			return redDiff * redDiff * coefR_perceptual + greenDiff * greenDiff * coefG_perceptual + blueDiff * blueDiff * coefB_perceptual;
		}
	}

	//
	// Sets the CPC video mode (Mode 0/1/2)
	// This affects the maximum number of displayable colors
	// mode: mode string (from CPCModes constants)
	//
	void CPCPalette::SetCPCMode(const std::string& mode) {
		m_mode = mode;
	}

	//
	// Sets pointers to the lock/enable state arrays from params
	// This makes the palette use the params arrays directly (no copying, no syncing)
	// locked: reference to params PaletteLocked array
	// enabled: reference to params PaletteEnabled array
	//
	void CPCPalette::SetLockEnableArrays(std::vector<bool>& locked, std::vector<bool>& enabled) {
		int penCount = PaletteMaxColors();
		//
		// Write-back any pending PenLock/PenEnable changes to params before re-syncing.
		// Skipped on first call (m_syncedFromParams == false) so the initial project state
		// is read from params rather than overwritten by the default-false/true values.
		// This also prevents dangling pointers: m_paletteLocked always points to
		// m_defaultLocked (a stable member), never into buildTiles which can reallocate.
		//
		if (m_syncedFromParams) {
			locked.assign(m_defaultLocked.begin(), m_defaultLocked.end());
			enabled.assign(m_defaultEnabled.begin(), m_defaultEnabled.end());
		}
		m_defaultLocked = locked;
		m_defaultEnabled = enabled;
		if ((int)m_defaultLocked.size() < penCount)
			m_defaultLocked.resize(penCount, false);
		if ((int)m_defaultEnabled.size() < penCount)
			m_defaultEnabled.resize(penCount, true);
		m_paletteLocked = &m_defaultLocked;
		m_paletteEnabled = &m_defaultEnabled;
		m_syncedFromParams = true;
	}

	//
	// Applies stored pen color indices to locked, enabled pens
	// Skips entries with index -1 (not set) and pens that are unlocked or disabled
	// colors: array of system color indices per pen
	//
	void CPCPalette::ApplyPenColors(std::vector<int>& colors) {
		int maxColors = GetSystemMaxColors();
		int penCount = PaletteMaxColors();
		if ((int)colors.size() < penCount)
			colors.resize(penCount, -1);
		for (int i = 0; i < penCount; i++) {
			if (colors[i] < 0)
				continue;
			if (!(*m_paletteLocked)[i])
				continue;
			if (!(*m_paletteEnabled)[i])
				continue;
			int idx = colors[i];
			if (idx >= maxColors)
				idx = 0;
			m_palette[i] = idx;
			m_paletteUsed[i] = true;
		}
	}
	//
	// Returns whether the pen has been explicitly assigned a color since the last reset
	//
	bool CPCPalette::PenGetUsed(int paletteIndex) {
		if (paletteIndex < 0 || paletteIndex >= static_cast<int>(m_paletteUsed.size()))
			return false;
		return m_paletteUsed[paletteIndex];
	}

}
