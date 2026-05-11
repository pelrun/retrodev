// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Amstrad CPC metadata -- raster-specific type serialisation.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <generators/raster/amstrad.cpc/cpc.raster.params.h>
#include <generators/raster/amstrad.cpc/cpc.raster.h>
#include <glaze/glaze.hpp>

//
// Glaze serialisation for CPC-specific raster configuration
//
template <> struct glz::meta<RetrodevLib::CpcRasterParams> {
	using T = RetrodevLib::CpcRasterParams;
	static constexpr auto value =
		glz::object("ruptureName", &T::ruptureName, "outputAsmPath", &T::outputAsmPath, "generatorMode", &T::generatorMode, "targetCrtcType", &T::targetCrtcType, "initC0", &T::initC0, "initC4", &T::initC4, "initC9", &T::initC9, "initC5", &T::initC5, "initIC", &T::initIC, "initC3h", &T::initC3h);
};

//
// Glaze serialisation for CPC CRTC raster frame command
//
template <> struct glz::meta<RetrodevLib::RasterFrameCmd> {
	using T = RetrodevLib::RasterFrameCmd;
	static constexpr auto value = glz::object(
		"r0", &T::r0, "r1", &T::r1, "r2", &T::r2, "r3", &T::r3, "r4", &T::r4,
		"r5", &T::r5, "r6", &T::r6, "r7", &T::r7, "r9", &T::r9, "r12", &T::r12,
		"r13", &T::r13, "disableVSync", &T::disableVSync, "activeMask", &T::activeMask,
		"smcMask", &T::smcMask, "smcPatchFunctionMask", &T::smcPatchFunctionMask, "smcLabelOverrides", &T::smcLabelOverrides
	);
};

//
// Glaze serialisation for CPC CRTC command types
//
template <> struct glz::meta<RetrodevLib::CpcFrameCommand> {
	using T = RetrodevLib::CpcFrameCommand;
	static constexpr auto value = glz::object("type", &T::type, "name", &T::name, "enabled", &T::enabled, "frame", &T::frame);
};

template <> struct glz::meta<RetrodevLib::CpcEffectCommand> {
	using T = RetrodevLib::CpcEffectCommand;
	static constexpr auto value = glz::object("type", &T::type, "name", &T::name, "enabled", &T::enabled, "targetMode", &T::targetMode, "targetScanline", &T::targetScanline, "writes", &T::writes);
};

template <> struct glz::meta<RetrodevLib::CpcVariableCommand> {
	using T = RetrodevLib::CpcVariableCommand;
	static constexpr auto value = glz::object("type", &T::type, "name", &T::name, "enabled", &T::enabled, "targetLine", &T::targetLine, "unrestrained", &T::unrestrained, "variable", &T::variable);
};

template <> struct glz::meta<RetrodevLib::CpcSubroutineCommand> {
	using T = RetrodevLib::CpcSubroutineCommand;
	static constexpr auto value = glz::object("type", &T::type, "name", &T::name, "enabled", &T::enabled, "targetLine", &T::targetLine, "subroutineName", &T::subroutineName);
};

template <> struct glz::meta<RetrodevLib::CpcRegWrite> {
	using T = RetrodevLib::CpcRegWrite;
	static constexpr auto value = glz::object("target", &T::target, "reg", &T::reg, "value", &T::value, "smcPatch", &T::smcPatch, "smcLabelOverride", &T::smcLabelOverride);
};

template <> struct glz::meta<RetrodevLib::CpcVariableSet> {
	using T = RetrodevLib::CpcVariableSet;
	static constexpr auto value = glz::object("varName", &T::varName, "value", &T::value);
};
