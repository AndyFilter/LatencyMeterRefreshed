#include "imgui_internal.h"
#include "imgui.h"
#include "imgui_extensions.h"

namespace ImGui
{
	void HeaderTitle(const char* text, float space)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return;

		ImGuiContext& g = *GImGui;

		auto textSize = CalcTextSize(text);
        ImVec2 text_pos(window->DC.CursorPos.x, window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset);

        auto lineGap = (textSize.x + space)/2;

        // Horizontal Separator
        float x1 = window->Pos.x;
        float x2 = window->Pos.x + window->Size.x;

        if (g.GroupStack.Size > 0 && g.GroupStack.back().WindowID == window->ID)
            x1 += window->DC.Indent.x;

        float lineWidth = (x2 - x1);
        x2 = lineWidth / 2 + x1 - lineGap;

        //const ImRect bb(ImVec2(x1, window->DC.CursorPos.y), ImVec2(x2, window->DC.CursorPos.y + 1));
        const ImRect bb(ImVec2(x1, text_pos.y), ImVec2(x1, text_pos.y) + ImVec2(((window->Pos.x + window->Size.x) - x1), textSize.y));
        ItemSize(ImVec2(0.0f, 0));

		BeginGroup();
			
        const bool item_visible = ItemAdd(bb, 0);
        if (item_visible)
        {
            window->DrawList->AddLine({ bb.Min.x, bb.Min.y - (bb.Min.y - bb.Max.y)/2 }, ImVec2(x2, bb.Min.y - (bb.Min.y - bb.Max.y) / 2), GetColorU32(ImGuiCol_Separator));

            RenderTextClipped(bb.Min, bb.Max, text, NULL, &textSize, { 0.5f, 0.5f }, &bb);

            window->DrawList->AddLine({ bb.Min.x + lineWidth/2 + lineGap, bb.Min.y - (bb.Min.y - bb.Max.y) / 2 }, ImVec2(window->Pos.x + window->Size.x, bb.Min.y - (bb.Min.y - bb.Max.y) / 2), GetColorU32(ImGuiCol_Separator));

            Dummy({textSize.x, textSize.y - g.Style.ItemSpacing.y});
        }

		EndGroup();
	}

    void ResizeSeparator(float& sizeValue, static float& startValue, ImGuiSeparatorFlags separatorFlags, ImVec2 sizeCap, ImVec2 size)
    {
		ImGuiWindow* window = GetCurrentWindow();

		auto cursorPos = ImGui::GetCursorPos();
		auto avail = GetContentRegionAvail();
		size.x = size.x == -1 ? avail.x : size.x == 0 ? 6 : size.x;
		size.y = size.y == -1 ? avail.y : size.y;

		ImGui::GetItemID();

		bool isVert = (separatorFlags & ImGuiSeparatorFlags_Vertical) == ImGuiSeparatorFlags_Vertical;

		if (isVert) {
			ImGui::GetWindowDrawList()->AddLine(cursorPos + ImVec2(size.x/2 + 1, 0), cursorPos + ImVec2(size.x/2 + 1, size.y), ImGui::GetColorU32(ImGuiCol_Separator));
			ImGui::SetCursorPos(cursorPos + ImVec2((size.x/2), 0));
			ImGui::Dummy({ size.x, size.y });
		}
		else {
			PushItemWidth(size.x);
			SeparatorSpace(separatorFlags, { 0, size.y });
			PopItemWidth();
			ImGui::SetCursorPos(cursorPos + ImVec2(0, size.y/2));
			ImGui::Dummy({ size.x, size.y });
		}

		if ((ImGui::IsItemHovered() && (IsMouseClicked(0) && startValue == 0)) || (ImGui::IsMouseDown(0) && startValue != 0))
		{
			if (ImGui::IsMouseDragging(0, 0))
			{
				if (!startValue) {
					startValue = sizeValue;
					ResetMouseDragDelta();
				}
				float drag = isVert ? ImGui::GetMouseDragDelta().x : (ImGui::GetMouseDragDelta().y * -1);
				sizeValue = startValue + drag;
			}
			else
				startValue = 0;
		}
		else
			startValue = 0;

		sizeValue = ImClamp<float>(sizeValue, sizeCap.x, (isVert ? window->Size.x : window->Size.y) - sizeCap.y);

		if (IsItemHovered())
			ImGui::SetMouseCursor(isVert ? ImGuiMouseCursor_ResizeEW : ImGuiMouseCursor_ResizeNS);
    }
}