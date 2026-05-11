// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Raster effect parameters -- generic system-agnostic configuration for raster code generation.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <export/export.params.h>
#include <string>
#include <vector>

namespace RetrodevLib {
	//
	// Generic raster parameters (system-agnostic).
	// Platform-specific settings (register names, timing modes, counter state, etc.) are stored
	// opaquely in the `config` field and deserialized by the platform module.
	//
	struct RasterParams {
		//
		// Target system name (e.g. "Amstrad CPC")
		//
		std::string targetSystem;
		//
		// Target video mode (e.g. "Mode 0", "Mode 1", "Mode 2")
		//
		std::string targetMode;
		//
		// Target palette type (e.g. "GA Palette", "ASIC Palette")
		//
		std::string targetPaletteType;
		//
		// Export configuration for this build item
		//
		ExportParams exportParams;
		//
		// Opaque serialized command list.
		// Each string is owned and interpreted exclusively by the platform module
		// (e.g. CPCRaster::Load / CPCRaster::Save). The project layer treats them as
		// plain strings and never parses their content.
		//
		std::vector<std::string> commands;
		//
		// Index of the currently selected command, or -1 if none.
		// Persisted so the editor reopens on the last-edited command.
		//
		int selectedCommand = -1;
		//
		// Opaque platform-specific configuration (serialized to JSON).
		// For Amstrad CPC: contains ruptureName, outputAsmPath, generatorMode, CRTC type, counter state.
		// Deserialized and interpreted exclusively by the platform module.
		// The project layer treats this as an opaque string.
		//
		std::string config;
	};
}
