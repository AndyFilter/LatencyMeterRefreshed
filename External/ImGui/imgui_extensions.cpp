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

    // Horizontal / Vertical separator with spacing
    void SeparatorSpace(int flags1, const ImVec2& size_arg)
    {
        //ImGuiContext& g = *GImGui;
        ImGuiSeparatorFlags flags = (ImGuiSeparatorFlags)flags1;
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems)
            return;

        ImGuiContext& g = *GImGui;

        if(flags1 == 0)
            flags = (window->DC.LayoutType == ImGuiLayoutType_Horizontal) ? ImGuiSeparatorFlags_Vertical : ImGuiSeparatorFlags_Horizontal;

        IM_ASSERT(ImIsPowerOfTwo(flags & (ImGuiSeparatorFlags_Horizontal | ImGuiSeparatorFlags_Vertical)));   // Check that only 1 option is selected

        float thickness_draw = 1.0f;
        float thickness_layout = 0.0f;
        const ImVec2 default_size = (flags == ImGuiSeparatorFlags_Horizontal) ?
                                    ImVec2(window->Size.x, 0) :
                                    ImVec2(0, window->DC.CurrLineSize.y);
        const ImVec2 size(size_arg.x == 0.0f ? default_size.x : size_arg.x, size_arg.y == 0.0f ? default_size.y : size_arg.y);
        const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
        if (flags & ImGuiSeparatorFlags_Vertical)
        {
            if (size_arg.x > 0)
            {
                ImGui::Dummy({ (size_arg.x / 2) , 0 });
                ImGui::SameLine();
            }

            // Vertical separator, for menu bars (use current line height). Not exposed because it is misleading and it doesn't have an effect on regular layout.
            float y1 = window->DC.CursorPos.y + (window->DC.CurrLineSize.y / 2) - (size.y / 2);
            float y2 = window->DC.CursorPos.y + (window->DC.CurrLineSize.y / 2) + (size.y / 2);
            const ImRect bb(ImVec2(window->DC.CursorPos.x, y1), ImVec2(window->DC.CursorPos.x + thickness_draw, y2));
            ItemSize(ImVec2(thickness_layout, 0.0f));
            ImGui::SameLine();
            if (!ItemAdd(bb, 0))
                return;

            // Draw
            window->DrawList->AddLine(ImVec2(bb.Min.x, bb.Min.y), ImVec2(bb.Min.x, bb.Max.y), GetColorU32(ImGuiCol_Separator));
            if (g.LogEnabled)
                LogText(" |");

            if (size_arg.x > 0)
            {
                ImGui::Dummy({ (size_arg.x / 2), 0 });
                ImGui::SameLine();
            }

            //ImGui::SameLine();
        }
        else if (flags & ImGuiSeparatorFlags_Horizontal)
        {
            ImGui::Dummy({ 0, (size.y / 2) });

            // Horizontal Separator
            float x1 = window->Pos.x + (window->Size.x / 2) - (size.x / 2);
            float x2 = window->Pos.x + (window->Size.x / 2) + (size.x / 2);

            // FIXME-WORKRECT: old hack (#205) until we decide of consistent behavior with WorkRect/Indent and Separator
            if (g.GroupStack.Size > 0 && g.GroupStack.back().WindowID == window->ID)
                x1 += window->DC.Indent.x;

            // FIXME-WORKRECT: In theory we should simply be using WorkRect.Min.x/Max.x everywhere but it isn't aesthetically what we want,
            // need to introduce a variant of WorkRect for that purpose. (#4787)
            if (ImGuiTable* table = g.CurrentTable)
            {
                x1 = table->Columns[table->CurrentColumn].MinX;
                x2 = table->Columns[table->CurrentColumn].MaxX;
            }

            ImGuiOldColumns* columns = (flags & ImGuiSeparatorFlags_SpanAllColumns) ? window->DC.CurrentColumns : NULL;
            if (columns)
                PushColumnsBackground();

            // We don't provide our width to the layout so that it doesn't get feed back into AutoFit
            // FIXME: This prevents ->CursorMaxPos based bounding box evaluation from working (e.g. TableEndCell)
            const ImRect bb(ImVec2(x1, window->DC.CursorPos.y), ImVec2(x2, window->DC.CursorPos.y + thickness_draw));
            ItemSize(ImVec2(0.0f, thickness_layout));
            const bool item_visible = ItemAdd(bb, 0);
            if (item_visible)
            {
                // Draw
                window->DrawList->AddLine(bb.Min, ImVec2(bb.Max.x, bb.Min.y), GetColorU32(ImGuiCol_Separator));
                if (g.LogEnabled)
                    LogRenderedText(&bb.Min, "--------------------------------\n");

            }
            if (columns)
            {
                PopColumnsBackground();
                columns->LineMinY = window->DC.CursorPos.y;
            }

            ImGui::Dummy({ 0, (size.y / 2) });
        }
    }
}