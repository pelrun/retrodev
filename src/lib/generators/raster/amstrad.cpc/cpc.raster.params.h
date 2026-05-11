// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Amstrad CPC raster parameters -- platform-specific configuration for CPC raster code generation.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <cstdint>
#include <string>

namespace RetrodevLib {
	//
	// Raster code generation timing modes (Amstrad CPC specific).
	// Interrupt-driven: uses interrupt-slot-based handler chain.
	// Timed raster loop: uses explicit WaitScanlines / WaitNops between register changes.
	//
	enum class CpcRasterTimingMode : uint8_t {
		InterruptDriven = 0,
		TimedRasterLoop = 1
	};

	//
	// Amstrad CPC-specific raster parameters.
	// Stored opaquely in RasterParams::config (serialized to JSON).
	// Deserialized by the CPC code generator before use.
	//
	struct CpcRasterParams {
		//
		// Generator rupture name used as the assembly label prefix.
		//
		std::string ruptureName = "rupture";
		//
		// Optional output path for exported assembly text.
		//
		std::string outputAsmPath;
		//
		// Generator timing mode: Interrupt-driven or Timed raster loop.
		//
		uint8_t generatorMode = (uint8_t)CpcRasterTimingMode::InterruptDriven;
		//
		// Target CRTC type (HD6845S, UM6845R, MC6845, ASIC, etc.)
		// Used for timing rules validation and type-specific behaviour.
		// 0 = Type0 (HD6845S, default)
		//
		uint8_t targetCrtcType = 0;
		//
		// CRTC COUNTER STATE AT Int1 ENTRY (Post-VSync) -- Hardware-Measured Values
		// ============================================================================
		// These values represent the actual CRTC counter state when the Int1 interrupt handler
		// code begins execution, AFTER GA_VSyncWaitON has waited for VSync rising edge and
		// the CRTC has already progressed into the new frame.
		//
		// VALUES ARE HARDWARE-MEASURED (emulator-validated):
		//   - initC0 = 31:  Horizontal counter is at char position 31 (near end of first scanline)
		//   - initC4 = 0:   Character row counter at row 0 (first row of frame)
		//   - initC9 = 2:   Scanline counter at scanline 2 (2 scanlines into first row)
		//     [This explains why Int1 can only safely write registers starting at scanline 3-4]
		//   - initC5 = 0:   Vertical adjust counter not yet active
		//   - initIC = 0:   GA interrupt counter state
		//   - initC3h = 0:  VSync width counter (Compendium sec.20.3.1 timing)
		//
		// IMPACT ON TIMING:
		//   The 2-scanline post-VSync window (C9=0..1) is hardware-reserved for VMA pipeline safety
		//   (Compendium sec.20.3.1, sec.13.2.1). No register writes are valid during this window.
		//   By the time Int1 code finishes WaitNops + ld b,2, the next safe write point is scanline 3-4.
		//
		// CONSEQUENCE FOR SCHEDULING:
		//   Scanlines 0-2 of each frame have zero budget; nothing may be placed there.
		//   This is enforced by the scheduler, not a workaround -- it directly follows from
		//   the actual hardware entry state.
		//
		int initC0 = 31;    // Horizontal counter (0..R0)
		int initC4 = 0;     // Character row counter (0..R4)
		int initC9 = 2;     // Raster line within row (0..R9) - CRITICAL: post-VSync window
		int initC5 = 0;     // Vertical adjust counter (for CRTC 1/2)
		int initIC = 0;     // GA interrupt counter (0..51)
		int initC3h = 0;    // VSync width counter (0..16 or R3[7:4])
	};
}
