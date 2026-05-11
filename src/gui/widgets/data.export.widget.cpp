// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Data export widget -- export script selection and parameter UI.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#include "data.export.widget.h"
#include <app/app.console.h>
#include <app/app.icons.mdi.h>
#include <assets/map/map.h>
#include <export/export.h>
#include <filesystem>
#include <cstring>
#include <algorithm>

namespace RetrodevGui {

	//
	// Script picker modal -- fully self-contained, called unconditionally every frame
	// so OpenPopup and BeginPopupModal always share the same ID stack context.
	//
	void DataExportWidget::RenderScriptPicker() {
		//
		// Trigger the popup open on the frame the flag is set.
		// Build the metadata cache at that point so file I/O happens once, not every frame.
		//
		if (m_showScriptPicker) {
			ImGui::OpenPopup("Export Script");
			m_showScriptPicker = false;
			m_pickerEntries.clear();
			m_pickerSelectedIndex = -1;
			RetrodevLib::ExportEngine::Initialize();
			for (const std::string& p : RetrodevLib::Project::GetScriptFiles()) {
				RetrodevLib::ScriptMetadata meta = RetrodevLib::ExportEngine::GetScriptMetadata(p);
				//
				// Skip utility scripts -- they are helper includes, not standalone export scripts.
				//
				if (meta.exporter == "util")
					continue;
				//
				// Skip scripts whose @exporter does not match the active build type.
				// An absent tag (empty string) means the script matches any type.
				//
				if (!meta.exporter.empty() && meta.exporter != m_activeBuildTypeTag)
					continue;
				//
				// Skip scripts whose @target does not match the active system.
				// An absent tag means the script is hardware-agnostic and always shown.
				//
				if (!meta.target.empty() && meta.target != m_activeSystemId)
					continue;
				PickerEntry e;
				e.path = p;
				e.filename = std::filesystem::path(p).filename().string();
				e.folder = std::filesystem::path(p).parent_path().filename().string();
				e.description = meta.description;
				e.exporter = meta.exporter;
				e.target = meta.target;
				e.params = meta.params;
				m_pickerEntries.push_back(std::move(e));
			}
		}
		//
		// Center and size on first appearance -- wider to accommodate the table
		//
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(800.0f, 480.0f), ImGuiCond_Appearing);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.0f, 10.0f));
		bool modalOpen = ImGui::BeginPopupModal("Export Script", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoMove);
		ImGui::PopStyleVar();
		if (!modalOpen)
			return;
		//
		// Header
		//
		ImGui::Spacing();
		ImGui::Text(ICON_SCRIPT_TEXT "  Select the AngelScript file to use as export script:");
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		//
		// Reserve space for the action bar before sizing the table child
		//
		float actionBarH = ImGui::GetStyle().ItemSpacing.y * 2.0f + ImGui::GetStyle().SeparatorTextPadding.y + ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;
		float listH = ImGui::GetContentRegionAvail().y - actionBarH;
		if (m_pickerEntries.empty()) {
			//
			// Empty state: centred hint inside the bordered area
			//
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 8.0f));
			if (ImGui::BeginChild("ScriptListChild", ImVec2(-1.0f, listH), ImGuiChildFlags_Borders)) {
				ImGui::PopStyleVar();
				ImVec2 area = ImGui::GetContentRegionAvail();
				const char* hint1 = "No AngelScript files in the project.";
				const char* hint2 = "Add .as files to enable export scripts.";
				float textH = ImGui::GetTextLineHeightWithSpacing() * 2.0f;
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (area.y - textH) * 0.5f);
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (area.x - ImGui::CalcTextSize(hint1).x) * 0.5f);
				ImGui::TextDisabled("%s", hint1);
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (area.x - ImGui::CalcTextSize(hint2).x) * 0.5f);
				ImGui::TextDisabled("%s", hint2);
			} else {
				ImGui::PopStyleVar();
			}
			ImGui::EndChild();
		} else {
			//
			// Scrollable child containing a 3-column table: Script | Folder | Description
			//
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			if (ImGui::BeginChild("ScriptListChild", ImVec2(-1.0f, listH), ImGuiChildFlags_Borders)) {
				ImGui::PopStyleVar();
				ImGuiTableFlags tableFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp;
				if (ImGui::BeginTable("ScriptTable", 3, tableFlags)) {
					ImGui::TableSetupScrollFreeze(0, 1);
					ImGui::TableSetupColumn("Script", ImGuiTableColumnFlags_WidthStretch, 0.30f);
					ImGui::TableSetupColumn("Folder", ImGuiTableColumnFlags_WidthStretch, 0.20f);
					ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch, 0.50f);
					ImGui::TableHeadersRow();
					for (int i = 0; i < (int)m_pickerEntries.size(); i++) {
						const PickerEntry& e = m_pickerEntries[i];
						bool isSelected = (m_pickerSelectedIndex == i);
						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(0);
						ImGui::PushID(i);
						//
						// Full-row selectable anchored to the first column;
						// ImGuiSelectableFlags_SpanAllColumns makes it cover all three
						//
						std::string label = std::string(ICON_SCRIPT_TEXT_OUTLINE " ") + e.filename;
						if (ImGui::Selectable(label.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns, ImVec2(0.0f, 0.0f)))
							m_pickerSelectedIndex = i;
						bool doConfirm = ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
						ImGui::TableSetColumnIndex(1);
						ImGui::TextDisabled("%s", e.folder.c_str());
						ImGui::TableSetColumnIndex(2);
						ImGui::PushTextWrapPos(0.0f);
						ImGui::TextDisabled("%s", e.description.c_str());
						ImGui::PopTextWrapPos();
						ImGui::PopID();
						if (doConfirm) {
							if (m_exportParams) {
								m_exportParams->scriptPath = RetrodevLib::Project::CollapsePath(e.path);
								m_exportParams->scriptParams.clear();
								SyncParamDefs(e.params);
								RetrodevLib::Project::MarkAsModified();
							}
							m_pickerSelectedIndex = -1;
							m_pickerEntries.clear();
							ImGui::EndTable();
							ImGui::EndChild();
							ImGui::CloseCurrentPopup();
							ImGui::EndPopup();
							return;
						}
					}
					ImGui::EndTable();
				}
			} else {
				ImGui::PopStyleVar();
			}
			ImGui::EndChild();
		}
		//
		// Action bar
		//
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		bool canSelect = m_pickerSelectedIndex >= 0;
		if (!canSelect)
			ImGui::BeginDisabled();
		if (ImGui::Button(ICON_CHECK "  Select", ImVec2(110.0f, 0.0f))) {
			const PickerEntry& e = m_pickerEntries[m_pickerSelectedIndex];
			if (m_exportParams) {
				m_exportParams->scriptPath = RetrodevLib::Project::CollapsePath(e.path);
				m_exportParams->scriptParams.clear();
				SyncParamDefs(e.params);
				RetrodevLib::Project::MarkAsModified();
			}
			m_pickerSelectedIndex = -1;
			m_pickerEntries.clear();
			ImGui::CloseCurrentPopup();
		}
		if (!canSelect)
			ImGui::EndDisabled();
		ImGui::SameLine();
		if (ImGui::Button(ICON_CLOSE "  Cancel", ImVec2(110.0f, 0.0f))) {
			m_pickerSelectedIndex = -1;
			m_pickerEntries.clear();
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	void DataExportWidget::Render(RetrodevLib::ProjectBuildType buildType, const std::string& buildName, RetrodevLib::GFXParams* params,
								  std::shared_ptr<RetrodevLib::IBitmapConverter> converter, std::shared_ptr<RetrodevLib::ITileExtractor> tileExtractor,
								  RetrodevLib::TileExtractionParams* tileParams, std::shared_ptr<RetrodevLib::ISpriteExtractor> spriteExtractor,
								  RetrodevLib::SpriteExtractionParams* spriteParams) {
		//
		// Resolve ExportParams for the owning build item
		//
		m_exportParams = nullptr;
		switch (buildType) {
			case RetrodevLib::ProjectBuildType::Bitmap:
				RetrodevLib::Project::BitmapGetExportParams(buildName, &m_exportParams);
				break;
			case RetrodevLib::ProjectBuildType::Tilemap:
				RetrodevLib::Project::TilesetGetExportParams(buildName, &m_exportParams);
				break;
			case RetrodevLib::ProjectBuildType::Sprite:
				RetrodevLib::Project::SpriteGetExportParams(buildName, &m_exportParams);
				break;
			case RetrodevLib::ProjectBuildType::Map:
				RetrodevLib::Project::MapGetExportParams(buildName, &m_exportParams);
				break;
			case RetrodevLib::ProjectBuildType::Build:
				// TODO: Add error
				break;
			case RetrodevLib::ProjectBuildType::Palette:
				// TODO: Add error
				break;
			case RetrodevLib::ProjectBuildType::Raster:
				// TODO: Add error
				break;
			case RetrodevLib::ProjectBuildType::VirtualFolder:
				// TODO: Add error
				break;
		}
		//
		// Resolve filter identifiers used by the script picker
		//
		m_activeSystemId = (params != nullptr) ? RetrodevLib::Converters::GetSystemId(params->SParams.TargetSystem) : std::string();
		switch (buildType) {
			case RetrodevLib::ProjectBuildType::Bitmap:
				m_activeBuildTypeTag = "bitmap";
				break;
			case RetrodevLib::ProjectBuildType::Tilemap:
				m_activeBuildTypeTag = "tiles";
				break;
			case RetrodevLib::ProjectBuildType::Sprite:
				m_activeBuildTypeTag = "sprites";
				break;
			case RetrodevLib::ProjectBuildType::Map:
				m_activeBuildTypeTag = "map";
				break;
			default:
				m_activeBuildTypeTag = {};
				break;
		}
		//
		// Script picker modal must be driven unconditionally every frame
		//
		RenderScriptPicker();
		ImGui::PushID(this);
		ImGui::SetNextItemOpen(m_outputOpen, ImGuiCond_Once);
		if (!ImGui::CollapsingHeader("Export")) {
			m_outputOpen = false;
			ImGui::PopID();
			return;
		}
		m_outputOpen = true;
		if (m_exportParams == nullptr) {
			ImGui::TextDisabled("Export configuration unavailable.");
			ImGui::PopID();
			return;
		}
		//
		// Seed default output name on first open
		//
		if (m_exportParams->outputName.empty() && !buildName.empty())
			m_exportParams->outputName = buildName + ".bin";
		//
		// Script row: read-only display + picker button
		//
		float btnW = ImGui::CalcTextSize("Select...").x + ImGui::GetStyle().FramePadding.x * 2.0f;
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Script:");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - btnW - ImGui::GetStyle().ItemSpacing.x);
		std::string displayScript = !m_exportParams->scriptPath.empty() ? std::filesystem::path(m_exportParams->scriptPath).filename().string() : std::string();
		ImGui::InputText("##ScriptDisplay", displayScript.data(), displayScript.size() + 1, ImGuiInputTextFlags_ReadOnly);
		ImGui::SameLine();
		if (ImGui::Button("Select...##ScriptPick"))
			m_showScriptPicker = true;
		//
		// Description line and param def refresh when script path changes
		//
		if (!m_exportParams->scriptPath.empty()) {
			std::string absScript = RetrodevLib::Project::ExpandPath(m_exportParams->scriptPath);
			std::string desc = RetrodevLib::ExportEngine::GetScriptDescription(absScript);
			if (!desc.empty()) {
				float labelW = ImGui::CalcTextSize("Script: ").x;
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + labelW);
				ImGui::PushTextWrapPos(0.0f);
				ImGui::TextDisabled("%s", desc.c_str());
				ImGui::PopTextWrapPos();
			}
			if (m_paramDefsScript != m_exportParams->scriptPath) {
				RetrodevLib::ScriptMetadata meta = RetrodevLib::ExportEngine::GetScriptMetadata(absScript);
				m_paramDefs = meta.params;
				m_paramDefsScript = m_exportParams->scriptPath;
				SyncParamDefs(m_paramDefs);
			}
		} else {
			m_paramDefs.clear();
			m_paramDefsScript.clear();
		}
		RenderParamControls();
		//
		// Output path
		//
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Output:");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		char outputBuf[512];
		strncpy(outputBuf, m_exportParams->outputName.c_str(), sizeof(outputBuf) - 1);
		outputBuf[sizeof(outputBuf) - 1] = '\0';
		if (ImGui::InputText("##OutputPath", outputBuf, sizeof(outputBuf))) {
			m_exportParams->outputName = outputBuf;
			RetrodevLib::Project::MarkAsModified();
		}
		ImGui::Separator();
		//
		// Export button -- canExport conditions vary by type
		//
		bool canExport = !m_exportParams->scriptPath.empty() && !m_exportParams->outputName.empty() && converter != nullptr && params != nullptr;
		if (tileExtractor != nullptr)
			canExport = canExport && tileExtractor->GetTileCount() > 0;
		if (spriteExtractor != nullptr)
			canExport = canExport && spriteExtractor->GetSpriteCount() > 0;
		if (!canExport)
			ImGui::BeginDisabled();
		if (ImGui::Button("Export##RunExport", ImVec2(120.0f, 0.0f))) {
			std::string absoluteScript = RetrodevLib::Project::ExpandPath(m_exportParams->scriptPath);
			std::string absoluteOutput = RetrodevLib::Project::ExpandPath(m_exportParams->outputName);
			RetrodevLib::ExportEngine::Initialize();
			RetrodevLib::ExportEngine::ClearErrors();
			AppConsole::Clear(AppConsole::Channel::Script);
			bool ok = RunExport(params, converter, tileExtractor, tileParams, spriteExtractor, spriteParams, absoluteScript, absoluteOutput);
			if (ok) {
				AppConsole::AddLogF(AppConsole::LogLevel::Info, "Export completed: %s", absoluteOutput.c_str());
			}
		}
		if (!canExport)
			ImGui::EndDisabled();
		ImGui::Spacing();
		ImGui::PopID();
	}

	bool DataExportWidget::RunExport(RetrodevLib::GFXParams* params, std::shared_ptr<RetrodevLib::IBitmapConverter> converter,
									 std::shared_ptr<RetrodevLib::ITileExtractor> tileExtractor, RetrodevLib::TileExtractionParams* tileParams,
									 std::shared_ptr<RetrodevLib::ISpriteExtractor> spriteExtractor, RetrodevLib::SpriteExtractionParams* spriteParams,
									 const std::string& absoluteScript, const std::string& absoluteOutput) {
		if (tileExtractor != nullptr) {
			return RetrodevLib::ExportEngine::ExportTileset(absoluteScript, absoluteOutput, m_exportParams->scriptParams, converter.get(), params, tileExtractor.get(), tileParams);
		}
		if (spriteExtractor != nullptr && spriteExtractor->GetSpriteCount() > 0) {
			return RetrodevLib::ExportEngine::ExportSprites(absoluteScript, absoluteOutput, m_exportParams->scriptParams, converter.get(), params, spriteExtractor.get(),
															spriteParams);
		}
		//
		// Bitmap / sprite / map path -- needs the converted image
		//
		auto img = converter->GetConverted(params);
		if (!img) {
			AppConsole::AddLog(AppConsole::LogLevel::Error, "No converted image -- run conversion before exporting.");
			return false;
		}
		return RetrodevLib::ExportEngine::ExportBitmap(absoluteScript, absoluteOutput, m_exportParams->scriptParams, img.get(), converter.get(), params);
	}

	//
	// Ensure every key declared by the script has a value in scriptParams.
	// Inserts "key=default" for any key that is absent; preserves existing values.
	//
	void DataExportWidget::SyncParamDefs(const std::vector<RetrodevLib::ScriptParamDef>& defs) {
		if (m_exportParams == nullptr)
			return;
		bool changed = false;
		for (const auto& def : defs) {
			//
			// Check whether the key already has a value in the serialised string
			//
			bool found = false;
			const std::string& s = m_exportParams->scriptParams;
			std::size_t pos = 0;
			while (pos < s.size()) {
				std::size_t semi = s.find(';', pos);
				if (semi == std::string::npos)
					semi = s.size();
				std::size_t eq = s.find('=', pos);
				if (eq != std::string::npos && eq < semi && s.substr(pos, eq - pos) == def.key) {
					found = true;
					break;
				}
				pos = semi + 1;
			}
			if (!found) {
				if (!m_exportParams->scriptParams.empty())
					m_exportParams->scriptParams += ';';
				m_exportParams->scriptParams += def.key + '=' + def.defaultValue;
				changed = true;
			}
		}
		if (changed)
			RetrodevLib::Project::MarkAsModified();
	}
	//
	// Read a single value from the serialised scriptParams string
	//
	static std::string GetParamValue(const std::string& scriptParams, const std::string& key) {
		std::size_t pos = 0;
		while (pos < scriptParams.size()) {
			std::size_t semi = scriptParams.find(';', pos);
			if (semi == std::string::npos)
				semi = scriptParams.size();
			std::size_t eq = scriptParams.find('=', pos);
			if (eq != std::string::npos && eq < semi && scriptParams.substr(pos, eq - pos) == key)
				return scriptParams.substr(eq + 1, semi - eq - 1);
			pos = semi + 1;
		}
		return std::string();
	}
	//
	// Write a single value into the serialised scriptParams string, replacing in-place
	//
	static void SetParamValue(std::string& scriptParams, const std::string& key, const std::string& value) {
		std::size_t pos = 0;
		while (pos < scriptParams.size()) {
			std::size_t semi = scriptParams.find(';', pos);
			if (semi == std::string::npos)
				semi = scriptParams.size();
			std::size_t eq = scriptParams.find('=', pos);
			if (eq != std::string::npos && eq < semi && scriptParams.substr(pos, eq - pos) == key) {
				scriptParams.replace(eq + 1, semi - eq - 1, value);
				return;
			}
			pos = semi + 1;
		}
		//
		// Key not found -- append it
		//
		if (!scriptParams.empty())
			scriptParams += ';';
		scriptParams += key + '=' + value;
	}
	//
	// Render one ImGui control per @param definition and write changes back
	// into m_exportParams->scriptParams immediately.
	//
	void DataExportWidget::RenderParamControls() {
		if (m_exportParams == nullptr || m_paramDefs.empty())
			return;
		for (const auto& def : m_paramDefs) {
			std::string value = GetParamValue(m_exportParams->scriptParams, def.key);
			std::string id = "##param_" + def.key;
			if (def.type == "bool") {
				bool checked = (value == "true");
				if (ImGui::Checkbox((def.label.empty() ? def.key : def.label).c_str(), &checked)) {
					SetParamValue(m_exportParams->scriptParams, def.key, checked ? "true" : "false");
					RetrodevLib::Project::MarkAsModified();
				}
			} else if (def.type == "combo") {
				ImGui::AlignTextToFramePadding();
				ImGui::Text("%s:", (def.label.empty() ? def.key : def.label).c_str());
				ImGui::SameLine();
				//
				// Find the index of the current value in the options list;
				// fall back to 0 if the stored value is not recognised.
				//
				int current = 0;
				for (int oi = 0; oi < (int)def.options.size(); oi++) {
					if (def.options[oi] == value) {
						current = oi;
						break;
					}
				}
				//
				// Build a null-separated, double-null-terminated items string for ImGui
				//
				std::string items;
				for (const auto& opt : def.options) {
					items += opt;
					items += '\0';
				}
				items += '\0';
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
				if (ImGui::Combo(id.c_str(), &current, items.c_str())) {
					SetParamValue(m_exportParams->scriptParams, def.key, def.options[current]);
					RetrodevLib::Project::MarkAsModified();
				}
			} else if (def.type == "int") {
				int v = 0;
				//
				// Parse int without exceptions -- iterate digits manually
				//
				const char* p = value.c_str();
				bool neg = (*p == '-');
				if (neg)
					p++;
				while (*p >= '0' && *p <= '9') {
					v = v * 10 + (*p++ - '0');
				}
				if (neg)
					v = -v;
				ImGui::AlignTextToFramePadding();
				ImGui::Text("%s:", (def.label.empty() ? def.key : def.label).c_str());
				ImGui::SameLine();
				ImGui::SetNextItemWidth(80.0f);
				if (ImGui::InputInt(id.c_str(), &v)) {
					SetParamValue(m_exportParams->scriptParams, def.key, std::to_string(v));
					RetrodevLib::Project::MarkAsModified();
				}
			} else {
				//
				// string or unknown type -- show a single-line text field
				//
				ImGui::AlignTextToFramePadding();
				ImGui::Text("%s:", (def.label.empty() ? def.key : def.label).c_str());
				ImGui::SameLine();
				char buf[256];
				strncpy(buf, value.c_str(), sizeof(buf) - 1);
				buf[sizeof(buf) - 1] = '\0';
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
				if (ImGui::InputText(id.c_str(), buf, sizeof(buf))) {
					SetParamValue(m_exportParams->scriptParams, def.key, buf);
					RetrodevLib::Project::MarkAsModified();
				}
			}
		}
	}
	//
	// Map-specific export render: no converter or GFXParams needed
	//
	void DataExportWidget::RenderMap(const std::string& buildName, const RetrodevLib::MapParams* mapParams) {
		//
		// Resolve ExportParams for this map build item
		//
		m_exportParams = nullptr;
		RetrodevLib::Project::MapGetExportParams(buildName, &m_exportParams);
		//
		// Maps have no hardware target -- clear system filter; set type tag for script picker
		//
		m_activeSystemId.clear();
		m_activeBuildTypeTag = "map";
		//
		// Script picker modal must be driven unconditionally every frame
		//
		RenderScriptPicker();
		ImGui::PushID(this);
		ImGui::SetNextItemOpen(m_outputOpen, ImGuiCond_Once);
		if (!ImGui::CollapsingHeader("Export")) {
			m_outputOpen = false;
			ImGui::PopID();
			return;
		}
		m_outputOpen = true;
		if (m_exportParams == nullptr) {
			ImGui::TextDisabled("Export configuration unavailable.");
			ImGui::PopID();
			return;
		}
		//
		// Seed default output name on first open
		//
		if (m_exportParams->outputName.empty() && !buildName.empty())
			m_exportParams->outputName = buildName + ".bin";
		//
		// Script row: read-only display + picker button
		//
		float btnW = ImGui::CalcTextSize("Select...").x + ImGui::GetStyle().FramePadding.x * 2.0f;
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Script:");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - btnW - ImGui::GetStyle().ItemSpacing.x);
		std::string displayScript = !m_exportParams->scriptPath.empty() ? std::filesystem::path(m_exportParams->scriptPath).filename().string() : std::string();
		ImGui::InputText("##ScriptDisplay", displayScript.data(), displayScript.size() + 1, ImGuiInputTextFlags_ReadOnly);
		ImGui::SameLine();
		if (ImGui::Button("Select...##ScriptPick"))
			m_showScriptPicker = true;
		//
		// Description line and param def refresh when script path changes
		//
		if (!m_exportParams->scriptPath.empty()) {
			std::string absScript = RetrodevLib::Project::ExpandPath(m_exportParams->scriptPath);
			std::string desc = RetrodevLib::ExportEngine::GetScriptDescription(absScript);
			if (!desc.empty()) {
				float labelW = ImGui::CalcTextSize("Script: ").x;
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + labelW);
				ImGui::PushTextWrapPos(0.0f);
				ImGui::TextDisabled("%s", desc.c_str());
				ImGui::PopTextWrapPos();
			}
			if (m_paramDefsScript != m_exportParams->scriptPath) {
				RetrodevLib::ScriptMetadata meta = RetrodevLib::ExportEngine::GetScriptMetadata(absScript);
				m_paramDefs = meta.params;
				m_paramDefsScript = m_exportParams->scriptPath;
				SyncParamDefs(m_paramDefs);
			}
		} else {
			m_paramDefs.clear();
			m_paramDefsScript.clear();
		}
		RenderParamControls();
		//
		// Output path
		//
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Output:");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		char outputBuf[512];
		strncpy(outputBuf, m_exportParams->outputName.c_str(), sizeof(outputBuf) - 1);
		outputBuf[sizeof(outputBuf) - 1] = '\0';
		if (ImGui::InputText("##OutputPath", outputBuf, sizeof(outputBuf))) {
			m_exportParams->outputName = outputBuf;
			RetrodevLib::Project::MarkAsModified();
		}
		ImGui::Separator();
		//
		// Export button -- enabled when a script, output name and map params are present
		//
		bool canExport = !m_exportParams->scriptPath.empty() && !m_exportParams->outputName.empty() && mapParams != nullptr && !mapParams->layers.empty();
		if (!canExport)
			ImGui::BeginDisabled();
		if (ImGui::Button("Export##RunExport", ImVec2(120.0f, 0.0f))) {
			std::string absoluteScript = RetrodevLib::Project::ExpandPath(m_exportParams->scriptPath);
			std::string absoluteOutput = RetrodevLib::Project::ExpandPath(m_exportParams->outputName);
			RetrodevLib::ExportEngine::Initialize();
			RetrodevLib::ExportEngine::ClearErrors();
			AppConsole::Clear(AppConsole::Channel::Script);
			//
			// mapParams always holds compact indices (lib invariant) -- pass directly to exporter
			//
			bool ok = RetrodevLib::ExportEngine::ExportMap(absoluteScript, absoluteOutput, m_exportParams->scriptParams, mapParams);
			if (ok) {
				AppConsole::AddLogF(AppConsole::LogLevel::Info, "Export completed: %s", absoluteOutput.c_str());
			}
		}
		if (!canExport)
			ImGui::EndDisabled();
		ImGui::Spacing();
		ImGui::PopID();
	}

}
