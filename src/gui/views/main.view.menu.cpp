// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Main view -- menu bar.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#include "main.view.menu.h"
#include "main.view.documents.h"
#include <app/app.icons.mdi.h>
#include <app/app.console.h>
#include <app/version.check/version.check.h>
#include <app/version.check/version.check.ui.h>
#include <assets/source/source.emulator.h>
#include <views/build/document.build.settings.h>
#include <dialogs/dialog.error.h>
#include <dialogs/dialog.about.h>
#include <dialogs/dialog.help.h>
#include <dialogs/dialog.confirm.h>
#include <imgui_internal.h>
#include <filesystem>

namespace RetrodevGui {

	//
	// Last directory used in a project open/save dialog -- persisted in retrodev.ini
	//
	static std::string s_lastProjectDir;

	//
	// Prepare a step-based build for buildItemName.
	// Saves all open documents, clears the Build console, sets the project CWD,
	// then calls Project::BuildPrepare to flatten the dependency tree into work steps.
	//
	void MainViewMenu::StartBuild(const std::string& buildItemName, bool debugAfter) {
		//
		// Save any unsaved project/document changes before building
		//
		DocumentsView::SaveAllModified();
		if (RetrodevLib::Project::IsModified()) {
			std::string currentProjectPath = RetrodevLib::Project::GetPath();
			if (RetrodevLib::Project::Save(currentProjectPath)) {
				RetrodevLib::Project::ClearModified();
				RetrodevGui::DocumentsView::ClearAllModifiedFlags();
			}
		}
		AppConsole::Clear(AppConsole::Channel::Build);
		//
		// Set CWD to the project folder so build tools resolve relative paths correctly.
		// Restored in Perform() when BuildStep reports completion.
		//
		m_prevCwd = std::filesystem::current_path();
		std::filesystem::path projectDir = std::filesystem::path(RetrodevLib::Project::GetPath()).parent_path();
		std::filesystem::current_path(projectDir);
		//
		// Mirror script log output to the Build channel for the duration of the build
		//
		AppConsole::SetScriptMirrorToBuild(true);
		if (!RetrodevLib::Project::BuildPrepare(buildItemName)) {
			AppConsole::SetScriptMirrorToBuild(false);
			std::filesystem::current_path(m_prevCwd);
			return;
		}
		m_buildRunning = true;
		m_buildItemName = buildItemName;
		m_debugAfterBuild = debugAfter;
	}
	//
	// Launch the emulator for the given build item, injecting machine-local exe paths.
	//
	static void LaunchEmulator(const std::string& buildItemName) {
		RetrodevLib::SourceParams* dbgParams = nullptr;
		if (!RetrodevLib::Project::BuildGetParams(buildItemName, &dbgParams) || dbgParams == nullptr)
			return;
		RetrodevLib::SourceParams::EmulatorParams& ep = dbgParams->emulatorParams;
		ep.winape.exePath = EmulatorSettings::GetExePath("WinAPE");
		ep.rvm.exePath = EmulatorSettings::GetExePath("RVM");
		ep.acedl.exePath = EmulatorSettings::GetExePath("ACE-DL");
		RetrodevLib::SourceEmulator::Launch(dbgParams);
	}

	//
	// INI settings handler -- persists the last directory used in a project file dialog
	//
	static void* MenuHandler_ReadOpen(ImGuiContext*, ImGuiSettingsHandler*, const char*) {
		return (void*)1;
	}
	static void MenuHandler_ReadLine(ImGuiContext*, ImGuiSettingsHandler*, void*, const char* line) {
		char buf[1024];
		if (sscanf(line, "LastProjectDir=%1023[^\n]", buf) == 1)
			s_lastProjectDir = buf;
	}
	static void MenuHandler_WriteAll(ImGuiContext*, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf) {
		buf->appendf("[%s][MainViewMenu]\n", handler->TypeName);
		buf->appendf("LastProjectDir=%s\n", s_lastProjectDir.c_str());
		buf->append("\n");
	}

	void MainViewMenu::RegisterSettingsHandler() {
		ImGuiSettingsHandler handler;
		handler.TypeName = "MenuState";
		handler.TypeHash = ImHashStr("MenuState");
		handler.ReadOpenFn = MenuHandler_ReadOpen;
		handler.ReadLineFn = MenuHandler_ReadLine;
		handler.WriteAllFn = MenuHandler_WriteAll;
		ImGui::AddSettingsHandler(&handler);
	}

	void MainViewMenu::RequestQuit() {
		bool projectModified = RetrodevLib::Project::IsOpen() && RetrodevLib::Project::IsModified();
		bool docsModified = DocumentsView::HasAnyModifiedDocuments();
		if (projectModified || docsModified) {
			ConfirmDialog::Show("There are unsaved changes. Quit anyway?", []() { m_quitPending = true; });
		} else {
			m_quitPending = true;
		}
	}
	//
	// Returns true when the run loop should exit
	//
	bool MainViewMenu::ShouldQuit() {
		return m_quitPending;
	}
	void MainViewMenu::PerformBuildToolbar() {
		const bool projectOpen = RetrodevLib::Project::IsOpen();
		const bool isModified = projectOpen && RetrodevLib::Project::IsModified();
		//
		// Refresh the build item list every frame and validate the stored selection
		//
		std::vector<std::string> buildItems;
		if (projectOpen)
			buildItems = RetrodevLib::Project::GetBuildItemsByType(RetrodevLib::ProjectBuildType::Build);
		std::string selectedItem = projectOpen ? RetrodevLib::Project::GetSelectedBuildItem() : std::string{};
		//
		// If the stored selection no longer exists in the list, fall back to the first item (or clear)
		//
		bool selectionValid = false;
		for (const auto& item : buildItems) {
			if (item == selectedItem) {
				selectionValid = true;
				break;
			}
		}
		if (!selectionValid) {
			selectedItem = buildItems.empty() ? std::string{} : buildItems[0];
			if (projectOpen)
				RetrodevLib::Project::SetSelectedBuildItem(selectedItem);
		}
		const bool canBuild = projectOpen && !selectedItem.empty() && !m_buildRunning;
		//
		// Measure right-side block width so we can right-justify it
		//
		float itemSpacing = ImGui::GetStyle().ItemSpacing.x;
		float frameH = ImGui::GetFrameHeight();
		float comboWidth = ImGui::GetFontSize() * 14.0f;
		float buildBtnWidth = ImGui::CalcTextSize(ICON_HAMMER " Build").x + ImGui::GetStyle().FramePadding.x * 2.0f;
		float debugBtnWidth = ImGui::CalcTextSize(ICON_BUG_PLAY " Debug").x + ImGui::GetStyle().FramePadding.x * 2.0f;
		float saveBtnWidth = ImGui::CalcTextSize(ICON_CONTENT_SAVE_ALERT).x + ImGui::GetStyle().FramePadding.x * 2.0f;
		//
		// Right block: [save] [combo] [Build] [Debug]  (save only when modified)
		//
		float rightWidth = comboWidth + itemSpacing + buildBtnWidth + itemSpacing + debugBtnWidth;
		if (isModified)
			rightWidth += saveBtnWidth + itemSpacing;
		//
		// Push cursor to align the block against the right edge of the menu bar
		//
		float cursorX = ImGui::GetContentRegionMax().x - rightWidth;
		if (cursorX > ImGui::GetCursorPosX())
			ImGui::SetCursorPosX(cursorX);
		//
		// Save button -- only rendered when there are unsaved changes
		//
		if (isModified) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.45f, 0.10f, 1.00f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.90f, 0.58f, 0.15f, 1.00f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.60f, 0.35f, 0.05f, 1.00f));
			if (ImGui::Button(ICON_CONTENT_SAVE_ALERT)) {
				DocumentsView::SaveAllModified();
				std::string currentProjectPath = RetrodevLib::Project::GetPath();
				if (RetrodevLib::Project::Save(currentProjectPath)) {
					RetrodevLib::Project::ClearModified();
					DocumentsView::ClearAllModifiedFlags();
				}
			}
			ImGui::PopStyleColor(3);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Unsaved changes -- click to save project");
			ImGui::SameLine();
		}
		//
		// Build item combo
		//
		ImGui::BeginDisabled(!projectOpen);
		ImGui::SetNextItemWidth(comboWidth);
		const char* comboPreview = selectedItem.empty() ? "(none)" : selectedItem.c_str();
		if (ImGui::BeginCombo("##BuildItemCombo", comboPreview, ImGuiComboFlags_None)) {
			for (int i = 0; i < (int)buildItems.size(); i++) {
				bool selected = (buildItems[i] == selectedItem);
				if (ImGui::Selectable(buildItems[i].c_str(), selected))
					RetrodevLib::Project::SetSelectedBuildItem(buildItems[i]);
				if (selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::BeginDisabled(!canBuild);
		if (ImGui::Button(ICON_HAMMER " Build"))
			StartBuild(selectedItem, false);
		ImGui::SameLine();
		if (ImGui::Button(ICON_BUG_PLAY " Debug"))
			StartBuild(selectedItem, true);
		ImGui::EndDisabled();
		//
		// Spinner shown to the right of the disabled buttons while a build is running
		//
		if (m_buildRunning) {
			ImGui::SameLine();
			const char* const kSpinFrames[] = {"|", "/", "-", "\\"};
			int spinIdx = (int)(ImGui::GetTime() * 6.0) % 4;
			ImGui::TextDisabled("%s", kSpinFrames[spinIdx]);
		}
		//
		// Suppress unused-variable warning for frameH (used as guard for minimum row height)
		//
		(void)frameH;
	}
	//
	// Returns true if application should quit, false otherwise
	//
	bool MainViewMenu::Perform() {
		if (ImGui::BeginMainMenuBar()) {
			ImGui::Dummy(ImVec2(10.0f, 0.0f));
			ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::GetStyleColorVec4(ImGuiCol_MenuBarBg));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem(ICON_FILE " New Project", "Ctrl+N")) {
					ImGui::FileDialog::Instance().Save("NewProjectDialog", "New Project", "RetroDev Project (*.retrodev){.retrodev}", s_lastProjectDir);
				}
				if (ImGui::MenuItem(ICON_FOLDER_OPEN " Open Project", "Ctrl+O")) {
					ImGui::FileDialog::Instance().Open("OpenProjectDialog", "Open Project", "RetroDev Project (*.retrodev){.retrodev}", false, s_lastProjectDir);
				}
				ImGui::BeginDisabled(!RetrodevLib::Project::IsOpen());
				if (ImGui::MenuItem(ICON_CONTENT_SAVE " Save Project", "Ctrl+S")) {
					//
					// Save all open documents first, then the project
					//
					DocumentsView::SaveAllModified();
					std::string currentProjectPath = RetrodevLib::Project::GetPath();
					if (RetrodevLib::Project::Save(currentProjectPath)) {
						RetrodevLib::Project::ClearModified();
						DocumentsView::ClearAllModifiedFlags();
					}
				}
				ImGui::EndDisabled();
				ImGui::BeginDisabled(!RetrodevLib::Project::IsOpen());
				if (ImGui::MenuItem(ICON_FOLDER_REMOVE " Close Project")) {
					m_closeProjectPending = true;
				}
				ImGui::EndDisabled();
				if (ImGui::MenuItem(ICON_POWER " Quit")) {
					RequestQuit();
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Help")) {
					if (ImGui::MenuItem(ICON_BOOK_OPEN_VARIANT " Documentation"))
						HelpDialog::Show();
					ImGui::Separator();
					//
					// Update badge -- shown next to the menu item when a new version is available
					//
					if (VersionCheckUi::HasPendingNotification()) {
						if (ImGui::MenuItem(ICON_UPDATE " Check for Updates  " ICON_CIRCLE_MEDIUM))
							VersionCheckUi::ShowPopup();
					} else {
						if (ImGui::MenuItem(ICON_UPDATE " Check for Updates"))
							VersionCheckUi::ShowPopup();
					}
					ImGui::Separator();
					if (ImGui::MenuItem(ICON_INFORMATION " About RetroDev"))
						AboutDialog::Show();
					ImGui::EndMenu();
				}
			ImGui::PopStyleVar();
			ImGui::PopStyleColor();
			//
			// Build toolbar: combo to select a build item, Build and Debug buttons
			// Rendered at the right edge of the menu bar, enabled only when a project is open
			//
			PerformBuildToolbar();
			ImGui::EndMainMenuBar();
		}
		//
		// Poll background build: drain queued log messages and check for completion
		//
		if (m_buildRunning) {
			RetrodevLib::Project::BuildStepStatus status = RetrodevLib::Project::BuildStep();
			if (status != RetrodevLib::Project::BuildStepStatus::Running) {
				AppConsole::SetScriptMirrorToBuild(false);
				std::filesystem::current_path(m_prevCwd);
				RetrodevLib::Project::ClearModified();
				RetrodevGui::DocumentsView::ClearAllModifiedFlags();
				m_buildRunning = false;
				if (status == RetrodevLib::Project::BuildStepStatus::Succeeded && m_debugAfterBuild)
					LaunchEmulator(m_buildItemName);
				m_debugAfterBuild = false;
			}
		}
		//
		// F5: build the currently selected build item, then launch if successful
		//
		if (ImGui::IsKeyPressed(ImGuiKey_F5, false)) {
			std::string selected = RetrodevLib::Project::GetSelectedBuildItem();
			if (!selected.empty() && !m_buildRunning)
				StartBuild(selected, true);
		}
		// Handle file dialog results
		if (ImGui::FileDialog::Instance().IsDone("NewProjectDialog")) {
			if (ImGui::FileDialog::Instance().HasResult()) {
				std::string projectPath = ImGui::FileDialog::Instance().GetResult().string();
				s_lastProjectDir = std::filesystem::path(projectPath).parent_path().string();
				//
				// Helper that performs the actual New, optionally asking to overwrite an existing file
				//
				auto doNew = [projectPath]() {
					if (std::filesystem::exists(projectPath)) {
						ConfirmDialog::Show("A project already exists at this location.\nOverwrite it?", [projectPath]() {
							if (!RetrodevLib::Project::New(projectPath))
								ErrorDialog::Show("Failed to create project:\n" + projectPath);
							else
								DocumentsView::CloseAllDocuments();
						});
					} else {
						if (!RetrodevLib::Project::New(projectPath))
							ErrorDialog::Show("Failed to create project:\n" + projectPath);
						else
							DocumentsView::CloseAllDocuments();
					}
				};
				//
				// If a project is already open, confirm closing it first
				//
				if (RetrodevLib::Project::IsOpen()) {
					std::string closeMsg = RetrodevLib::Project::IsModified() ? "The current project has unsaved changes.\nClose it and create a new project?"
																			  : "Close the current project and create a new one?";
					ConfirmDialog::Show(closeMsg, [doNew]() {
						RetrodevLib::Project::Close();
						DocumentsView::CloseAllDocuments();
						doNew();
					});
				} else {
					doNew();
				}
			}
			ImGui::FileDialog::Instance().Close();
		}
		if (ImGui::FileDialog::Instance().IsDone("OpenProjectDialog")) {
			if (ImGui::FileDialog::Instance().HasResult()) {
				std::string projectPath = ImGui::FileDialog::Instance().GetResult().string();
				s_lastProjectDir = std::filesystem::path(projectPath).parent_path().string();
				//
					// Helper that performs the actual Open
					//
					auto doOpen = [projectPath]() {
						RetrodevLib::Project::OpenResult openResult = RetrodevLib::Project::Open(projectPath);
						if (openResult == RetrodevLib::Project::OpenResult::Ok) {
							DocumentsView::CloseAllDocuments();
						} else if (openResult == RetrodevLib::Project::OpenResult::UnsupportedVersion) {
							ErrorDialog::Show(
								"The project file format is outdated or unsupported and cannot be loaded:\n" + projectPath +
								"\n\nThis project was created with an incompatible version of RetroDev.\nPlease recreate the project.");
						} else {
							ErrorDialog::Show("Failed to open project:\n" + projectPath);
						}
					};
				//
				// If a project is already open, confirm closing it first
				//
				if (RetrodevLib::Project::IsOpen()) {
					std::string closeMsg = RetrodevLib::Project::IsModified() ? "The current project has unsaved changes.\nClose it and open another project?"
																			  : "Close the current project and open another one?";
					ConfirmDialog::Show(closeMsg, [doOpen]() {
						RetrodevLib::Project::Close();
						DocumentsView::CloseAllDocuments();
						doOpen();
					});
				} else {
					doOpen();
				}
			}
			ImGui::FileDialog::Instance().Close();
		}

		ErrorDialog::Render();
		AboutDialog::Render();
		HelpDialog::Render();
		ConfirmDialog::Render();
		//
		// Close Project confirmation modal: opened the frame after the menu item is clicked
		//
		if (m_closeProjectPending) {
			ImGui::OpenPopup("Close Project##Modal");
			m_closeProjectPending = false;
		}
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 12.0f));
		ImVec2 closeCenter = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(closeCenter, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		if (ImGui::BeginPopupModal("Close Project##Modal", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("Are you sure you want to close the project?");
			ImGui::Separator();
			if (ImGui::Button(ICON_CHECK " Close", ImVec2(120.0f, 0.0f))) {
				RetrodevLib::Project::Close();
				DocumentsView::CloseAllDocuments();
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button(ICON_CLOSE " Cancel", ImVec2(120.0f, 0.0f)))
				ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}
		ImGui::PopStyleVar();

		return m_quitPending;
	}

}