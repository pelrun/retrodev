// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Source asset -- RASM assembler invocation.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include "source.h"

namespace RetrodevLib {
	namespace RasmImpl {

		//
		// Assemble a single source file using RASM.
		// source:      source file path (relative to the project folder)
		// includeDirs: include search paths (-I)
		// defines:     preprocessor defines injected before assembly (e.g. "MYFLAG=1")
		// toolOpts:    raw CLI options string from SourceParams::toolOptions["RASM"]
		// projectDir:  absolute path to the project folder (used to resolve relative paths)
		// Returns true if assembly succeeded with no errors.
		//
		bool Build(const std::string& source, const std::vector<std::string>& includeDirs, const std::vector<std::string>& defines, const std::string& toolOpts,
				   const std::string& projectDir);

	}
}
