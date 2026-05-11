// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Converter registry -- maps target system IDs to converter implementations.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once
#include <convert/convert.bitmap.h>
#include <convert/convert.palette.h>
#include <vector>
#include <string>

namespace RetrodevLib {
	//
	// Display name constants for all supported target systems.
	// These are the authoritative strings stored in project files and shown in the UI.
	// All comparisons against targetSystem must use these constants.
	//
	namespace SupportedSystems {
		inline const std::string AmstradCPC  = "Amstrad CPC/CPC+";
		inline const std::string ZXSpectrum  = "ZX Spectrum";
		inline const std::string Commodore64 = "Commodore 64";
		inline const std::string MSX         = "MSX";
	}
	//
	// Converter registry and system utilities.
	//
	class Converters {
	public:
		//
		// Sentinel value returned by GetAspectSystems() as the first entry.
		// When selected, no aspect correction is applied (square pixels).
		//
		static constexpr const char* Agnostic = "Agnostic";
		//
		// Returns the list of supported systems for conversion
		//
		static std::vector<std::string> Get();
		//
		// Returns the stable script-facing identifier for a given system display name.
		// Used to match the @target tag in export scripts against the active project system.
		// Returns an empty string if the display name is not recognised.
		//
		static std::string GetSystemId(const std::string& displayName);
		//
		// Returns the list of systems available for aspect ratio selection in the map canvas.
		// The first entry is always "Agnostic" (square pixels, no aspect correction).
		// The remaining entries are the display names of supported systems, in the same
		// order as Get(), filtered to those that have aspect ratio data.
		//
		static std::vector<std::string> GetAspectSystems();
		//
		// Returns the pixel aspect scale factors and the list of available modes for the
		// given system display name and mode string.
		// For "Agnostic" or any unrecognised system, hScale and vScale are set to 1.0
		// and modes is returned empty.
		// hScale and vScale follow the same normalisation as ScreenAspect: smaller axis = 1.0.
		//
		static void GetAspectData(const std::string& system, const std::string& mode, float& hScale, float& vScale, std::vector<std::string>& modes);
		//
		// Returns a converter for the given system and parameters
		//
		static std::shared_ptr<IBitmapConverter> GetBitmapConverter(GFXParams* params);
		//
		// Create a palette converter for the given system (independent of bitmap conversion)
		// Used for color selection without image processing
		//
		static std::shared_ptr<IPaletteConverter> GetPaletteConverter(
			const std::string& systemName,
			const std::string& targetMode,
			const std::string& paletteType
		);
	};
}
