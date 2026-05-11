// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Amstrad CPC CRTC -- raster command types, frame simulation helpers, validation and Z80 code generation.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <generators/raster/raster.params.h>
#include <generators/raster/amstrad.cpc/cpc.raster.params.h>
#include <string>
#include <vector>
#include <map>
#include <variant>
#include <functional>

//
// Debug system: set to enable structured logging to a file.
// When disabled (default), no overhead and zero output.
//
// #define CPC_RASTER_DEBUG
#ifdef CPC_RASTER_DEBUG
#  define CPC_RASTER_DEBUG_PATH "c:/temp/cpc_raster.log"
#endif

namespace RetrodevLib {

	//
	// Command types supported by the CPC CRTC raster editor.
	// The integer value is persisted as part of the serialized command blob.
	//
	enum class CpcRasterCommandType {
		Frame      = 0,  // CRTC frame (registers + timing)
		Effect     = 1,  // Register writes at a specific scanline
		Variable   = 2,  // Memory location write at a specific scanline
		Subroutine = 3   // User-supplied Z80 routine call at a specific scanline
	};
	//
	// CRTC register values for a full-frame definition.
	// Field names match the CRTC register numbers (R0..R9).
	// Defaults reproduce the standard CPC PAL frame: 312 scan lines, 25 displayed rows.
	//
	struct RasterFrameCmd {
		uint8_t r0  = 63;    // Horizontal total (chars per line - 1)
		uint8_t r1  = 40;    // Horizontal displayed (chars)
		uint8_t r2  = 46;    // Horizontal sync position (char)
		uint8_t r3  = 0x8E;  // Sync widths: bits[3:0]=hsync width (14), bits[7:4]=vsync width (8)
		uint8_t r4  = 38;    // Vertical total (char rows - 1)
		uint8_t r5  = 0;     // Vertical total adjust (scan lines, 0..31)
		uint8_t r6  = 25;    // Vertical displayed (char rows)
		uint8_t r7  = 30;    // Vertical sync position (char row)
		uint8_t r9  = 7;     // Maximum scan line address (scan lines per char row - 1)
		uint8_t r12 = 0x30;  // Video memory address high byte (page select + size bits)
		uint8_t r13 = 0x00;  // Video memory address low byte
		//
		// When true, VSync is suppressed for this frame (R7 is set to 255 in the
		// generated code). The frame's scan lines accumulate into the running total
		// of the current screen group instead of closing the frame. The screen group
		// is closed (and the 312-line check applied) by the first frame that has
		// disableVSync = false.
		//
		bool disableVSync = false;
		//
		// Bitmask of registers explicitly written by this command.
		// MASK_R12 is a special flag: when set, the generated code will write R12/R13
		// using a game-supplied symbol (no specific value is stored here -- the actual
		// address is defined by the application).
		//
		uint32_t activeMask = 0;  // Start with all registers unchecked; user activates what they need
		//
		// SMC (Self-Modifying Code) patchable registers.
		// When a bit is set, the generated code emits a label pointing to the immediate byte
		// so user code can patch the value at runtime without reassembling.
		//
		uint32_t smcMask = 0;
		//
		// Per-register SMC patch function mask. When a bit is set for a register,
		// a callable Z80 patch function is generated that atomically updates both
		// the register write AND all dependent wait instructions (for boundary-affected
		// registers R4, R5, R9). Phase 1 supports R5 only.
		//
		uint32_t smcPatchFunctionMask = 0;
		//
		// Per-register SMC label overrides. If non-empty for a register, use as label instead of default.
		//
		std::map<uint8_t, std::string> smcLabelOverrides;
		//
		// Per-register mask bits
		//
		static constexpr uint32_t MASK_R0  = (1u << 0);
		static constexpr uint32_t MASK_R1  = (1u << 1);
		static constexpr uint32_t MASK_R2  = (1u << 2);
		static constexpr uint32_t MASK_R3  = (1u << 3);
		static constexpr uint32_t MASK_R4  = (1u << 4);
		static constexpr uint32_t MASK_R5  = (1u << 5);
		static constexpr uint32_t MASK_R6  = (1u << 6);
		static constexpr uint32_t MASK_R7  = (1u << 7);
		static constexpr uint32_t MASK_R9  = (1u << 8);
		//
		// MASK_R12: flag that this frame changes the VMA base address (R12/R13).
		// No actual register value is stored; code generation emits a symbolic reference.
		//
		static constexpr uint32_t MASK_R12 = (1u << 9);
		static constexpr uint32_t MASK_ALL = 0x03FFu;
	};
	//
	// Register write target (CRTC or Gate Array).
	//
	enum class CpcRegTarget : uint8_t {
		CRTC = 0,  // CRTC register (reg field used)
		GA   = 1   // Gate Array (value field = full command byte)
	};
	//
	// Effect target line mode: how to compute the target scanline.
	//
	enum class EffectTargetMode : uint8_t {
		Absolute = 0,  // targetLine is a specific scanline (0–311)
		Relative = 1   // targetLine is relative offset from previous Effect (0 = same, 1 = next)
	};
	//
	// A single register write: target, register number, and value.
	//
	struct CpcRegWrite {
		CpcRegTarget target = CpcRegTarget::CRTC;
		uint8_t reg   = 0;    // CRTC register (0..15); unused for GA
		uint8_t value = 0;    // Register value or GA command byte
		bool smcPatch = false;  // Emit SMC label for this write
		std::string smcLabelOverride;  // Custom label name if non-empty
	};
	//
	// Variable write: memory location and value to write at a specific scanline.
	//
	struct CpcVariableSet {
		std::string varName;  // Label suffix — generates prefix_VarName in output
		uint8_t value = 1;    // Value to write (typically 1 for signal)
	};
	//
	// CRTC type enumeration (from Compendium §14).
	//
	enum class CpcCrtcType : uint8_t {
		Type0   = 0,   // HD6845S — original Amstrad CPC
		Type1   = 1,   // UM6845R — some CPC 464 units
		Type2   = 2,   // MC6845  — some CPC 6128 units
		Type3   = 3,   // ASIC (CPC+ / GX4000)
		Type4   = 4,   // MC6845B — rare
		Unknown = 255
	};
	//
	// Separate command types for cleaner serialization (each type only includes relevant fields)
	//
	struct CpcFrameCommand {
		CpcRasterCommandType type = CpcRasterCommandType::Frame;
		std::string name = "Frame";
		bool enabled = true;
		RasterFrameCmd frame;
	};

	struct CpcEffectCommand {
		CpcRasterCommandType type = CpcRasterCommandType::Effect;
		std::string name = "Effect";
		bool enabled = true;
		EffectTargetMode targetMode = EffectTargetMode::Relative;
		//
		// Target scanline: interpretation depends on targetMode
		// Absolute: scanline number (0-311)
		// Relative: offset from previous effect (0=same line, 1=next line, 6=6 lines later, etc.)
		//
		int targetScanline = 1;
		std::vector<CpcRegWrite> writes;
	};

	struct CpcVariableCommand {
		CpcRasterCommandType type = CpcRasterCommandType::Variable;
		std::string name = "Variable";
		bool enabled = true;
		int targetLine = 0;
		bool unrestrained = true;
		CpcVariableSet variable;
	};

	struct CpcSubroutineCommand {
		CpcRasterCommandType type = CpcRasterCommandType::Subroutine;
		std::string name = "Subroutine";
		bool enabled = true;
		int targetLine = 0;
		std::string subroutineName;
	};

	//
	// Tagged union: holds one of the four command types
	//
	using CpcRasterCommand = std::variant<CpcFrameCommand, CpcEffectCommand, CpcVariableCommand, CpcSubroutineCommand>;

	//
	// Helper to get the command type from any variant command
	//
	inline CpcRasterCommandType GetCommandType(const CpcRasterCommand& cmd) {
		if (std::holds_alternative<CpcFrameCommand>(cmd))
			return std::get<CpcFrameCommand>(cmd).type;
		else if (std::holds_alternative<CpcEffectCommand>(cmd))
			return std::get<CpcEffectCommand>(cmd).type;
		else if (std::holds_alternative<CpcVariableCommand>(cmd))
			return std::get<CpcVariableCommand>(cmd).type;
		else
			return std::get<CpcSubroutineCommand>(cmd).type;
	}

	//
	// Helper to get the command name from any variant command
	//
	inline const std::string& GetCommandName(const CpcRasterCommand& cmd) {
		if (std::holds_alternative<CpcFrameCommand>(cmd))
			return std::get<CpcFrameCommand>(cmd).name;
		else if (std::holds_alternative<CpcEffectCommand>(cmd))
			return std::get<CpcEffectCommand>(cmd).name;
		else if (std::holds_alternative<CpcVariableCommand>(cmd))
			return std::get<CpcVariableCommand>(cmd).name;
		else
			return std::get<CpcSubroutineCommand>(cmd).name;
	}
	//
	// Severity of a single validation entry.
	//
	enum class CpcValidationSeverity {
		Error,    // Hard CRTC constraint violation -- will not work correctly on hardware
		Warning   // Advisory -- non-standard or risky setting that may work but should be verified
	};
	//
	// A single violation produced by CPCCrtc::Validate.
	// cmdIndex is the index inside RasterParams::commands of the originating command (-1 = global).
	//
	struct CpcValidationEntry {
		CpcValidationSeverity severity = CpcValidationSeverity::Error;
		int cmdIndex = -1;
		std::string message;
	};
	//
	// Result returned by CPCCrtc::Validate.
	//
	struct CpcValidationResult {
		bool ok = true;
		std::vector<CpcValidationEntry> entries;
	};

	namespace CpcRaster {

		//
		// Register sentinel constants (used for special write types: GA, subroutine, variable).
		// These occupy slots in the ScheduledAction.reg field above the standard CRTC register range.
		//
		static constexpr uint8_t kRegGA         = 253;  // Gate Array color/mode command
		static constexpr uint8_t kRegSubroutine = 254;  // User-supplied Z80 subroutine call
		static constexpr uint8_t kRegVariable   = 255;  // Memory location variable write

		//
		// CRTC counter snapshot at a given character clock position.
		// Used by CrtcSimulator to track state during generation.
		//
		struct CrtcCounters {
		// Primary CRTC counters
		int c0  = 0;    // Horizontal (0..R0)
		int c4  = 0;    // Character row (0..R4)
		int c9  = 0;    // Raster line within row (0..R9)
		int c5  = 0;    // Vertical adjust (0..R5-1)
		bool inAdjust      = false;  // CRTC in vertical-total-adjust phase (C5 active per scanline)
		bool frameComplete = false;  // Set when frame boundary is crossed, cleared each call
		int c3l = 0;    // HSync width counter (0..R3[3:0])
		int c3h = 0;    // VSync width counter (0..R3[7:4] or 16)
		int ma  = 0;    // Memory address counter (14-bit, 0..16383)
		// GA interrupt counter
		int ic  = 0;    // Interrupt counter (0..51)
		// Derived state
		bool hsync          = false;  // HSync active (C0 >= R2, running)
		bool vsync          = false;  // VSync active (C3h > 0)
		bool vsyncStart     = false;  // VSync fires this char clock (C4 just matched R7)
		bool disPen         = false;  // Display enabled: (C4 < R6) && (C0 < R1)
		bool maOverflow     = false;  // MA page bits changed on this advance
		bool intFired       = false;  // IC just reached 52 and fired
		bool vsyncIntFired  = false;  // VSync-triggered INT (IC >= 32 at VSync)
		bool vsyncIcReset   = false;  // IC was unconditionally zeroed 2 HSyncs after VSync
		// Position tracking
		int interruptSlot = 0;     // 0..5 (which GA interrupt period)
		int icLineInSlot  = 0;     // 0..51 (IC value within slot)
		int absNop        = 0;     // Total NOPs elapsed since init
		int absScanline   = 0;     // Total scanlines (C0 wraps) since init
	};
	//
	// Register known-state tracker (for hoisting and elision optimizations).
	//
	struct CrtcKnownState {
		uint8_t regValues[16] = {};  // actual register values when known
		bool regKnown[16] = {};      // true if register state is known
		bool isKnown(uint8_t reg) const { return reg < 16 && regKnown[reg]; }
		bool matches(uint8_t reg, uint8_t value) const {
			return reg < 16 && regKnown[reg] && regValues[reg] == value;
		}
		void set(uint8_t reg, uint8_t value) {
			if (reg < 16) { regValues[reg] = value; regKnown[reg] = true; }
		}
		void clear(uint8_t reg) {
			if (reg < 16) regKnown[reg] = false;
		}
		void resetToDefaults();  // fills from CPC BIOS defaults
	};
	//
	// CRTC register constraint rule: encapsulates timing and positioning constraints for a single register.
	// Each register has different constraints depending on CRTC state and frame context.
	// Rules are evaluated to determine safe scanline windows and NOP offset constraints.
	//
	struct RegisterRule {
		// Returns earliest safe scanline in [frameStartSl, frameEndSl) where register can be written.
		// Called by scheduler to find the preferred placement scanline.
		std::function<int(int frameStartSl, int frameEndSl, const RasterFrameCmd&, const std::vector<CrtcCounters>& lineState)> getEarliestScanline;

		// Returns latest scanline in [frameStartSl, frameEndSl) where register write is still effective.
		// Called by scheduler to bound the safe deferral window.
		std::function<int(int frameStartSl, int frameEndSl, const RasterFrameCmd&, const std::vector<CrtcCounters>& lineState)> getLatestScanline;

		// Returns minimum NOP offset (within scanline) where write may start (-1 = unconstrained).
		// Used to delay writes until a safe position within the scanline (e.g., R1 must write late).
		std::function<int(int scanline, int frameStartSl, const RasterFrameCmd&, uint8_t value, const CrtcCounters& lineState)> getNopMinOffset;

		// Returns maximum NOP offset (within scanline) where write may start (-1 = unconstrained).
		// Used to limit writes to early positions (e.g., R7 must write before HSYNC window).
		// Receives the scanline and CRTC counter state at that scanline for simulator-based queries.
		std::function<int(int scanline, const CrtcCounters& ctr, const RasterFrameCmd&, uint8_t value)> getNopMaxOffset;

		// Validation check: return non-empty string if write would be unsafe, empty string if OK.
		std::function<std::string(int scanline, const CrtcCounters&, const RasterFrameCmd&, uint8_t value)> checkWrite;
	};
	//
	// Per-slot timing breakdown for a raster generation.
	//
	struct SlotTimingReport {
		int slot               = -1;   // 0..5 (interrupt) or -1 (full frame for loop)
		int overheadNops       = 0;    // Handler/loop overhead (RST, push, ret, ei, etc.)
		int waitNops           = 0;    // Total idle time via WaitScanlines/WaitNops
		int activeNops         = 0;    // CRTC/GA/variable write costs
		int freeNops           = 0;    // Remaining NOPs available for user code
		int elidedNops         = 0;    // NOPs recovered by redundant-write elimination
		std::vector<std::pair<int, int>> waitRanges;  // {startNop, durationNops} per wait
	};
	//
	// Full timing report for a frame or interrupt sequence.
	//
	struct RasterTimingReport {
		std::vector<SlotTimingReport> slots;  // one per slot/frame
		int totalWaitNops      = 0;
		int totalActiveNops    = 0;
		int totalFreeNops      = 0;
		int totalElidedNops    = 0;  // total NOPs recovered by hoisting/elision
		std::vector<std::pair<uint8_t, uint8_t>> hoisteRegs;  // {reg, value} emitted to Start
		std::vector<std::string> errors;     // scheduling/generation errors
		std::vector<std::string> warnings;   // non-fatal issues
	};
	//
	// One pre-built slot entry consumed by RenderMonitorImage.
	// Mirrors the slot structure built by the visualizer: the merged CRTC state plus
	// the absolute scan-line range [startSl, endSl) that this command occupies.
	// slotIndex is the 0-based slot number used to pick the checker-pattern hue.
	//
	struct MonitorSlot {
		RasterFrameCmd state;
		int startSl   = 0;
		int endSl     = 0;
		int slotIndex = 0;
	};
	//
	// Character-clock-level CRTC state simulator.
	// Tracks all CRTC and GA counters with NOP-accuracy. Used by code generator and MonitorSim.
	//
	class CrtcSimulator {
	public:
		//
		// Initialize simulator with known counter state and register values.
		// Call before generating code, with initial values from RasterParams.
		//
		void init(const CrtcCounters& initialCounters, const RasterFrameCmd& initialRegs);
		//
		// Advance by exactly N NOPs (character clocks). Updates all counters.
		// Call after every emitted instruction.
		//
		void advance(int nops);
		//
		// Apply a register write to the live register state.
		// Does NOT advance time; call advance() separately.
		//
		void applyRegWrite(uint8_t reg, uint8_t value);
		//
		// Query current counter snapshot.
		//
		const CrtcCounters& counters() const;
		const RasterFrameCmd& registers() const;
		//
		// Check whether writing reg=value is safe at the CURRENT counter position.
		// Returns list of violations/warnings (empty = safe).
		//
		std::vector<CpcValidationEntry> checkWrite(
			uint8_t reg, uint8_t value,
			CpcCrtcType crtcType = CpcCrtcType::Unknown) const;
		//
		// NOPs remaining in the current scanline from current C0 position.
		//
		int nopsToEndOfLine() const;
		//
		// NOPs until next occurrence of specific C4/C9 state (for safe-window search).
		// Returns -1 if not reachable within 312 lines.
		//
		int nopsUntil(int targetC4, int targetC9) const;
		//
		// NOPs until next VMA reload trigger (C4=C9=C0=0 for types 0/2/3/4;
		// or next C0=0 while C4=0 for type 1).
		// Returns 0 if at trigger point right now.
		//
		int nopsUntilMaReset(CpcCrtcType crtcType = CpcCrtcType::Type0) const;
		//
		// Advance simulator by exactly one full frame: return to C4=0, C9=0 state.
		// Returns total NOPs consumed by the frame (typically (R4+1)*(R9+1)+R5).
		//
		int runFrame();
		//
		// Total NOPs elapsed since initialization.
		//
		int currentScanline() const;
		//
		// Register timing rule accessor: returns constraint lambda set for register.
		//
		RegisterRule GetRegisterRule(uint8_t reg) const;
		//
		// === Static helpers (formerly CRTCSim) ===
		//
		// Total scanlines in a frame: (R4+1)*(R9+1) + R5
		//
		static int TotalScanLines(const RasterFrameCmd& f);
		//
		// Scanlines where GA interrupt fires (52-line periods, 1-based)
		//
		static std::vector<int> InterruptLines(const RasterFrameCmd& f);
		//
		// True if scanline is within vertical display window
		//
		static bool IsDisplayScanLine(const RasterFrameCmd& f, int scanLine);
		//
		// True if character column is within horizontal sync pulse
		//
		static bool IsHSyncChar(const RasterFrameCmd& f, int col);
		//
		// True if scanline is within vertical sync band
		//
		static bool IsVSyncScanLine(const RasterFrameCmd& f, int scanLine);
		//
		// Merge two frame commands: apply src onto base
		//
		static RasterFrameCmd MergeFrame(const RasterFrameCmd& base, const RasterFrameCmd& src);
		//
		// Build a pixel-accurate RGBA32 monitor image from a list of pre-merged slots.
		// Canvas is PAL-proportioned: outWidth = maxCols * 8 pixels (8 pixels per char
		// column), outHeight = totalScanLines.  The caller should display the result at a
		// 4:3 aspect ratio to reproduce the correct monitor geometry.
		// outPixels is filled with width*height RGBA32 values (R in the lowest byte),
		// row-major, top-to-bottom.
		//
		// Colour mapping:
		//   Active VMA (display cols 0..R1-1, display rows) -- per-slot checker pattern
		//   Horizontal/vertical border                      -- dark grey
		//   HSync column                                    -- purple
		//   VSync row                                       -- cyan-blue
		//   HSync inside VSync                              -- dark mauve
		//   Outside all defined frames                      -- black
		//
		static void RenderMonitorImage(
			const std::vector<MonitorSlot>& slots,
			int maxCols,
			int totalScanLines,
			int monitorPhaseLines,
			std::vector<uint32_t>& outPixels,
			int& outWidth,
			int& outHeight);

	private:
		CrtcCounters   m_ctr;
		RasterFrameCmd m_regs;
		int            m_interruptSlot    = 0;
		int            m_vsyncIcCountdown = 0;  // Countdown 2 HSyncs after VSync; resets IC when 0

		void advanceOneChar();  // Advance by 1 NOP
		void onLineEnd();       // Handle C0 wrap, update C9/C4/C5/IC/VSync/MA
	};
	//
	}  // namespace CpcRaster

	//
	// CPC CRTC raster editor entry point.
	// Instance-based class: one instance per opened raster document.
	// Handles all serialization/deserialization of generic RasterParams, CPC commands, and CPC config.
	//
	class CPCRaster {
	public:
		CPCRaster() = default;
		~CPCRaster() = default;

		//
		// Load a complete project from a RasterParams.
		// Deserializes the opaque commands array and config field into member state.
		// Must be called before any getter methods.
		//
		void LoadProject(const RasterParams& genericParams);

		//
		// Save the current project state back to a RasterParams.
		// Serializes the CPC commands and config into the opaque fields.
		//
		void SaveProject(RasterParams& genericParams);

		//
		// Get the loaded generic RasterParams.
		// Returns nullptr if LoadProject has not been called.
		//
		RasterParams* GetRasterParams();
		const RasterParams* GetRasterParams() const;

		//
		// Get the loaded CPC commands array.
		//
		std::vector<CpcRasterCommand>& GetCommands();
		const std::vector<CpcRasterCommand>& GetCommands() const;

		//
		// Get the loaded CPC configuration.
		//
		CpcRasterParams& GetCpcConfig();
		const CpcRasterParams& GetCpcConfig() const;

		//
		// Static helpers for single command serialization (used internally and by tests).
		// Deserialize a command from its opaque string representation.
		// Returns a default-constructed CpcRasterCommand if the string is empty or malformed.
		//
		static CpcRasterCommand Load(const std::string& blob);
		//
		// Serialize a command to its opaque string representation for storage in RasterParams::commands[].
		//
		static std::string Save(const CpcRasterCommand& cmd);
		//
		// Serialize/deserialize CPC configuration
		//
		static std::string SaveCpcConfig(const CpcRasterParams& config);
		static CpcRasterParams LoadCpcConfig(const std::string& configJson);

		//
		// Validate the loaded project against CPC CRTC constraints.
		// Works with internal library state directly (no serialization needed).
		//
		void Validate(CpcValidationResult& result);
		//
		// Generate Z80 assembly source for the loaded raster effect.
		// Works with internal library state directly (no serialization needed).
		// outReport (optional) is filled with per-slot timing and optimization data.
		// Returns the generated source, or an empty string if generation is not yet possible.
		//
		std::string GenerateCode(CpcRaster::RasterTimingReport* outReport = nullptr);

		//
		// Render pixel-accurate monitor image from loaded raster commands.
		// Works with internal library state directly (no serialization needed).
		// Uses CrtcSimulator to build frame slots and RenderMonitorImage to generate pixels.
		// outPixels is filled with RGBA32 pixel data (row-major, top-to-bottom).
		// outPhaseLines returns the cumulative monitor phase drift for free-running sync.
		// Returns true if image was successfully rendered, false if project is not ready.
		//
		bool Monitor(
			std::vector<uint32_t>& outPixels,
			int& outWidth,
			int& outHeight,
			int& outPhaseLines);

	private:
		//
		// Pre-generation analysis: detect registers that are written to only one value.
		// Works directly with m_cpcCommands (no serialization).
		//
		CpcRaster::CrtcKnownState DetectConstantRegisters();

		// Instance state: loaded project data
		RasterParams m_rasterParams;
		std::vector<CpcRasterCommand> m_cpcCommands;
		CpcRasterParams m_cpcConfig;
	};

}
