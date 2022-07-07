#include <Windows.h>
#include <iostream>
#include <chrono>

#include "gui.h"
#include "External/ImGui/imgui.h"


float lastFrameGui = 0.0f;
float lastFrame = 0.0f;

float accentColor[4];
float accentBrightness = 1;

uint64_t micros()
{
    uint64_t us = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch())
        .count();
    return us;
}

// Not the best GUI solution, but it's simple, fast and gets the job done.
int OnGui()
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open..", "Ctrl+O")) { /* Do stuff */ }
            if (ImGui::MenuItem("Save", "Ctrl+S")) { /* Do stuff */ }
            if (ImGui::MenuItem("Close", "Ctrl+W")) { return 1; }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::Text("Time is: %ims", clock());  

    ImGui::Text("Framerate: %f", 1/(clock() - lastFrameGui) * 1000);

    const auto avail = ImGui::GetContentRegionAvail();

    ImGuiStyle& style = ImGui::GetStyle();
    ImVec2 winSize = ImGui::GetWindowSize();

    ImGui::BeginChild("Stats", { avail.x - 16, 200}, true);
    ImGui::Text("test");

    ImGui::SliderFloat("Brightness", &accentBrightness, 0, 1);

    ImGui::ColorEdit4("Accent Color", (float*)accentColor);

    auto _accentColor = ImVec4(*(ImVec4*)accentColor) * accentBrightness;
    auto darker = ImVec4(*(ImVec4*)&_accentColor) * 0.9f;
    auto darkest = ImVec4(*(ImVec4*)&_accentColor) * 0.8f;

    _accentColor.w = ImVec4(*(ImVec4*)accentColor).w;
    darker.w = ImVec4(*(ImVec4*)accentColor).w;
    darkest.w = ImVec4(*(ImVec4*)accentColor).w;

    ImGui::Dummy( {0, 20} );

    ImGui::ColorEdit4("Real Accent Color", (float*)&_accentColor);
    ImGui::ColorEdit4("Accent Color Darker", (float*)&darker);
    ImGui::ColorEdit4("Accent Color Darkest", (float*)&darkest);

    style.Colors[ImGuiCol_Header] = *(ImVec4*)&_accentColor;
    style.Colors[ImGuiCol_HeaderHovered] = darker;
    style.Colors[ImGuiCol_HeaderActive] = darkest;

    ImGui::EndChild();

    lastFrameGui = clock();

    return 0;
}

// Main code
int main(int, char**)
{
    GUI::Setup(OnGui);

    bool done = false;

    uint64_t start = micros();

    memcpy(&accentColor, &ImGui::GetStyle().Colors[ImGuiCol_Header], sizeof(float) * 4);

    // Main Loop
    while (!done)
    {
        lastFrame = micros() - start;

        if(clock() - lastFrameGui > 20)
            if (GUI::DrawGui())
                break;
    }

    GUI::Destroy();
}
