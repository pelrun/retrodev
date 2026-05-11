// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Version constants.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once
#include <string>

namespace RetrodevLib {

	//
	// Version numbers -- Major and Minor are updated manually.
	// Build is replaced by the release build script; the token below is
	// the placeholder that gets restored after each release build.
	//
	static constexpr int k_versionMajor = 1;
	static constexpr int k_versionMinor = 0;
	static constexpr const char* k_versionBuild = "@BUILD@"
												  "BETA";
	//
	inline std::string GetVersion() {
		return std::to_string(k_versionMajor) + "." + std::to_string(k_versionMinor) + "." + k_versionBuild;
	}
}
