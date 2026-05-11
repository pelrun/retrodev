// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Project metadata -- generic raster build item serialisation.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <project/project.h>
#include <generators/raster/raster.params.h>
#include <glaze/glaze.hpp>
#include <string>

//
// Glaze serialisation for generic RasterParams
// Platform-specific configuration is serialized opaquely in the `config` field.
//
template <> struct glz::meta<RetrodevLib::RasterParams> {
	using T = RetrodevLib::RasterParams;
	static constexpr auto value =
		glz::object("targetSystem", &T::targetSystem, "targetMode", &T::targetMode, "targetPaletteType", &T::targetPaletteType, "exportParams", &T::exportParams, "commands", &T::commands, "selectedCommand", &T::selectedCommand, "config", &T::config);
};
//
// Glaze serialisation for ProjectBuildRasterEntry
//
template <> struct glz::meta<RetrodevLib::ProjectBuildRasterEntry> {
	using T = RetrodevLib::ProjectBuildRasterEntry;
	static constexpr auto value = glz::object("name", &T::name, "rasterParams", &T::rasterParams);
};
