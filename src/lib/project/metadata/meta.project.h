// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Project metadata -- top-level project file structure.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <project/project.h>
#include <glaze/glaze.hpp>
#include <glaze/json/custom.hpp>
#include "meta.color.correction.h"
#include "meta.quantization.h"
#include "meta.dithering.h"
#include "meta.converter.h"
#include "meta.resize.h"
#include "meta.tileset.h"
#include "meta.sprites.h"
#include "meta.map.h"
#include "meta.build.h"
#include "meta.palette.h"
#include "meta.raster.h"
#include <string>
#include <vector>

//
// Glaze metadata for JSON serialization
//
template <> struct glz::meta<RetrodevLib::ProjectImageEntry> {
	using T = RetrodevLib::ProjectImageEntry;
	static constexpr auto value = object("filePath", &T::filePath);
};
//
// Glaze metadata for JSON serialization
//
template <> struct glz::meta<RetrodevLib::ProjectAudioEntry> {
	using T = RetrodevLib::ProjectAudioEntry;
	static constexpr auto value = object("filePath", &T::filePath);
};
//
// Glaze metadata for JSON serialization
//
template <> struct glz::meta<RetrodevLib::ProjectSourceEntry> {
	using T = RetrodevLib::ProjectSourceEntry;
	static constexpr auto value = object("filePath", &T::filePath);
};
//
// Glaze metadata for JSON serialization
//
template <> struct glz::meta<RetrodevLib::ProjectScriptEntry> {
	using T = RetrodevLib::ProjectScriptEntry;
	static constexpr auto value = object("filePath", &T::filePath);
};
//
// Glaze metadata for JSON serialization
//
template <> struct glz::meta<RetrodevLib::ProjectDataEntry> {
	using T = RetrodevLib::ProjectDataEntry;
	static constexpr auto value = object("filePath", &T::filePath);
};
//
// Glaze metadata for JSON serialization
//
template <> struct glz::meta<RetrodevLib::ExportParams> {
	using T = RetrodevLib::ExportParams;
	static constexpr auto value = object("scriptPath", &T::scriptPath, "outputName", &T::outputName, "scriptParams", &T::scriptParams);
};
//
// Glaze metadata for JSON serialization
//
template <> struct glz::meta<RetrodevLib::ProjectBuildBitmapEntry> {
	using T = RetrodevLib::ProjectBuildBitmapEntry;
	static constexpr auto value = object("name", &T::name, "sourceFilePath", &T::sourceFilePath, "params", &T::params, "exportParams", &T::exportParams);
};
//
// Glaze metadata for JSON serialization
//
template <> struct glz::meta<RetrodevLib::ProjectBuildTilesEntry> {
	using T = RetrodevLib::ProjectBuildTilesEntry;
	static constexpr auto value =
		object("name", &T::name, "sourceFilePath", &T::sourceFilePath, "params", &T::params, "tileParams", &T::tileParams, "exportParams", &T::exportParams);
};
//
// Glaze metadata for JSON serialization
//
template <> struct glz::meta<RetrodevLib::ProjectBuildSpriteEntry> {
	using T = RetrodevLib::ProjectBuildSpriteEntry;
	static constexpr auto value =
		object("name", &T::name, "sourceFilePath", &T::sourceFilePath, "params", &T::params, "spriteParams", &T::spriteParams, "exportParams", &T::exportParams);
};
//
// Glaze metadata for JSON serialization
//
template <> struct glz::meta<RetrodevLib::ProjectBuildMapEntry> {
	using T = RetrodevLib::ProjectBuildMapEntry;
	static constexpr auto value = object("name", &T::name, "mapParams", &T::mapParams, "exportParams", &T::exportParams);
};
//
// Glaze metadata for JSON serialization
//
template <> struct glz::meta<RetrodevLib::ProjectBuildBuildEntry> {
	using T = RetrodevLib::ProjectBuildBuildEntry;
	static constexpr auto value = object("name", &T::name, "sourceParams", &T::sourceParams, "exportParams", &T::exportParams);
};
//
// Glaze metadata for JSON serialization
//
template <> struct glz::meta<RetrodevLib::ProjectBuildPaletteEntry> {
	using T = RetrodevLib::ProjectBuildPaletteEntry;
	static constexpr auto value = object("name", &T::name, "paletteParams", &T::paletteParams, "exportParams", &T::exportParams);
};
//
// Glaze metadata for JSON serialization
//
template <> struct glz::meta<RetrodevLib::ProjectFile> {
	using T = RetrodevLib::ProjectFile;
	static constexpr auto value =
		object("version", &T::version, "ProjectName", &T::ProjectName, "images", &T::images, "audio", &T::audio, "sources", &T::sources, "scripts", &T::scripts, "data", &T::data,
			   "buildBitmaps", &T::buildBitmaps, "buildTiles", &T::buildTiles, "buildSprites", &T::buildSprites, "buildMaps", &T::buildMaps, "buildBuilds", &T::buildBuilds,
			   "buildPalettes", &T::buildPalettes, "buildRasters", &T::buildRasters, "buildFolders", &T::buildFolders, "selectedBuildItem", &T::selectedBuildItem);
};
