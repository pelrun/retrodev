// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// About dialog -- version and credits popup.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#include "dialog.about.h"
#include <app/app.h>
#include <app/app.resources.h>
#include <app/app.icons.mdi.h>
#include <SDL3_image/SDL_image.h>
#include <system/version.h>

namespace RetrodevGui {

	void AboutDialog::Show() {
		m_open = true;
	}
	//
	// Render a coloured hyperlink label that opens url in the default browser when clicked
	//
	void AboutDialog::RenderLink(const char* label, const char* url) {
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 180, 255, 255));
		ImGui::TextUnformatted(label);
		ImGui::PopStyleColor();
		if (ImGui::IsItemHovered()) {
			ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
			ImVec2 rMin = ImGui::GetItemRectMin();
			ImVec2 rMax = ImGui::GetItemRectMax();
			ImGui::GetWindowDrawList()->AddLine(ImVec2(rMin.x, rMax.y), ImVec2(rMax.x, rMax.y), IM_COL32(0, 180, 255, 180));
		}
		if (ImGui::IsItemClicked())
			SDL_OpenURL(url);
	}
	//
	// Render the About modal; must be called every frame
	//
	void AboutDialog::Render() {
		//
		// Open the popup on the frame after Show() was called
		//
		if (m_open) {
			ImGui::OpenPopup("About RetroDev");
			m_open = false;
		}
		//
		// Lazy-load logo from embedded resource on first render attempt
		//
		if (!m_logoLoaded) {
			m_logoLoaded = true;
			const Resource& res = Resources::GetResource("gui.res.images.about-logo.png");
			if (res._ptr != nullptr) {
				SDL_IOStream* io = SDL_IOFromConstMem(res._ptr, (int)res._size);
				if (io)
					m_logoTexture = IMG_LoadTexture_IO(Application::GetRenderer(), io, true);
			}
		}
		//
		// Center the popup on the main viewport
		//
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(1060.0f, 0.0f), ImGuiCond_Appearing);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 16.0f));
		if (ImGui::BeginPopupModal("About RetroDev", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
			ImGui::Dummy(ImVec2(0.0f, 6.0f));
			//
			// Top section: logo on the left, credits on the right
			//
			const ImVec2 logoSize(240.0f, 240.0f);
			ImGui::BeginGroup();
			if (m_logoTexture) {
				ImGui::Image(ImTextureRef(m_logoTexture), logoSize);
				// ImGui::GetWindowDrawList()->AddRect(
				//     ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
				//     IM_COL32(0, 150, 0, 200), 8.0f);
			} else {
				//
				// Styled placeholder rendered via draw list when no logo resource is found
				//
				ImVec2 pos = ImGui::GetCursorScreenPos();
				ImGui::Dummy(logoSize);
				ImDrawList* dl = ImGui::GetWindowDrawList();
				dl->AddRectFilled(pos, ImVec2(pos.x + logoSize.x, pos.y + logoSize.y), IM_COL32(0, 50, 0, 255), 8.0f);
				dl->AddRect(pos, ImVec2(pos.x + logoSize.x, pos.y + logoSize.y), IM_COL32(0, 150, 0, 200), 8.0f);
				const char* appName = "RETRODEV";
				ImVec2 textSz = ImGui::CalcTextSize(appName);
				dl->AddText(ImVec2(pos.x + (logoSize.x - textSz.x) * 0.5f, pos.y + (logoSize.y - textSz.y) * 0.5f), IM_COL32(0, 220, 0, 255), appName);
			}
			ImGui::EndGroup();
			ImGui::SameLine(0.0f, 20.0f);
			//
			// Credits column
			//
			ImGui::BeginGroup();
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 220, 0, 255));
			ImGui::Text("RetroDev");
			ImGui::PopStyleColor();
			ImGui::TextDisabled("Version %s  --  Built with love for retro machines", RetrodevLib::GetVersion().c_str());
			ImGui::Spacing();
			ImGui::TextWrapped("A modern development toolchain for classic home computers. "
							   "Convert, edit and export graphics, sprites and other assets "
							   "targeting vintage hardware.");
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();
			ImGui::TextDisabled("Author");
			ImGui::TextUnformatted("Astharoth / TLOTB");
			ImGui::Spacing();
			ImGui::TextDisabled("Website");
			RenderLink("http://www.tlotb.org", "http://www.tlotb.org");
			ImGui::EndGroup();
			ImGui::Spacing();
			//
			// Open source libraries
			//
			ImGui::SeparatorText(ICON_PACKAGE " Open Source Libraries");
			ImGui::Spacing();
			if (ImGui::BeginChild("##libs", ImVec2(0.0f, 210.0f), ImGuiChildFlags_Borders)) {
				//
				// Render one library row: name, short description, clickable URL
				//
				auto renderLib = [](const char* name, const char* desc, const char* url) {
					ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(220, 220, 220, 255));
					ImGui::TextUnformatted(name);
					ImGui::PopStyleColor();
					ImGui::SameLine();
					ImGui::TextDisabled("%s", desc);
					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 180, 255, 255));
					ImGui::TextUnformatted(url);
					ImGui::PopStyleColor();
					if (ImGui::IsItemHovered()) {
						ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
						ImVec2 rMin = ImGui::GetItemRectMin();
						ImVec2 rMax = ImGui::GetItemRectMax();
						ImGui::GetWindowDrawList()->AddLine(ImVec2(rMin.x, rMax.y), ImVec2(rMax.x, rMax.y), IM_COL32(0, 180, 255, 180));
					}
					if (ImGui::IsItemClicked())
						SDL_OpenURL(url);
				};
				renderLib("Dear ImGui", "-- Bloat-free Immediate Mode GUI for C++", "https://github.com/ocornut/imgui");
				renderLib("SDL3", "-- Simple DirectMedia Layer", "https://libsdl.org");
				renderLib("SDL3_image", "-- Image loading library for SDL3", "https://github.com/libsdl-org/SDL_image");
				renderLib("Glaze", "-- Extremely fast JSON/binary serializer for C++", "https://github.com/stephenberry/glaze");
				renderLib("FreeType", "-- Font rendering engine", "https://freetype.org");
				renderLib("AngelScript", "-- Embedded scripting language for C++", "https://www.angelcode.com/angelscript");
				renderLib("RASM", "-- Z80 cross-assembler for Amstrad CPC", "https://github.com/EdouardBERGE/rasm");
			}
			ImGui::EndChild();
			ImGui::Spacing();
			//
			// Thanks
			//
			ImGui::SeparatorText(ICON_HEART " Thanks");
			ImGui::Spacing();
			ImVec2 avail = ImGui::GetContentRegionAvail();
			float indent = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.x * 2.0f;
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + indent);
			if (ImGui::BeginChild("##thanks", ImVec2(avail.x - indent, ImGui::GetTextLineHeightWithSpacing() * 5.0f), ImGuiChildFlags_None)) {
				ImGui::Spacing();
				ImGui::TextWrapped("Special thanks to Roudoudou, DemoniakLudo, Mr.Capa, Bladerunner / TLOTB, Estrayk / Paradox and Mowgly "
								   "for their feedback, testing, information and ideas throughout development. "
								   "And to the Amstrad Power Telegram Channel \xe2\x80\x94 for the endless enthusiasm they "
								   "put on keeping alive Amstrad machines.");
			}
			ImGui::EndChild();
			//
			// Greetings
			//
			ImGui::SeparatorText(ICON_HAND_WAVE " Greetings");
			ImGui::Spacing();
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + indent);
			if (ImGui::BeginChild("##greetings", ImVec2(avail.x - indent, ImGui::GetTextLineHeightWithSpacing() * 6.0f), ImGuiChildFlags_None)) {
				ImGui::Spacing();
				ImGui::TextWrapped("Greetings fly out to the groups and projects that keep the Amstrad CPC scene alive and thriving: "
								   "4Mhz, Capasoft, CNG, The Mojon Twins, the CPCTelera authors, the 8BP authors, "
								   "Batman Group, ESP Soft, the CPCWiki team, Logon System "
								   "and all the groups who contributed with information or public source code "
								   "to keep retro development alive.");
			}
			ImGui::EndChild();
			ImGui::Separator();
			ImGui::Spacing();
			//
			// Centered close button
			//
			ImGui::SetCursorPosX((ImGui::GetWindowSize().x - 120.0f) * 0.5f);
			if (ImGui::Button("Close", ImVec2(120.0f, 0.0f)))
				ImGui::CloseCurrentPopup();
			ImGui::Dummy(ImVec2(0.0f, 6.0f));
			ImGui::EndPopup();
		}
		ImGui::PopStyleVar();
	}

}
