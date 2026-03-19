#include "../GUI/UIComponents.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include "../App/AppCore.h"
#include "../External/ImGui/imgui.h"
#include "../Helpers/AppHelper.h"
#include "../Helpers/JsonHelper.h"
#include "Gui.h"

#include "../External/ImGui/imgui_extensions.h"
#include "../External/ImGui/imgui_internal.h"
#include "Serial/Serial.h"

#define REVERT_ARRAY(mainVar, bakVar, size)                                                                            \
	if (ImGui::IsItemClicked(1))                                                                                       \
		memcpy(mainVar, bakVar, sizeof(float) * size);

using namespace AppCore;
using namespace AppHelper;

namespace UIComponents {
	ImGuiStyle *style = nullptr;
	ImGuiIO *io = nullptr;

	void SetupData() {
		style = &ImGui::GetStyle();
		io = &ImGui::GetIO();
	}

	int DrawMenuBar(AppState &state, bool &isSettingOpen, bool &badVerPopupOpen) {
		using namespace AppHelper;

		if (ImGui::BeginMenuBar()) {
			short openFullscreenCommand = 0;
			badVerPopupOpen = false;
			// static bool saveMeasurementsPopup = false;
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("Open", "Ctrl+O")) {
					const auto res = OpenMeasurements();
					if ((res & JSON_HANDLE_INFO_BAD_VERSION) != 0u) {
						badVerPopupOpen = true;
						// ImGui::OpenPopup("JSON version mismatch!");
						// printf("BAD VERSION!\n");
					}
				}
				if (ImGui::MenuItem("Save", "Ctrl+S")) {
					SaveMeasurements();
				}
				if (ImGui::MenuItem("Save as", "Ctrl+Shift+S")) {
					SaveAsMeasurements();
				}
				if (ImGui::MenuItem("Save Pack", "Alt+S")) {
					SavePackMeasurements();
				}
				if (ImGui::MenuItem("Save Pack As", "Alt+Shift+S")) {
					SavePackAsMeasurements();
				}
				if (ImGui::MenuItem("Export as CSV"
						"")) {
					ExportCSV();
				}
				if (ImGui::MenuItem("Fullscreen", "Alt+Enter")) {
					// Gui::SetFullScreenState(g_IsFullscreen);

					if (!state.isFullscreen && !state.fullscreenModeOpenPopup)
						openFullscreenCommand = 1;
					else if (!state.fullscreenModeClosePopup && !state.fullscreenModeOpenPopup)
						openFullscreenCommand = -1;
				}
				if (ImGui::MenuItem("Settings", "")) {
					isSettingOpen = !isSettingOpen;
				}
				if (ImGui::MenuItem("Close", "")) {
					return 1;
				}
				ImGui::EndMenu();
			}

			// Draw FPS
			{
				const auto menuBarAvail = ImGui::GetContentRegionAvail();

				static const float kWidth = ImGui::CalcTextSize("FPS12345FPS    FPS123456789FPS").x;

				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (menuBarAvail.x - kWidth));
				ImGui::BeginGroup();

				ImGui::Text("FPS:");
				ImGui::SameLine();
				ImGui::Text("%.1f GUI", ImGui::GetIO().Framerate);
				ImGui::SameLine();
				// Right click total frames to reset it
				ImGui::Text(" %.1f CPU", 1000000.f / state.averageFrametime);
				static uint64_t totalFramerateStartTime{GetTimeMicros()};
				// The framerate is sometimes just too high to be properly displayed by the moving average, and it's
				// "small" buffer. So this should show average framerate over the entire program lifespan
				ImGui::SetItemTooltip("Avg. framerate: %.1f",
				                      static_cast<float>(state.totalFrames) / (
					                      GetTimeMicros() - totalFramerateStartTime) * 1000000.f);
				if (ImGui::IsItemClicked(1)) {
					totalFramerateStartTime = GetTimeMicros();
					state.totalFrames = 0;
				}

				ImGui::EndGroup();
			}
			ImGui::EndMenuBar();

			if (openFullscreenCommand == 1) {
				ImGui::OpenPopup("Enter Fullscreen mode?");
				state.fullscreenModeOpenPopup = true;
			} else if (openFullscreenCommand == -1) {
				ImGui::OpenPopup("Exit Fullscreen mode?");
				state.fullscreenModeClosePopup = true;
			}

			if (badVerPopupOpen)
				ImGui::OpenPopup("JSON version mismatch!");
		}


		// if (ImGui::BeginMenuBar()) {
		//     short openFullscreenCommand = 0;
		//     if (ImGui::BeginMenu("File")) {
		//         if (ImGui::MenuItem("Open", "Ctrl+O")) {
		//             auto res = OpenMeasurements();
		//             if (res & JSON_HANDLE_INFO_BAD_VERSION) {
		//                 badVerPopupOpen = true;
		//             }
		//         }
		//         if (ImGui::MenuItem("Save", "Ctrl+S")) { SaveMeasurements(); }
		//         if (ImGui::MenuItem("Save as", "Ctrl+Shift+S")) { SaveAsMeasurements(); }
		//         if (ImGui::MenuItem("Save Pack", "Alt+S")) { SavePackMeasurements(); }
		//         if (ImGui::MenuItem("Save Pack As", "Alt+Shift+S")) { SavePackAsMeasurements(); }
		//         if (ImGui::MenuItem("Export as CSV""")) { ExportCSV(); }
		//         if (ImGui::MenuItem("Fullscreen", "Alt+Enter")) {
		//             if (!state.isFullscreen && !state.fullscreenModeOpenPopup)
		//                 openFullscreenCommand = 1;
		//             else if (!state.fullscreenModeClosePopup && !state.fullscreenModeOpenPopup)
		//                 openFullscreenCommand = -1;
		//         }
		//         if (ImGui::MenuItem("Settings", "")) { isSettingOpen = !isSettingOpen; }
		//         if (ImGui::MenuItem("Close", "")) {
		//             // This is tricky, it returns from OnGui. For now we just let it be.
		//         }
		//         ImGui::EndMenu();
		//     }
		//
		//     // Draw FPS
		//     {
		//         auto menuBarAvail = ImGui::GetContentRegionAvail();
		//         static const float width = ImGui::CalcTextSize("FPS12345FPS    FPS123456789FPS").x;
		//         ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (menuBarAvail.x - width));
		//         ImGui::BeginGroup();
		//         ImGui::Text("FPS:");
		//         ImGui::SameLine();
		//         ImGui::Text("%.1f GUI", ImGui::GetIO().Framerate);
		//         ImGui::SameLine();
		//         ImGui::Text(" %.1f CPU", 1000000.f / averageFrametime);
		//         static uint64_t totalFramerateStartTime{ micros() };
		//         ImGui::SetItemTooltip("Avg. framerate: %.1f", ((float)totalFrames / (micros() -
		//         totalFramerateStartTime)) * 1000000.f); if (ImGui::IsItemClicked(1)) {
		//             totalFramerateStartTime = micros();
		//             // totalFrames = 0; // Can't modify passed by value
		//         }
		//         ImGui::EndGroup();
		//     }
		//     ImGui::EndMenuBar();
		//
		//     if (openFullscreenCommand == 1) {
		//         ImGui::OpenPopup("Enter Fullscreen mode?");
		//         state.fullscreenModeOpenPopup = true;
		//     } else if (openFullscreenCommand == -1) {
		//         ImGui::OpenPopup("Exit Fullscreen mode?");
		//         state.fullscreenModeClosePopup = true;
		//     }
		//
		//     if (badVerPopupOpen)
		//         ImGui::OpenPopup("JSON version mismatch!");
		// }

		return 0;
	}

	void DrawSettingsWindow(AppState &state, bool &isSettingOpen) {
		using namespace AppHelper;
		if (isSettingOpen) {
			ImGui::SetNextWindowSize({480.0f, 480.0f});

			const bool wasLastSettingsOpen = isSettingOpen;
			if (ImGui::Begin("Settings", &isSettingOpen, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
				// On Settings Window Closed:
				if (wasLastSettingsOpen && !isSettingOpen && HasConfigChanged()) {
					// Call a popup by name
					ImGui::OpenPopup("Save Style?");
				}

				// Define the messagebox popup
				if (ImGui::BeginPopupModal("Save Style?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
					isSettingOpen = true;
					ImGui::PushFont(g_boldFont);
					ImGui::Text("Do you want to save before exiting?");
					ImGui::PopFont();


					ImGui::Separator();
					ImGui::Dummy({0, ImGui::GetTextLineHeight()});

					// These buttons don't really fit the window perfectly with some FONTS, I might have to look into
					// that.
					ImGui::BeginGroup();

					if (ImGui::Button("Save")) {
						SaveCurrentUserConfig();
						isSettingOpen = false;
						ImGui::CloseCurrentPopup();
					}
					ImGui::SameLine();
					if (ImGui::Button("Discard")) {
						RevertConfig();
						isSettingOpen = false;
						ImGui::CloseCurrentPopup();
					}
					ImGui::SameLine();
					if (ImGui::Button("Cancel")) {
						ImGui::CloseCurrentPopup();
					}

					ImGui::EndGroup();

					ImGui::EndPopup();
				}


				static int selectedSettings = 0;
				const auto avail = ImGui::GetContentRegionAvail();

				constexpr std::array<const char *, 3> ITEMS{"Style", "Performance", "Misc"};

				// Makes list take ({listBoxSize}*100)% width of the parent
				const float listBoxSize = 0.3f;

				if (ImGui::BeginListBox("##SettingList", {avail.x * listBoxSize, avail.y})) {
					for (int i = 0; i < std::size(ITEMS); i++) {
						if (const bool isSelected = selectedSettings == i; ImGui::Selectable(
								ITEMS[i], isSelected, 0, {0, 0})) {
							selectedSettings = i;
						}
					}
					ImGui::EndListBox();
				}

				ImGui::SameLine();

				ImGui::BeginGroup();

				switch (selectedSettings) {
					case 0: // Style
					{
						if (ImGui::BeginChild("Style##Child",
						                      {(1 - listBoxSize) * avail.x - style->FramePadding.x * 2,
						                       avail.y - ImGui::GetFrameHeightWithSpacing()},
						                      1)) {

							ImGui::PushID("Main Color Settings");
							ImGui::ColorEdit4("Main Color", state.currentUserData.style.mainColor);

							ImGui::SliderFloat("Brightness", &state.currentUserData.style.mainColorBrightness, -0.5f,
							                   0.5f);
							RIGHT_CLICK_REVERTIBLE(&state.currentUserData.style.mainColorBrightness,
							                       state.backupUserData.style.mainColorBrightness);
							ImGui::PopID();

							auto accentColor = ImVec4(
									*reinterpret_cast<ImVec4 *>(state.currentUserData.style.mainColor));
							auto darkerAccent = ImVec4(accentColor) *
							                    (1 - state.currentUserData.style.mainColorBrightness);
							auto darkestAccent = ImVec4(accentColor) *
							                     (1 - state.currentUserData.style.mainColorBrightness * 2);

							// Alpha has to be set separatly, because it also gets multiplied times brightness.
							const auto alphaAccent = ImVec4(
									*reinterpret_cast<ImVec4 *>(state.currentUserData.style.mainColor)).w;
							accentColor.w = alphaAccent;
							darkerAccent.w = alphaAccent;
							darkestAccent.w = alphaAccent;

							ImGui::Dummy({0, ImGui::GetFrameHeight() / 2});

							ImVec2 colorsAvail = ImGui::GetContentRegionAvail();

							ImGui::ColorButton("Accent Color", static_cast<ImVec4>(accentColor), 0,
							                   {colorsAvail.x, ImGui::GetFrameHeight()});
							ImGui::ColorButton("Accent Color Dark", darkerAccent, 0,
							                   {colorsAvail.x, ImGui::GetFrameHeight()});
							ImGui::ColorButton("Accent Color Darkest", darkestAccent, 0,
							                   {colorsAvail.x, ImGui::GetFrameHeight()});

							ImGui::SeparatorSpace(ImGuiSeparatorFlags_Horizontal,
							                      {colorsAvail.x * 0.9f, ImGui::GetFrameHeight()});

							ImGui::PushID("Font Color Settings");
							ImGui::ColorEdit4("Font Color", state.currentUserData.style.fontColor);

							ImGui::SliderFloat("Brightness", &state.currentUserData.style.fontColorBrightness, -1.0f,
							                   1.0f, "%.3f");
							RIGHT_CLICK_REVERTIBLE(&state.currentUserData.style.fontColorBrightness,
							                       state.backupUserData.style.fontColorBrightness);

							auto _fontColor =
									ImVec4(*reinterpret_cast<ImVec4 *>(state.currentUserData.style.fontColor));
							auto darkerFont = ImVec4(_fontColor) *
							                  (1 - state.currentUserData.style.fontColorBrightness);

							// Alpha has to be set separatly, because it also gets multiplied times brightness.
							const auto alphaFont =
									ImVec4(*reinterpret_cast<ImVec4 *>(state.currentUserData.style.fontColor)).w;
							_fontColor.w = alphaFont;
							darkerFont.w = alphaFont;

							ImGui::Dummy({0, ImGui::GetFrameHeight() / 2});

							ImGui::ColorButton("Font Color", _fontColor, 0, {colorsAvail.x, ImGui::GetFrameHeight()});
							ImGui::ColorButton("Font Color Dark", darkerFont, 0,
							                   {colorsAvail.x, ImGui::GetFrameHeight()});

							ImGui::PopID();

							ImGui::SeparatorSpace(ImGuiSeparatorFlags_Horizontal,
							                      {colorsAvail.x * 0.9f, ImGui::GetFrameHeight()});


							ImGui::ColorEdit4("Int. Plot", state.currentUserData.style.internalPlotColor);
							ImGui::SetItemTooltip("Internal plot color");
							ImGui::ColorEdit4("Ext. Plot", state.currentUserData.style.externalPlotColor);
							ImGui::SetItemTooltip("External plot color");
							ImGui::ColorEdit4("Input Plot", state.currentUserData.style.inputPlotColor);
							ImGui::SetItemTooltip("Input plot color");

							ApplyCurrentStyle();

							ImGui::SeparatorSpace(ImGuiSeparatorFlags_Horizontal,
							                      {colorsAvail.x * 0.9f, ImGui::GetFrameHeight()});

							// This has to be done manually (instead of ImGui::Combo()) to be able to change the FONTS
							// of each selectable to a corresponding one.
							if (ImGui::BeginCombo("Font", FONTS[state.currentUserData.style.selectedFont])) {
								for (int i = 0; i < ((io->Fonts->Fonts.Size - 1) / 2) + 1; i++) {

									ImGui::PushFont(io->Fonts->Fonts[FONT_INDEX[i]]);

									const bool isSelected = state.currentUserData.style.selectedFont == i;
									if (ImGui::Selectable(FONTS[i], isSelected, 0, {0, 0})) {
										if (state.currentUserData.style.selectedFont != i) {
											io->FontDefault = io->Fonts->Fonts[FONT_INDEX[i]];
											if (auto *const boldFont = GetFontBold(i); boldFont != nullptr)
												g_boldFont = boldFont;
											else
												g_boldFont = io->Fonts->Fonts[FONT_INDEX[i]];
											state.currentUserData.style.selectedFont = i;
										}
									}
									ImGui::PopFont();
								}
								ImGui::EndCombo();
							}

							const bool hasFontSizeChanged = ImGui::SliderFloat(
									"Font Size", &state.currentUserData.style.fontSize, 0.5f, 2, "%.2f");
							RIGHT_CLICK_REVERTIBLE(&state.currentUserData.style.fontSize,
							                       state.backupUserData.style.fontSize);
							if (hasFontSizeChanged || ImGui::IsItemClicked(1)) {
								for (int i = 0; i < io->Fonts->Fonts.Size; i++) {
									io->Fonts->Fonts[i]->Scale = state.currentUserData.style.fontSize;
								}
							}
						}

						ImGui::EndChild();

						break;
					}
					// Performance
					case 1: {
						if (ImGui::BeginChild("Performance##Child",
						                      {((1 - listBoxSize) * avail.x) - (style->FramePadding.x * 2),
						                       avail.y - ImGui::GetFrameHeightWithSpacing()},
						                      1)) {
							const auto performanceAvail = ImGui::GetContentRegionAvail();
							ImGui::Checkbox("###LockGuiFpsCB", &state.currentUserData.performance.lockGuiFps);
							ImGui::SetItemTooltip(
									"It's strongly recommended to keep GUI FPS locked for the best performance");

							ImGui::BeginDisabled(!state.currentUserData.performance.lockGuiFps);
							ImGui::SameLine();
							ImGui::PushItemWidth(performanceAvail.x - ImGui::GetItemRectSize().x -
							                     (style->FramePadding.x * 3) - ImGui::CalcTextSize("GUI Refresh Rate").
							                     x);
							// ImGui::SliderInt("GUI FPS", &guiLockedFps, 30, 360, "%.1f");
							ImGui::DragInt("GUI Refresh Rate", &state.currentUserData.performance.guiLockedFps, .5f, 30,
							               480);
							RIGHT_CLICK_REVERTIBLE(&state.currentUserData.performance.guiLockedFps,
							                       state.backupUserData.performance.guiLockedFps);
							ImGui::PopItemWidth();
							ImGui::EndDisabled();

							ImGui::Spacing();

							ImGui::Checkbox("Show Plots", &state.currentUserData.performance.showPlots);
							ImGui::SetItemTooltip("Plots can have a small impact on the performance");

							ImGui::Spacing();

							// ImGui::Checkbox("VSync", &state.currentUserData.performance.VSync);
							ImGui::SliderInt("VSync", &(int &) state.currentUserData.performance.VSync, 0, 4, "%d",
							                 ImGuiSliderFlags_AlwaysClamp);
							ImGui::SetItemTooltip(
									"This setting synchronizes your monitor with the program to avoid screen "
									"tearing.\n(Adds a significant amount of latency)");
						}
						ImGui::EndChild();
						break;
					}
					// Misc
					case 2: {
						if (ImGui::BeginChild("Misc##Child",
						                      {((1 - listBoxSize) * avail.x) - (style->FramePadding.x * 2),
						                       avail.y - ImGui::GetFrameHeightWithSpacing()},
						                      1)) {
							ImGui::Checkbox("Save config locally", &state.currentUserData.misc.localUserData);
#ifdef _WIN32
							ImGui::SetItemTooltip(
									"Chose whether you want to save config file in the same directory as the program, "
									"or in the AppData folder");
#else
							ImGui::SetItemTooltip(
									"Chose whether you want to save config file in the same directory as the program, "
									"or in the /home/{usr}/.LatencyMeter folder");
#endif
						}

						ImGui::EndChild();

						break;
					}
					default:
						break;
				}

				ImGui::BeginGroup();

				const auto buttonsAvail = ImGui::GetContentRegionAvail();

				if (ImGui::Button("Revert", {((buttonsAvail.x / 2) - style->FramePadding.x),
				                             ImGui::GetFrameHeight()})) {
					RevertConfig();
				}

				ImGui::SameLine();

				if (ImGui::Button("Save", {((buttonsAvail.x / 2) - style->FramePadding.x), ImGui::GetFrameHeight()})) {
					SaveCurrentUserConfig();
					// SaveStyleConfig(colors, brightnesses, selectedFont, fontSize);
				}

				ImGui::EndGroup();

				ImGui::EndGroup();
			}
			ImGui::End();
		}
	}

	void HandleShortcuts(AppState &state) {
		using namespace AppHelper;
		if (ImGui::Shortcut(ImGuiKey_W | ImGuiMod_Ctrl)) {
			char tabClosePopupName[48];
			snprintf(tabClosePopupName, 48, "Save before Closing?###TabExit%i", state.selectedTab);

			if (state.tabsInfo.size() > 1) {
				if (!state.tabsInfo[state.selectedTab].isSaved) {
					ImGui::OpenPopup(tabClosePopupName);
				} else {
					DeleteTab(state.selectedTab);
					if (state.selectedTab >= state.tabsInfo.size() - 1)
						state.selectedTab = static_cast<int>(state.tabsInfo.size() - 1);
				}
			}
		}

		if (ImGui::Shortcut(ImGuiKey_Enter | ImGuiMod_Alt) || ImGui::Shortcut(ImGuiKey_KeypadEnter | ImGuiMod_Alt)) {
			state.isFullscreen = Gui::GetFullScreenState();
			if (!state.isFullscreen) {
				ImGui::OpenPopup("Enter Fullscreen mode?");
			} else {
				ImGui::OpenPopup("Exit Fullscreen mode?");
			}
		}

		if (ImGui::Shortcut(ImGuiKey_Escape)) {
			state.isFullscreen = Gui::GetFullScreenState();
			if (state.isFullscreen) {
				ImGui::OpenPopup("Exit Fullscreen mode?");
			}
		}
	}

	void DrawPopups(AppState &state) {
		using namespace AppHelper;
		if (ImGui::BeginPopupModal("JSON version mismatch!", nullptr,
		                           ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
		                           ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
			ImGui::Text("Some of the imported files are either from an older or newer version of the program\nThe data "
					"might be corrupted or wrong!");
			if (ImGui::Button("Ok", {-1, 0})) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		if (ImGui::BeginPopupModal("Enter Fullscreen mode?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("Are you sure you want to enter Fullscreen mode?");
			ImGui::SetItemTooltip("Press Escape to Exit");
			ImGui::SeparatorSpace(0, {0, 10});
			if (ImGui::Button("Yes") || (IS_ONLY_ENTER_PRESSED && !ImGui::GetIO().KeyAlt)) {
				state.isFullscreen = Gui::SetFullScreenState(true) == 0;

				ImGui::CloseCurrentPopup();
				state.fullscreenModeOpenPopup = false;
			}
			ImGui::SameLine();
			if (ImGui::Button("No") || IS_ONLY_ESCAPE_PRESSED) {
				ImGui::CloseCurrentPopup();
				state.fullscreenModeOpenPopup = false;
			}
			ImGui::EndPopup();
		}

		if (ImGui::BeginPopupModal("Exit Fullscreen mode?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("Are you sure you want to exit Fullscreen mode?");
			ImGui::SeparatorSpace(0, {0, 10});

			if (ImGui::Button("Yes") || ImGui::Shortcut(ImGuiKey_Enter) || ImGui::Shortcut(ImGuiKey_KeypadEnter)) {
				state.isFullscreen = Gui::SetFullScreenState(false) != 0;

				ImGui::CloseCurrentPopup();
				state.fullscreenModeClosePopup = false;
			}
			ImGui::SameLine();
			if (ImGui::Button("No") || ImGui::Shortcut(ImGuiKey_Escape)) {
				ImGui::CloseCurrentPopup();
				state.fullscreenModeClosePopup = false;
			}
			ImGui::EndPopup();
		}
	}

	void DrawTabs(AppState &state) {
		int deletedTabIndex = -1;

		if (ImGui::BeginTabBar("TestsTab", ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_Reorderable |
		                                   ImGuiTabBarFlags_FittingPolicyScroll)) {

			std::scoped_lock const lock(state.latencyStatsMutex);
			for (int tabN = 0; tabN < state.tabsInfo.size(); tabN++) {
				// char idText[24];
				// snprintf(idText, 24, "Tab Item %i", tabN);
				// ImGui::PushID(idText);

				char tabNameLabel[64 + 16];
				snprintf(tabNameLabel, 64 + 16, "%s###Tab%i", state.tabsInfo[tabN].name, tabN);

				// ImGui::PushItemWidth(ImGui::CalcTextSize(tabNameLabel).x + 100);
				// if(!state.tabsInfo[tabN].isSaved)
				//	ImGui::SetNextItemWidth(ImGui::CalcTextSize(state.tabsInfo[tabN].name, NULL).x +
				//(style.FramePadding.x * 2) + 15); else
				ImGui::SetNextItemWidth(ImGui::CalcTextSize(state.tabsInfo[tabN].name, nullptr).x +
				                        (style->FramePadding.x * 2) + (state.tabsInfo[tabN].isSaved ? 0 : 15));
				// state.selectedTab = state.tabsInfo.size() - 1;
				auto flags = state.selectedTab == tabN ? ImGuiTabItemFlags_SetSelected : 0;
				flags |= state.tabsInfo[tabN].isSaved ? 0 : ImGuiTabItemFlags_UnsavedDocument;
				if (ImGui::BeginTabItem(tabNameLabel, nullptr, flags)) {
					ImGui::EndTabItem();
				}
				// ImGui::PopItemWidth();
				// ImGui::PopID();

				if (ImGui::IsItemHovered()) {
					if (ImGui::IsMouseDown(0)) {
						state.selectedTab = tabN;
					}

					const auto mouseDelta = io->MouseWheel;
					state.selectedTab += static_cast<int>(mouseDelta);
					state.selectedTab = std::clamp(state.selectedTab, 0, static_cast<int>(state.tabsInfo.size()) - 1);
				}

				char popupName[32]{0};
				snprintf(popupName, 32, "Change tab %i name", tabN);

				char popupTextEdit[32]{0};
				snprintf(popupTextEdit, 32, "Ed Txt %i", tabN);

				char tabClosePopupName[48];
				snprintf(tabClosePopupName, 48, "Save before Closing?###TabExit%i", tabN);

				bool openTabClose = false;

				if (ImGui::IsItemFocused()) {
					if (ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
						DEBUG_PRINT("deleting tab: %i\n", tabN);
						if (state.tabsInfo.size() > 1) {
							if (!state.tabsInfo[tabN].isSaved) {
								ImGui::OpenPopup(tabClosePopupName);
							} else {
								deletedTabIndex = tabN;
								if (tabN == state.tabsInfo.size() - 1) {
									const ImGuiContext &g = *GImGui;
									ImGui::SetFocusID(g.CurrentTabBar->Tabs[tabN - 1].ID, ImGui::GetCurrentWindow());
								}
								// Unfocus so the next items won't get deleted (This can also be achieved by checking if
								// DEL was not down) ImGui::SetWindowFocus(nullptr);
							}
						}
					}

					if (ImGui::IsKeyDown(ImGuiKey_F2)) {
						ImGui::OpenPopup(popupTextEdit);
					}

					if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
						state.selectedTab--;

					if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
						state.selectedTab++;

					state.selectedTab = std::clamp(state.selectedTab, 0, static_cast<int>(state.tabsInfo.size()) - 1);
				}

				if (ImGui::BeginPopup(popupTextEdit, ImGuiWindowFlags_NoMove)) {
					ImGui::SetKeyboardFocusHere(0);
					if (ImGui::InputText("Tab name", state.tabsInfo[tabN].name, TAB_NAME_MAX_SIZE,
					                     ImGuiInputTextFlags_AutoSelectAll)) {
						char defaultTabName[16]{0};
						snprintf(defaultTabName, 16, "Tab %i", tabN);
						state.tabsInfo[tabN].isSaved = (strcmp(defaultTabName, state.tabsInfo[tabN].name) == 0);
					}
					ImGui::EndPopup();
				}

				if (ImGui::IsItemClicked(1) || (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))) {
					DEBUG_PRINT("Open tab %i tooltip\n", tabN);
					ImGui::OpenPopup(popupName);
				}

				if (ImGui::BeginPopup(popupName, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings)) {
					// Check if this name already exists
					// ImGui::SetKeyboardFocusHere(0);
					if (ImGui::InputText("Tab name", state.tabsInfo[tabN].name, TAB_NAME_MAX_SIZE,
					                     ImGuiInputTextFlags_AutoSelectAll)) {
						char defaultTabName[16]{0};
						snprintf(defaultTabName, 16, "Tab %i", tabN);
						state.tabsInfo[tabN].isSaved = (strcmp(defaultTabName, state.tabsInfo[tabN].name) == 0);
					}

					if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_Escape))
						ImGui::CloseCurrentPopup();

					// ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 4);
					if (state.tabsInfo.size() > 1) {
						if (ImGui::Button("Close Tab", {-1, 0})) {
							if (!state.tabsInfo[tabN].isSaved) {
								ImGui::CloseCurrentPopup();
								// ImGui::OpenPopup(tabClosePopupName);
								openTabClose = true;
							} else {
								deletedTabIndex = tabN;
								ImGui::CloseCurrentPopup();
							}
						}
					}

					ImGui::EndPopup();
				}

				if (openTabClose) {
					ImGui::OpenPopup(tabClosePopupName);
				}

				if (ImGui::BeginPopupModal(tabClosePopupName, nullptr,
				                           ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings)) {
					ImGui::Text("Are you sure you want to close this tab?\nAll measurements will be lost if you don't "
							"save");

					ImGui::SeparatorSpace(ImGuiSeparatorFlags_Horizontal, {0, 10});

					auto popupAvail = ImGui::GetContentRegionAvail();

					popupAvail.x -= style->ItemSpacing.x * 2;

					if (ImGui::Button("Save", {popupAvail.x / 3, 0})) {
						if (SaveMeasurements())
							deletedTabIndex = tabN;
						ImGui::CloseCurrentPopup();
					}
					ImGui::SameLine();
					if (ImGui::Button("Discard", {popupAvail.x / 3, 0})) {
						deletedTabIndex = tabN;
						ImGui::CloseCurrentPopup();
					}
					ImGui::SameLine();
					if (ImGui::Button("Cancel", {popupAvail.x / 3, 0})) {
						ImGui::CloseCurrentPopup();
					}

					ImGui::EndPopup();
				}
			}


			if (ImGui::TabItemButton(" + ", ImGuiTabItemFlags_Trailing)) {
				auto newTab = TabInfo();
#ifdef _WIN32
				strcat_s < TAB_NAME_MAX_SIZE > (newTab.name, std::to_string(state.tabsInfo.size()).c_str());
#else
				strcat(newTab.name, std::to_string(state.tabsInfo.size()).c_str());
#endif
				state.tabsInfo.push_back(newTab);
				state.selectedTab = static_cast<int>(state.tabsInfo.size() - 1);

				// ImGui::EndTabItem();
			}

			// ImGui::Text("Selected Item %i", state.selectedTab);

			ImGui::EndTabBar();
		}

		if (ImGui::IsItemHovered()) {
			const auto mouseDelta = io->MouseWheel;
			state.selectedTab += static_cast<int>(mouseDelta);
			state.selectedTab = std::clamp(state.selectedTab, 0, static_cast<int>(state.tabsInfo.size()) - 1);
		}


		if (deletedTabIndex >= 0) {
			DeleteTab(deletedTabIndex);
		}
	}

	void DrawConnectionPanel(AppState &state, std::uint64_t curTime) {
		static ImVec2 restItemsSize;

		ImGui::BeginGroup();

		const bool isSelectedConnected = state.isInternalMode ? state.isAudioDevConnected : Serial::g_isConnected;

		ImGui::PushItemWidth(80);
		ImGui::BeginDisabled(isSelectedConnected);

		if (state.isInternalMode) {
			// #ifdef _WIN32
			if (ImGui::BeginCombo("Device",
			                      !state.availableAudioDevices.empty()
				                      ? state.availableAudioDevices[state.selectedPort].friendlyName.c_str()
				                      : "0 Found")) {
				for (size_t i = 0; i < state.availableAudioDevices.size(); i++) {
					if (!state.availableAudioDevices[i].isInput)
						continue;
					const bool isSelected = state.selectedPort == i;
					// char _buf[128];
					// sprintf_s(_buf, "%s (%s)", state.availableAudioDevices[i].friendlyName.c_str(),
					// state.availableAudioDevices[i].portName);
					if (ImGui::Selectable(state.availableAudioDevices[i].friendlyName.c_str(), isSelected, 0, {0, 0})) {
						if (state.selectedPort != i)
							Serial::Close();
						state.selectedPort = static_cast<int>(i);
					}
				}
				ImGui::EndCombo();
			}
			// #endif
		} else {
			if (ImGui::BeginCombo("Port", !Serial::g_availablePorts.empty()
				                              ? Serial::g_availablePorts[state.selectedPort].c_str()
				                              : "0 Found")) {
				for (size_t i = 0; i < Serial::g_availablePorts.size(); i++) {
					if (const bool isSelected = state.selectedPort == i;
						ImGui::Selectable(Serial::g_availablePorts[i].c_str(), isSelected, 0, {0, 0})) {
						if (state.selectedPort != i)
							Serial::Close();
						state.selectedPort = static_cast<int>(i);
					}
				}
				ImGui::EndCombo();
			}
		}
		ImGui::PopItemWidth();
		ImGui::EndDisabled();

		ImGui::SameLine();

		if (ImGui::Button("Refresh") && state.serialStatus == Status_Idle) {
			if (state.isInternalMode)
				DiscoverAudioDevices(state);
			else
				Serial::FindAvailableSerialPorts();
		}

		ImGui::SameLine();
		// ImGui::SeparatorSpace(ImGuiSeparatorFlags_Vertical, { 0, 0 });

		if (isSelectedConnected) {
			if (ImGui::Button("Disconnect")) {
				AttemptDisconnect(state);
			}
		} else {
			if (ImGui::Button("Connect") && ((!Serial::g_availablePorts.empty() && !state.isInternalMode) ||
			                                 (!state.availableAudioDevices.empty() && state.isInternalMode))) {
				AttemptConnect(state);
			}
		}

		ImGui::SameLine();
		ImGui::SeparatorSpace(ImGuiSeparatorFlags_Vertical, {0, 0});

		if (ImGui::Button("Clear")) {
			ImGui::OpenPopup("Clear?");
		}

		if (ImGui::BeginPopupModal("Clear?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("Are you sure you want to clear EVERYTHING?");

			ImGui::SeparatorSpace(0, {0, 10});

			if (ImGui::Button("Yes") || IS_ONLY_ENTER_PRESSED) {
				ClearData(state);
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("No") || IS_ONLY_ESCAPE_PRESSED) {
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		// ImGui::SameLine();

		ImGui::EndGroup();

		const auto portItemsSize = ImGui::GetItemRectSize();

		if (portItemsSize.x + restItemsSize.x + style->WindowPadding.x + style->ItemSpacing.x <
		    ImGui::GetContentRegionAvail().x) {
			ImGui::SameLine();
			ImGui::Dummy({-10, 0});
			ImGui::SameLine();
			// ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
			// ImGui::SameLine();

			ImGui::SeparatorSpace(ImGuiSeparatorFlags_Vertical, {0, 0});
		} else
			ImGui::SeparatorSpace(ImGuiSeparatorFlags_Horizontal, {0, 0});

		ImGui::BeginGroup();

		ImGui::Checkbox("Game Mode", &state.isGameMode);
		ImGui::SetItemTooltip(
				"This mode instead of lighting up the rectangle at the bottom will simulate pressing the left mouse "
				"button (fire in game).\nTo register the delay between input and shot in game.");

		ImGui::SameLine();

		ImGui::BeginDisabled(isSelectedConnected);
		if (ImGui::Checkbox("Internal Mode", &state.isInternalMode)) {
			std::swap(state.lastSelectedPort, state.selectedPort);
		}
		ImGui::EndDisabled();
		ImGui::SetItemTooltip("Uses a sensor connected to a 3.5mm jack. (More info on Github)");

		ImGui::SameLine();

		if (ImGui::Checkbox("Audio Mode", &state.isAudioMode)) {
			if (state.isAudioMode) {
				state.isPlaybackDevConnected =
						GetAudioProcessor().Initialize() &&
						GetAudioProcessor().PrimePlaybackStream(
								state.availableAudioDevices[state.selectedOutputAudioDeviceIdx].id);
			} else if (!state.isAudioDevConnected && !state.isInternalMode)
				GetAudioProcessor().Terminate();
		}
		static bool wasOutAudioDevJustSelected = false;
		if (!wasOutAudioDevJustSelected && state.isAudioMode && ImGui::BeginPopupContextItem()) {
			if (ImGui::BeginCombo("Output device",
			                      state.availableAudioDevices.empty()
				                      ? "No Audio Devices Found"
				                      : state.availableAudioDevices[state.selectedOutputAudioDeviceIdx]
				                        .friendlyName.c_str())) {

				for (int i = 0; i < state.availableAudioDevices.size(); i++) {
					const auto &dev = state.availableAudioDevices[i];
					if (dev.isInput)
						continue;
					if (ImGui::Selectable(dev.friendlyName.c_str(), i == state.selectedOutputAudioDeviceIdx)) {
						state.selectedOutputAudioDeviceIdx = i;
						state.isPlaybackDevConnected = GetAudioProcessor().PrimePlaybackStream(
								state.availableAudioDevices[state.selectedOutputAudioDeviceIdx].id);
						// was_out_audio_dev_just_selected = true; // Uncomment to auto-close the popup on select
						break;
					}
				}

				ImGui::EndCombo();
			}
			ImGui::EndPopup();
		} else {
			wasOutAudioDevJustSelected = false;
		}
		ImGui::SetItemTooltip(state.isAudioMode
			                      ? "[Right-Click to select output device] Measures Audio Latency (Uses a "
			                      "microphone instead of a photoresistor)"
			                      : "Measures Audio Latency (Uses a microphone instead of a photoresistor)");

		if (state.isInternalMode) {
			ImGui::SameLine();
			ImGui::SeparatorSpace(ImGuiSeparatorFlags_Vertical, {0, 0});
			ImGui::SameLine();

			ImGui::BeginDisabled(!CanDoInternalTest(state, curTime));
			auto cursorPos = ImGui::GetCurrentWindow()->DC.CursorPos;
			if (ImGui::Button("Run Test")) {
				StartInternalTest(state);
			}
			ImGui::EndDisabled();
			auto itemSize = ImGui::GetItemRectSize();
			const auto frac =
					std::clamp<float>(static_cast<float>(curTime - state.lastInternalTest) /
					                  (1000.f * static_cast<uint64_t>(FRAMES_TO_CAPTURE) / AUDIO_SAMPLE_RATE +
					                   INTERNAL_TEST_DELAY),
					                  0, 1);
			// ImGui::ProgressBar(frac);
			cursorPos.y += ImGui::GetCurrentWindow()->DC.CurrLineTextBaseOffset + style->FramePadding.y * 2 +
					ImGui::GetTextLineHeight() + 2;
			cursorPos.x += 5;
			itemSize.x -= 10;
			itemSize.x *= frac;
			// itemSize.x = itemSize.x < 0 ? 0 : itemSize.x;
			ImGui::GetWindowDrawList()->AddLine(cursorPos, {cursorPos.x + itemSize.x, cursorPos.y},
			                                    ImGui::GetColorU32(ImGuiCol_SeparatorActive), 2);
			ImGui::SetItemTooltip("It's advised to use Shift+R instead.");
		}
		ImGui::EndGroup();

		restItemsSize = ImGui::GetItemRectSize();
	}

	void DrawLatencyStatsPanel(AppState &state) {
		ImGui::PushFont(g_boldFont);

		if (ImGui::BeginChild("MeasurementStats",
		                      {0, (ImGui::GetTextLineHeightWithSpacing() * 4) + (style->WindowPadding.y * 2) -
		                          style->ItemSpacing.y + (style->FramePadding.y * 2)},
		                      1)) {
			if (ImGui::BeginTable("Latency Stats Table", 4,
			                      ImGuiTableFlags_BordersInner | ImGuiTableFlags_NoPadOuterX)) {
				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(1);
				ImGui::Text("Internal [ms]");
				ImGui::SetItemTooltip("Time measured by the computer from the moment it got the start signal (button "
						"press) to the signal that the light intensity change was spotted");
				ImGui::TableNextColumn();
				ImGui::Text("External [ms]");
				ImGui::SetItemTooltip(
						"Time measured by the microcontroller from the button press to the change of light intensity");
				ImGui::TableNextColumn();
				ImGui::Text("Input [ms]");
				ImGui::SetItemTooltip(
						"Time between computer sending a message to the microcontroller and receiving it back (Ping)");

				ImGui::TableNextRow();

				state.latencyStatsMutex.lock();

				ImGui::TableSetColumnIndex(0);
				ImGui::Text("Highest");
				ImGui::TableNextColumn();
				ImGui::Text("%.2f",
				            static_cast<float>(state.tabsInfo[state.selectedTab].latencyStats.internalLatency.highest) /
				            1000.f);
				ImGui::TableNextColumn();
				ImGui::Text("%.2f",
				            static_cast<float>(state.tabsInfo[state.selectedTab].latencyStats.externalLatency.highest) /
				            1000.f);
				ImGui::TableNextColumn();
				ImGui::Text("%.2f",
				            static_cast<float>(state.tabsInfo[state.selectedTab].latencyStats.inputLatency.highest) /
				            1000.f);

				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				ImGui::Text("Average");
				ImGui::TableNextColumn();
				ImGui::Text("%.2f", state.tabsInfo[state.selectedTab].latencyStats.internalLatency.average / 1000);
				ImGui::TableNextColumn();
				ImGui::Text("%.2f", state.tabsInfo[state.selectedTab].latencyStats.externalLatency.average / 1000);
				ImGui::TableNextColumn();
				ImGui::Text("%.2f", state.tabsInfo[state.selectedTab].latencyStats.inputLatency.average / 1000);

				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				ImGui::Text("Lowest");
				ImGui::TableNextColumn();
				ImGui::Text("%.2f",
				            static_cast<float>(state.tabsInfo[state.selectedTab].latencyStats.internalLatency.lowest) /
				            1000.f);
				ImGui::TableNextColumn();
				ImGui::Text("%.2f",
				            static_cast<float>(state.tabsInfo[state.selectedTab].latencyStats.externalLatency.lowest) /
				            1000.f);
				ImGui::TableNextColumn();
				ImGui::Text("%.2f",
				            static_cast<float>(state.tabsInfo[state.selectedTab].latencyStats.inputLatency.lowest) /
				            1000.f);

				state.latencyStatsMutex.unlock();

				ImGui::EndTable();
			}

			ImGui::EndChild();
		}

		ImGui::PopFont();
	}

	void DrawPlotsPanel(AppState &state) {
		if (state.currentUserData.performance.showPlots) {
			const auto plotsAvail = ImGui::GetContentRegionAvail();
			auto plotHeight = std::min(std::max((plotsAvail.y - 4 * style->FramePadding.y) / 4, 40.f), 100.f);

			const auto &measurements = state.tabsInfo[state.selectedTab].latencyData.measurements;

			state.latencyStatsMutex.lock();

			// Separate Plots
			SetPlotLinesColor(*reinterpret_cast<ImVec4 *>(state.currentUserData.style.internalPlotColor));
			ImGui::PlotLines(
					"Internal Latency",
					[](void *data, const int idx) {
						const auto *appState = static_cast<AppState *>(data);
						return static_cast<float>(
							       appState->tabsInfo[appState->selectedTab].latencyData.measurements[idx].timeInternal)
						       /
						       1000.f;
					},
					&state, static_cast<int>(measurements.size()), 0, nullptr, FLT_MAX, FLT_MAX, {0, plotHeight});
			SetPlotLinesColor(*reinterpret_cast<ImVec4 *>(state.currentUserData.style.externalPlotColor));
			ImGui::PlotLines(
					"External Latency",
					[](void *data, const int idx) {
						const auto *appState = static_cast<AppState *>(data);
						return static_cast<float>(
							       appState->tabsInfo[appState->selectedTab].latencyData.measurements[idx].timeExternal)
						       /
						       1000.f;
					},
					&state, static_cast<int>(measurements.size()), 0, nullptr, FLT_MAX, FLT_MAX, {0, plotHeight});
			SetPlotLinesColor(*reinterpret_cast<ImVec4 *>(state.currentUserData.style.inputPlotColor));
			ImGui::PlotLines(
					"Input Latency",
					[](void *data, const int idx) {
						const auto *appState = static_cast<AppState *>(data);
						return static_cast<float>(
							       appState->tabsInfo[appState->selectedTab].latencyData.measurements[idx].timeInput) /
						       1000.f;
					},
					&state, static_cast<int>(measurements.size()), 0, nullptr, FLT_MAX, FLT_MAX, {0, plotHeight});
			ImGui::PopStyleColor(6);

			// Combined Plots
			const auto startCursorPos = ImGui::GetCursorPos();
			ImGui::SetCursorPos(startCursorPos);
			SetPlotLinesColor(*reinterpret_cast<ImVec4 *>(state.currentUserData.style.internalPlotColor));
			ImGui::PlotLines(
					"Combined Plots",
					[](void *data, const int idx) {
						const auto *appState = static_cast<AppState *>(data);
						return static_cast<float>(
							       appState->tabsInfo[appState->selectedTab].latencyData.measurements[idx].timeInternal)
						       /
						       1000.f;
					},
					&state, static_cast<int>(measurements.size()), 0, nullptr,
					static_cast<float>(state.tabsInfo[state.selectedTab].latencyStats.inputLatency.lowest) / 1000,
					(static_cast<float>(state.tabsInfo[state.selectedTab].latencyStats.internalLatency.highest) / 1000)
					+
					1,
					{0, plotHeight});

			ImGui::PushStyleColor(ImGuiCol_FrameBg, {0, 0, 0, 0});
			ImGui::PushStyleColor(ImGuiCol_Border, {0, 0, 0, 0});
			ImGui::SetCursorPos(startCursorPos);
			SetPlotLinesColor(*reinterpret_cast<ImVec4 *>(state.currentUserData.style.externalPlotColor));
			ImGui::PlotLines(
					"###ExternalPlot",
					[](void *data, const int idx) {
						const auto *appState = static_cast<AppState *>(data);
						return static_cast<float>(
							       appState->tabsInfo[appState->selectedTab].latencyData.measurements[idx].timeExternal)
						       /
						       1000.f;
					},
					&state, static_cast<int>(measurements.size()), 0, nullptr,
					static_cast<float>(state.tabsInfo[state.selectedTab].latencyStats.inputLatency.lowest) / 1000,
					(static_cast<float>(state.tabsInfo[state.selectedTab].latencyStats.internalLatency.highest) / 1000)
					+
					1,
					{0, plotHeight});

			ImGui::SetCursorPos(startCursorPos);
			SetPlotLinesColor(*reinterpret_cast<ImVec4 *>(state.currentUserData.style.inputPlotColor));
			ImGui::PlotLines(
					"###InputPlot",
					[](void *data, const int idx) {
						const auto *appState = static_cast<AppState *>(data);
						return static_cast<float>(
							       appState->tabsInfo[appState->selectedTab].latencyData.measurements[idx].timeInput) /
						       1000.f;
					},
					&state, static_cast<int>(measurements.size()), 0, nullptr,
					static_cast<float>(state.tabsInfo[state.selectedTab].latencyStats.inputLatency.lowest) / 1000,
					(static_cast<float>(state.tabsInfo[state.selectedTab].latencyStats.internalLatency.highest) / 1000)
					+
					1,
					{0, plotHeight});
			ImGui::PopStyleColor(8);

			state.latencyStatsMutex.unlock();
		}

		ImGui::EndChild();
	}

	void DrawMeasurementsTable(AppState &state, ImVec2 spaceAvail) {
		if (ImGui::BeginTable("measurementsTable", 5,
		                      ImGuiTableFlags_Reorderable | ImGuiTableFlags_SortMulti | ImGuiTableFlags_Sortable |
		                      ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY,
		                      {spaceAvail.x, 200})) {
			ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed, 0.0f,
			                        0);
			ImGui::TableSetupColumn("Internal Latency (ms)", ImGuiTableColumnFlags_WidthStretch, 0.0f, 1);
			ImGui::TableSetupColumn("External Latency (ms)", ImGuiTableColumnFlags_WidthStretch, 0.0f, 2);
			ImGui::TableSetupColumn("Input Latency (ms)", ImGuiTableColumnFlags_WidthStretch, 0.0f, 3);
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 0.0f, 4);
			ImGui::TableHeadersRow();

			// Copy of the original vector has to be used because there is a possibility that some items will be
			// removed from it. In which case changes will be updated on the next frame
			std::vector<LatencyReading> latencyTestsCopy = state.tabsInfo[state.selectedTab].latencyData.measurements;

			if (ImGuiTableSortSpecs *sortsSpecs = ImGui::TableGetSortSpecs())
				if (sortsSpecs->SpecsDirty) {
					state.sortSpec = sortsSpecs;
					if (state.tabsInfo[state.selectedTab].latencyData.measurements.size() > 1)
						qsort(state.tabsInfo[state.selectedTab].latencyData.measurements.data(),
						      state.tabsInfo[state.selectedTab].latencyData.measurements.size(),
						      sizeof(state.tabsInfo[state.selectedTab].latencyData.measurements[0]), CompareLatency);
					state.sortSpec = nullptr;
					sortsSpecs->SpecsDirty = false;
				}

			ImGuiListClipper clipper;
			clipper.Begin(static_cast<int>(latencyTestsCopy.size()), ImGui::GetTextLineHeightWithSpacing());
			while (clipper.Step())
				for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
					auto &[timeExternal, timeInternal, timeInput, index] = latencyTestsCopy[i];

					ImGui::PushID(i + 100);

					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					ImGui::Text("%i", index);
					ImGui::TableNextColumn();

					ImGui::Text("%i", timeInternal / 1000);
					/* \xC2\xB5 is the 'microseconds' character (looks like u (not you, just "u")) */
					ImGui::SetItemTooltip("%i\xC2\xB5s", timeInternal);
					ImGui::TableNextColumn();

					ImGui::Text("%i", timeExternal / 1000);
					ImGui::SetItemTooltip("%i\xC2\xB5s", timeExternal);
					ImGui::TableNextColumn();

					ImGui::Text("%i", timeInput / 1000);
					ImGui::SetItemTooltip("%i\xC2\xB5s", timeInput);

					ImGui::TableNextColumn();

					if (ImGui::SmallButton("X")) {
						RemoveMeasurement(i);
					}

					ImGui::PopID();
				}


			if (state.wasMeasurementAddedGUI)
				ImGui::SetScrollHereY();


			ImGui::EndTable();
		}
	}

	void DrawNotesPanel(AppState &state, const ImVec2 availSize) {
		ImGui::SeparatorText("Measurement notes");
		const ImVec2 size = {availSize.x, std::min(ImGui::GetContentRegionAvail().y - availSize.y - 5, 600.f)};
		ImGui::InputTextMultiline("##Measurements Notes", state.tabsInfo[state.selectedTab].latencyData.note, 1000,
		                          size, ImGuiInputTextFlags_NoHorizontalScroll);
	}

	void DrawDetectionRect(const AppState &state, const ImVec2 &detectionRectSize, const ImVec2 &windowAvail) {
		// auto windowAvail = ImGui::GetContentRegionMax();
		//  Color change detection rectangle.
		ImVec2 rectSize{0, detectionRectSize.y};
		rectSize.x = detectionRectSize.x == 0
			             ? windowAvail.x + style->WindowPadding.x + style->FramePadding.x
			             : detectionRectSize.x;
		ImVec2 pos = {0, windowAvail.y - rectSize.y + (style->WindowPadding.y * 2) + (style->FramePadding.y * 2) + 10};
		pos.y += 12;
		const ImRect bb{pos, pos + rectSize};
		ImGui::RenderFrame(bb.Min, bb.Max,
		                   // Change the color to white to be detected by the light sensor
		                   state.serialStatus == Status_WaitingForResult && !state.isGameMode && !state.isAudioMode
			                   ? ImGui::ColorConvertFloat4ToU32(ImVec4(1.f, 1.f, 1.f, 1))
			                   : ImGui::ColorConvertFloat4ToU32(ImVec4(0.f, 0.f, 0.f, 1.f)),
		                   false);
		// if (ImGui::IsItemFocused())
		//	ImGui::SetFocusID(0, ImGui::GetCurrentWindow());

		if (state.isInternalMode && state.serialStatus == Status_Idle && state.currentUserData.performance.showPlots &&
		    HasRecordingEnded()) {
			ImGui::SetCursorPosX(0);
			// rectSize.y -= style.WindowPadding.y/2;
			ImGui::PushStyleColor(ImGuiCol_FrameBg, {0, 0, 0, 0});
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
			SetPlotLinesColor({1, 1, 1, 0.5});
			// else
			//	SetPlotLinesColor({ 0,0,0,0.5 });

			auto &processor = GetAudioProcessor();
			if (state.isAudioDevConnected && processor.snapshotReady.load(std::memory_order_acquire)) {
				const short *readBuf = processor.GetSnapshotBuffer();
				ImGui::PlotLines("##audioBufferPlot2",
				                 [](void *data, const int idx) {
					                 const auto *buf = static_cast<const short *>(data);
					                 return static_cast<float>(buf[idx]);
				                 },
				                 const_cast<short*>(readBuf), FRAMES_TO_CAPTURE / MAIN_BUFFER_SIZE_FRACTION_RECIPROCAL, 0, nullptr,
				                 -static_cast<float>(SHRT_MAX), static_cast<float>(SHRT_MAX), rectSize);

				if (state.lastInternalGateSampleIdx != 0) {
					const auto thresholdSamplePosition = static_cast<float>(state.lastInternalGateSampleIdx) / (
						                                     FRAMES_TO_CAPTURE / MAIN_BUFFER_SIZE_FRACTION_RECIPROCAL);
					const auto lineXPos = thresholdSamplePosition * (rectSize.x);
					ImGui::GetWindowDrawList()->AddLine({pos.x + lineXPos, pos.y},
					                                    {pos.x + lineXPos, pos.y + rectSize.y},
					                                    ImColor(0.7f, 9.f, 0.5f, 0.85f), 1);
				}
			}
			ImGui::PopStyleColor(3);
			ImGui::PopStyleVar();
		}
	}
} // namespace UIComponents
