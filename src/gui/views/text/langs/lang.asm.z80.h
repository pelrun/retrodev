// ---------------------------------------------------------------------------
//
// Retrodev Gui
//
// Z80 Assembler language definition and parser for TextEditor.
//
// (c) TLOTB 2026
//
// ---------------------------------------------------------------------------

#pragma once
#include <imgui.text.editor.h>

namespace RetrodevGui {

	enum class Z80TimingType {
		Cycles,      // Standard Z80 T-states
		CyclesM1,    // Z80 T-states including M1 wait states
		Instructions // Amstrad CPC NOP-equivalent units
	};

	class Z80AsmLanguage : public ImGui::ILanguageDefinition {
	public:
		Z80AsmLanguage();
		ImGui::ILanguageParser* CreateParser(ImGui::TextEditorMI* host) const override;
		Z80TimingType mTimingType = Z80TimingType::Cycles;
	};

}
