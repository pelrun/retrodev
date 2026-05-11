// ---------------------------------------------------------------------------
//
// Retrodev Gui
//
// AngelScript language definition and parser for TextEditor.
//
// (c) TLOTB 2026
//
// ---------------------------------------------------------------------------

#pragma once
#include <imgui.text.editor.h>

namespace RetrodevGui {

	class AngelScriptLanguage : public ImGui::ILanguageDefinition {
	public:
		AngelScriptLanguage();
		ImGui::ILanguageParser* CreateParser(ImGui::TextEditorMI* host) const override;
	};

}
