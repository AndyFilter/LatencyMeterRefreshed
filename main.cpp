#include <Windows.h>

#include "gui.h"
#include "External/ImGui/imgui.h"

// Not the best GUI solution, but it's simple, fast and gets the job done.
void OnGui()
{

}

// Main code
int main(int, char**)
{
    GUI::Setup(OnGui);

    bool done = false;

    // Main Loop
    while (!done)
    {
        if (GUI::DrawGui())
            break;
    }

    GUI::Destroy();
}