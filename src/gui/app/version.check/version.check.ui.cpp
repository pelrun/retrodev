// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Version check UI -- notification banner and update popup.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#include "version.check.ui.h"
#include "version.check.h"
#include <app/app.icons.mdi.h>
#include <system/version.h>
#include <SDL3/SDL.h>

namespace RetrodevGui {

    //
    // Manifest URL -- update this to point to the hosted version.json for this project
    //
    static constexpr const char* k_manifestUrl =
        "https://raw.githubusercontent.com/tlotb/retrodev/master/bld/version.json";

    //
    // Cached result from the last completed Poll() so we can render it without re-polling every widget call
    //
    static VersionCheckResult s_cachedResult;

    void VersionCheckUi::Tick() {
        //
        // Fire the background check once on the first Tick call
        //
        if (!m_checkStarted) {
            m_checkStarted = true;
            VersionCheck::StartAsync(k_manifestUrl);
        }
        //
        // Poll latest result and cache it for render calls this frame
        //
        s_cachedResult = VersionCheck::Poll();
        //
        // Transition to pending notification exactly once when update is found
        //
        if (s_cachedResult.state == VersionCheckState::UpdateAvailable && !m_notificationShown) {
            m_pendingNotification = true;
            m_notificationShown = true;
            m_popupOpen = true;
        }
    }

    bool VersionCheckUi::HasPendingNotification() {
        return m_pendingNotification;
    }

    void VersionCheckUi::ShowPopup() {
        //
        // Always open the popup -- it renders different content per state
        //
        m_popupOpen = true;
    }

    void VersionCheckUi::RenderPopup() {
        //
        // Open the modal on the frame requested
        //
        if (m_popupOpen) {
            ImGui::OpenPopup("Check for Updates##versioncheck");
            m_popupOpen = false;
            m_popupShowing = true;
        }
        if (!m_popupShowing)
            return;
        //
        // Center on main viewport
        //
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(480.0f, 0.0f), ImGuiCond_Appearing);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 10.0f));
        if (ImGui::BeginPopupModal("Check for Updates##versioncheck", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
            ImGui::Dummy(ImVec2(0.0f, 4.0f));
            VersionCheckState state = s_cachedResult.state;
            if (state == VersionCheckState::Checking || state == VersionCheckState::Idle) {
                //
                // Fetch still in progress
                //
                ImGui::TextUnformatted(ICON_LOADING " Checking for updates...");
            } else if (state == VersionCheckState::Failed) {
                //
                // Network or parse failure
                //
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(220, 80, 80, 255));
                ImGui::TextUnformatted(ICON_ALERT_CIRCLE_OUTLINE " Could not reach the update server.");
                ImGui::PopStyleColor();
                ImGui::TextDisabled("Check your internet connection and try again.");
            } else if (state == VersionCheckState::UpToDate) {
                //
                // Already on the latest version
                //
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 220, 0, 255));
                ImGui::TextUnformatted(ICON_CHECK_CIRCLE_OUTLINE " RetroDev is up to date.");
                ImGui::PopStyleColor();
                ImGui::Spacing();
                ImGui::TextDisabled("Current version: %s", RetrodevLib::GetVersion().c_str());
            } else {
                //
                // Update available -- show version comparison and action buttons
                //
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 220, 0, 255));
                ImGui::TextUnformatted(ICON_UPDATE " A new version of RetroDev is available!");
                ImGui::PopStyleColor();
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::AlignTextToFramePadding();
                ImGui::TextDisabled("Current version:");
                ImGui::SameLine();
                ImGui::Text("%s", RetrodevLib::GetVersion().c_str());
                ImGui::AlignTextToFramePadding();
                ImGui::TextDisabled("Latest version: ");
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 220, 0, 255));
                ImGui::Text("%s", s_cachedResult.latestVersion.c_str());
                ImGui::PopStyleColor();
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                bool hasDownload = !s_cachedResult.downloadUrl.empty();
                bool hasNotes    = !s_cachedResult.releaseNotesUrl.empty();
                ImGui::BeginDisabled(!hasDownload);
                if (ImGui::Button(ICON_DOWNLOAD " Download", ImVec2(140.0f, 0.0f))) {
                    if (hasDownload)
                        SDL_OpenURL(s_cachedResult.downloadUrl.c_str());
                }
                ImGui::EndDisabled();
                if (hasNotes) {
                    ImGui::SameLine();
                    if (ImGui::Button(ICON_TEXT_BOX_OUTLINE " Release Notes", ImVec2(180.0f, 0.0f)))
                        SDL_OpenURL(s_cachedResult.releaseNotesUrl.c_str());
                }
                ImGui::SameLine();
            }
            //
            // Close button -- always shown, clears notification badge only when update was shown
            //
            if (state == VersionCheckState::UpdateAvailable)
                ImGui::SameLine();
            float closeWidth = 100.0f;
            ImGui::SetCursorPosX(ImGui::GetWindowWidth() - closeWidth - ImGui::GetStyle().WindowPadding.x);
            if (ImGui::Button("Close", ImVec2(closeWidth, 0.0f))) {
                if (state == VersionCheckState::UpdateAvailable)
                    m_pendingNotification = false;
                m_popupShowing = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::Dummy(ImVec2(0.0f, 4.0f));
            ImGui::EndPopup();
        }
        ImGui::PopStyleVar();
    }

}
