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

    void ResizeSeparator(float& sizeValue, float& startValue, ImGuiSeparatorFlags separatorFlags, ImVec2 sizeCap, ImVec2 size)
    {
		const ImGuiWindow * window = GetCurrentWindow();

		const auto cursorPos = GetCursorPos();
		const auto avail = GetContentRegionAvail();
		size.x = size.x == -1 ? avail.x : size.x == 0 ? 6 : size.x;
		size.y = size.y == -1 ? avail.y : size.y;

		GetItemID();

		const bool isVert = (separatorFlags & ImGuiSeparatorFlags_Vertical) == ImGuiSeparatorFlags_Vertical;

		if (isVert) {
			GetWindowDrawList()->AddLine(cursorPos + ImVec2(size.x/2 + 1, 0), cursorPos + ImVec2(size.x/2 + 1, size.y), GetColorU32(ImGuiCol_Separator));
			SetCursorPos(cursorPos + ImVec2((size.x/2), 0));
			Dummy({ size.x, size.y });
		}
		else {
			PushItemWidth(size.x);
			SeparatorSpace(separatorFlags, { 0, size.y });
			PopItemWidth();
			SetCursorPos(cursorPos + ImVec2(0, size.y/2));
			Dummy({ size.x, size.y });
		}

		if ((IsItemHovered() && (IsMouseClicked(0) && startValue == 0)) || (IsMouseDown(0) && startValue != 0))
		{
			if (IsMouseDragging(0, 0))
			{
				if (!startValue) {
					startValue = sizeValue;
					ResetMouseDragDelta();
				}
				float drag = isVert ? GetMouseDragDelta().x : (GetMouseDragDelta().y * -1);
				sizeValue = startValue + drag;
			}
			else
				startValue = 0;
		}
		else
			startValue = 0;

		sizeValue = ImClamp<float>(sizeValue, sizeCap.x, (isVert ? window->Size.x : window->Size.y) - sizeCap.y);

		if (IsItemHovered())
			SetMouseCursor(isVert ? ImGuiMouseCursor_ResizeEW : ImGuiMouseCursor_ResizeNS);
    }

    // Horizontal / Vertical separator with spacing
    void SeparatorSpace(ImGuiSeparatorFlags flags1, const ImVec2& size_arg)
    {
        ImGuiSeparatorFlags flags = flags1;
		const ImGuiWindow * window = GetCurrentWindow();
        if (window->SkipItems)
            return;

        if(flags1 == 0)
            flags = (window->DC.LayoutType == ImGuiLayoutType_Horizontal) ? ImGuiSeparatorFlags_Vertical : ImGuiSeparatorFlags_Horizontal;

        IM_ASSERT(ImIsPowerOfTwo(flags & (ImGuiSeparatorFlags_Horizontal | ImGuiSeparatorFlags_Vertical)));   // Check that only 1 option is selected

        const ImVec2 defaultSize = (flags == ImGuiSeparatorFlags_Horizontal) ?
                                    ImVec2(window->Size.x, 0) :
                                    ImVec2(0, window->DC.CurrLineSize.y);
        const ImVec2 size(size_arg.x == 0.0f ? defaultSize.x : size_arg.x, size_arg.y == 0.0f ? defaultSize.y : size_arg.y);
        if (flags & ImGuiSeparatorFlags_Vertical)
        {
            if (size_arg.x > 0)
            {
                Dummy({ (size_arg.x / 2) , 1 });
                SameLine();
            }

        	SeparatorEx(flags);

        	SameLine();

            if (size_arg.x > 0)
            {
                Dummy({ (size_arg.x / 2), 0 });
                SameLine();
            }
        }
        else if (flags & ImGuiSeparatorFlags_Horizontal)
        {
        	if (size_arg.y > 0)
        		Dummy({ 0, (size.y / 2) });

        	SeparatorEx(flags);

        	if (size_arg.y > 0)
        		Dummy({ 0, (size.y / 2) });
        }
    }
}