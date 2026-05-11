// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Console output panel -- captures and displays build/script/find log messages.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#include "app.console.h"
#include "app.h"
#include <views/text/document.text.h>
#include <cstdarg>
#include <filesystem>

namespace RetrodevGui {

	// Per-channel entry lists, active tab state, display options and reveal-pending flags
	std::vector<AppConsole::LogEntry> AppConsole::m_channels[4];
	AppConsole::Channel AppConsole::m_activeChannel = AppConsole::Channel::Output;
	bool AppConsole::m_autoScroll = true;
	AppConsole::LogLevel AppConsole::m_filterLevel = AppConsole::LogLevel::Info;
	bool AppConsole::m_revealPending = false;
	AppConsole::Channel AppConsole::m_revealChannel = AppConsole::Channel::Output;
	bool AppConsole::m_pendingChannelSwitch = false;
	bool AppConsole::m_autoHide = true;
	bool AppConsole::m_scriptMirrorToBuild = false;

	void AppConsole::Render() {
		// Clear button
		if (ImGui::Button("Clear"))
			Clear(m_activeChannel);
		ImGui::SameLine();
		// Auto-scroll checkbox
		ImGui::Checkbox("Auto-scroll", &m_autoScroll);
		ImGui::SameLine();
		// Auto-hide checkbox -- collapses the panel when clicking outside it
		ImGui::Checkbox("Auto-hide", &m_autoHide);
		ImGui::SameLine();
		// Filter level combo -- shows messages at the selected level and above
		const char* kLevelNames[] = {"Info", "Warning", "Error"};
		int currentLevel = static_cast<int>(m_filterLevel);
		ImGui::SetNextItemWidth(90.0f);
		if (ImGui::Combo("Level", &currentLevel, kLevelNames, 3))
			m_filterLevel = static_cast<LogLevel>(currentLevel);
		ImGui::Separator();
		// Reserve space at the bottom for the tab bar before rendering content
		// Tab bar height = one frame height + tab bar border
		const float tabBarHeight = ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y + ImGui::GetStyle().TabBarBorderSize;
		// Log content area -- shrunk vertically to leave room for the tab bar below
		RenderChannel(m_activeChannel);
		// Tab bar sits below the content area
		const char* kTabNames[] = {"Output", "Find", "Script", "Build"};
		ImGui::SetCursorPosY(ImGui::GetWindowHeight() - tabBarHeight);
		if (ImGui::BeginTabBar("ConsoleTabs", ImGuiTabBarFlags_NoTooltip)) {
			for (int i = 0; i < 4; i++) {
				//
				// Always pass SetSelected for the active tab so ImGui's internal state
				// stays in sync with m_activeChannel, even when the tab bar is newly created
				//
				ImGuiTabItemFlags flags = (static_cast<Channel>(i) == m_activeChannel) ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
				if (ImGui::BeginTabItem(kTabNames[i], nullptr, flags)) {
					ImGui::EndTabItem();
				}
				//
				// Only update m_activeChannel on an explicit user click, detected as
				// the item being hovered and mouse released -- not on programmatic selection
				//
				if (ImGui::IsItemActivated())
					m_activeChannel = static_cast<Channel>(i);
			}
			m_pendingChannelSwitch = false;
			ImGui::EndTabBar();
		}
	}

	//
	// Parse a log line for a [filepath:line] token and open that file at that line.
	// Skips sentinel tokens like <unknown> and plain labels like [Script] / [RASM].
	// Project-relative paths are resolved to absolute using the project folder.
	// The first valid navigable token wins.
	//
	static void NavigateToLogEntry(const std::string& message) {
		std::string projectFolder = RetrodevLib::Project::GetProjectFolder();
		std::size_t start = 0;
		while (start < message.size()) {
			std::size_t lb = message.find('[', start);
			if (lb == std::string::npos)
				break;
			std::size_t rb = message.find(']', lb + 1);
			if (rb == std::string::npos)
				break;
			std::string token = message.substr(lb + 1, rb - lb - 1);
			//
			// Skip sentinel tokens (<unknown>) and plain source tags ([Script], [RASM])
			//
			if (!token.empty() && token[0] != '<') {
				std::size_t colon = token.rfind(':');
				if (colon != std::string::npos && colon > 0 && colon + 1 < token.size()) {
					bool allDigits = true;
					for (std::size_t k = colon + 1; k < token.size(); k++)
						if (token[k] < '0' || token[k] > '9') {
							allDigits = false;
							break;
						}
					if (allDigits) {
						int line = 0;
						for (std::size_t k = colon + 1; k < token.size(); k++)
							line = line * 10 + (token[k] - '0');
						if (line > 0) {
							std::string filepath = token.substr(0, colon);
							std::string absPath = projectFolder.empty() ? filepath : (std::filesystem::path(projectFolder) / filepath).string();
							if (std::filesystem::exists(absPath)) {
								std::error_code ec;
								std::filesystem::path canonical = std::filesystem::canonical(absPath, ec);
								if (!ec)
									absPath = canonical.string();
								DocumentText::OpenAtLine(absPath, line);
								return;
							}
						}
					}
				}
			}
			start = rb + 1;
		}
	}

	void AppConsole::RenderChannel(Channel channel) {
		// Height leaves room for the tab bar that will be drawn below this child
		const float tabBarHeight = ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.y + ImGui::GetStyle().TabBarBorderSize;
		const float childHeight = ImGui::GetContentRegionAvail().y - tabBarHeight;
		// Scrollable log display area with horizontal overflow support
		ImFont* consoleFont = Application::EditorFont != nullptr ? Application::EditorFont : ImGui::GetFont();
		consoleFont->Scale = 0.85f;
		if (ImGui::BeginChild("LogScrollArea", ImVec2(0, childHeight), false, ImGuiWindowFlags_HorizontalScrollbar)) {
			ImGui::PushFont(consoleFont);
			// Render log entries at or above the selected filter level
			for (const auto& entry : m_channels[static_cast<int>(channel)]) {
				if (static_cast<int>(entry.level) < static_cast<int>(m_filterLevel))
					continue;
				// Color-code text by severity and render without any text wrapping
				ImVec4 color = GetColorForLevel(entry.level);
				ImGui::PushStyleColor(ImGuiCol_Text, color);
				ImGui::TextUnformatted(entry.message.c_str());
				ImGui::PopStyleColor();
				// Double-click navigates to the source file and line embedded in the message
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
					NavigateToLogEntry(entry.message);
			}
			ImGui::PopFont();
			consoleFont->Scale = 1.0f;
			// Auto-scroll to bottom when new messages arrive
			if (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
				ImGui::SetScrollHereY(1.0f);
		}
		ImGui::EndChild();
	}

	bool AppConsole::GetAutoHide() {
		return m_autoHide;
	}
	void AppConsole::SetAutoHide(bool value) {
		m_autoHide = value;
	}

	bool AppConsole::ShouldReveal(Channel channel, LogLevel level) {
		// Warnings and errors always reveal, regardless of channel
		// Non-output channels (Find, Script, Build) always reveal on any message
		return level >= LogLevel::Warning || channel == Channel::Script || channel == Channel::Find || channel == Channel::Build;
	}

	void AppConsole::AddLog(LogLevel level, const char* message) {
		// Shorthand overload -- always targets the Output channel
		m_channels[static_cast<int>(Channel::Output)].emplace_back(message, level);
		if (ShouldReveal(Channel::Output, level)) {
			m_revealPending = true;
			m_revealChannel = Channel::Output;
		}
	}

	void AppConsole::AddLogF(LogLevel level, const char* format, ...) {
		// Shorthand formatted overload -- always targets the Output channel
		char buffer[4096];
		va_list args;
		va_start(args, format);
		vsnprintf(buffer, sizeof(buffer), format, args);
		va_end(args);
		buffer[sizeof(buffer) - 1] = '\0';
		m_channels[static_cast<int>(Channel::Output)].emplace_back(buffer, level);
		if (ShouldReveal(Channel::Output, level)) {
			m_revealPending = true;
			m_revealChannel = Channel::Output;
		}
	}

	void AppConsole::AddLog(Channel channel, LogLevel level, const char* message) {
		// Full overload -- targets the specified channel
		m_channels[static_cast<int>(channel)].emplace_back(message, level);
		//
		// During a build/debug operation, mirror script messages to the Build channel
		//
		if (channel == Channel::Script && m_scriptMirrorToBuild)
			m_channels[static_cast<int>(Channel::Build)].emplace_back(message, level);
		if (ShouldReveal(channel, level)) {
			m_revealPending = true;
			m_revealChannel = channel;
		}
	}

	void AppConsole::AddLogF(Channel channel, LogLevel level, const char* format, ...) {
		// Full formatted overload -- targets the specified channel
		char buffer[4096];
		va_list args;
		va_start(args, format);
		vsnprintf(buffer, sizeof(buffer), format, args);
		va_end(args);
		buffer[sizeof(buffer) - 1] = '\0';
		m_channels[static_cast<int>(channel)].emplace_back(buffer, level);
		//
		// During a build/debug operation, mirror script messages to the Build channel
		//
		if (channel == Channel::Script && m_scriptMirrorToBuild)
			m_channels[static_cast<int>(Channel::Build)].emplace_back(buffer, level);
		if (ShouldReveal(channel, level)) {
			m_revealPending = true;
			m_revealChannel = channel;
		}
	}

	void AppConsole::SetScriptMirrorToBuild(bool enable) {
		m_scriptMirrorToBuild = enable;
	}

	void AppConsole::Clear() {
		// Clear all entries across every channel
		for (int i = 0; i < 4; i++)
			m_channels[i].clear();
	}

	void AppConsole::Clear(Channel channel) {
		// Clear all entries in the specified channel only
		m_channels[static_cast<int>(channel)].clear();
	}

	bool AppConsole::TakeRevealRequest(Channel& outChannel) {
		// Return false if no reveal is pending
		if (!m_revealPending)
			return false;
		// Consume the pending request and return the target channel
		m_revealPending = false;
		outChannel = m_revealChannel;
		return true;
	}

	void AppConsole::SetActiveChannel(Channel channel) {
		// Update the active channel and flag a pending tab switch for the next render
		m_activeChannel = channel;
		m_pendingChannelSwitch = true;
	}

	ImVec4 AppConsole::GetColorForLevel(LogLevel level) {
		// Map each severity level to a distinct text color
		switch (level) {
			case LogLevel::Info:
				return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
			case LogLevel::Warning:
				return ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
			case LogLevel::Error:
				return ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
			default:
				return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		}
	}

}
