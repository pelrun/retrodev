// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Converter registry -- maps target system IDs to converter implementations.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#include "converters.h"
#include "amstrad.cpc/cpc.bitmap.h"
#include <system/amstrad.cpc/devices/cpc.screen.h>
#include <log/log.h>

namespace RetrodevLib {

	//
	// List of supported systems for conversion
	//
	static const std::vector<std::string> SupportedSystemsNames = {SupportedSystems::AmstradCPC, SupportedSystems::ZXSpectrum, SupportedSystems::Commodore64,
																   SupportedSystems::MSX};

	//
	// Returns the list of supported systems for conversion
	//
	std::vector<std::string> Converters::Get() {
		return SupportedSystemsNames;
	}
	//
	// Maps a system display name to a stable, lowercase script-facing identifier.
	// The identifier is what script authors write in the @target tag.
	// It is decoupled from the display name so UI labels can change without
	// breaking existing scripts.
	//
	std::string Converters::GetSystemId(const std::string& displayName) {
		if (displayName == SupportedSystems::AmstradCPC)
			return "amstrad.cpc";
		if (displayName == SupportedSystems::ZXSpectrum)
			return "spectrum";
		if (displayName == SupportedSystems::Commodore64)
			return "c64";
		if (displayName == SupportedSystems::MSX)
			return "msx";
		return std::string();
	}

	//
	// Returns the list of systems available for aspect ratio selection in the map canvas.
	// "Agnostic" is always first; the remaining entries are those in SupportedSystemsNames
	// that have registered aspect data in ScreenAspect.
	//
	std::vector<std::string> Converters::GetAspectSystems() {
		std::vector<std::string> result;
		result.push_back(Agnostic);
		result.push_back(SupportedSystems::AmstradCPC);
		return result;
	}
	//
	// Resolves pixel aspect scale factors and the mode list for the given system display name.
	// Delegates to ScreenAspect so all aspect knowledge stays in the system layer.
	//
	void Converters::GetAspectData(const std::string& system, const std::string& mode, float& hScale, float& vScale, std::vector<std::string>& modes) {
		hScale = 1.0f;
		vScale = 1.0f;
		if (system == SupportedSystems::AmstradCPC) {
			modes = ConverterAmstradCPC::CPCModesList;
			float rawH, rawV;
			CPCScreen::GetPixelAspectRatio(mode, rawH, rawV);
			float minVal = rawH < rawV ? rawH : rawV;
			hScale = rawH / minVal;
			vScale = rawV / minVal;
		}
	}
	//
	// Returns a converter for the given system and parameters
	//
	std::shared_ptr<IBitmapConverter> Converters::GetBitmapConverter(GFXParams* params) {
		if (params == nullptr) {
			Log::Error("Converters::GetBitmapConverter called with null params");
			return nullptr;
		}
		//
		// Select converter based on target system
		//
		if (params->SParams.TargetSystem == SupportedSystems::AmstradCPC)
			return std::make_shared<RetrodevLib::ConverterAmstradCPC::CPCBitmap>();
		else if (params->SParams.TargetSystem == SupportedSystems::ZXSpectrum) {
			Log::Error("Converters::GetBitmapConverter ZX Spectrum converter is not implemented yet");
			return nullptr;
		} else if (params->SParams.TargetSystem == SupportedSystems::Commodore64) {
			Log::Error("Converters::GetBitmapConverter Commodore 64 converter is not implemented yet");
			return nullptr;
		} else if (params->SParams.TargetSystem == SupportedSystems::MSX) {
			Log::Error("Converters::GetBitmapConverter MSX converter is not implemented yet");
			return nullptr;
		}
		Log::Warning("Converters::GetBitmapConverter no converter for system '%s'", params->SParams.TargetSystem.c_str());
		return nullptr;
	}
	//
	// Create a palette converter for the given system (independent of bitmap conversion)
	//
	std::shared_ptr<IPaletteConverter> Converters::GetPaletteConverter(
		const std::string& systemName,
		const std::string& targetMode,
		const std::string& paletteType)
	{
		if (systemName == SupportedSystems::AmstradCPC) {
			auto palette = std::make_shared<RetrodevLib::ConverterAmstradCPC::CPCPalette>();
			//
			// Only set mode/type if provided, otherwise use palette defaults
			// Defaults: Mode 0, GA Palette
			//
			if (!targetMode.empty())
				palette->SetCPCMode(targetMode);
			if (!paletteType.empty())
				palette->SetPaletteType(paletteType);
			return palette;
		}
		Log::Warning("Converters::GetPaletteConverter: system '%s' not supported", systemName.c_str());
		return nullptr;
	}

}