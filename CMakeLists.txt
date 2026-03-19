cmake_minimum_required(VERSION 3.27)
project(LatencyMeterRefreshed)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Force PortAudio static
set(PA_BUILD_STATIC ON CACHE INTERNAL "" FORCE)
set(PA_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(PA_BUILD_TESTS OFF CACHE BOOL "" FORCE)

add_executable(${PROJECT_NAME} main.cpp
        GUI/UIComponents.cpp
        GUI/UIComponents.h
        Serial/Serial.cpp
        Helpers/JsonHelper.cpp
        Audio/AudioProcessor.cpp
        External/nlohmann/json.hpp
        External/ImGui/imgui_extensions.cpp
        External/ImGui/imgui_widgets.cpp
        External/ImGui/imgui_tables.cpp
        External/ImGui/imgui_demo.cpp
        External/ImGui/imgui_draw.cpp
        External/ImGui/imgui.cpp
        GUI/Gui.cpp
        Helpers/AppHelper.cpp
        Helpers/AppHelper.h
        App/Models.h
        App/AppState.h
        App/AppCore.cpp
        App/AppCore.h
        App/AppGui.cpp
        App/AppGui.h
        App/AppInput.cpp
        App/AppInput.h)

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else ()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif ()

target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${CMAKE_SOURCE_DIR}
)

# PortAudio is cross-platform
add_subdirectory(Dependencies/portaudio)
target_link_libraries(${PROJECT_NAME} PRIVATE portaudio)
target_include_directories(${PROJECT_NAME} PRIVATE
        Dependencies/portaudio/include
)

# =========================
# Linux (GLFW + OpenGL)
# =========================
if (UNIX AND NOT APPLE)

    add_subdirectory(Dependencies/glfw)
    find_package(OpenGL REQUIRED)
    find_package(X11 REQUIRED)

    target_sources(${PROJECT_NAME} PRIVATE
            External/ImGui/imgui_impl_opengl3.cpp
            External/ImGui/imgui_impl_glfw.cpp
    )

    target_link_libraries(${PROJECT_NAME} PRIVATE
            glfw
            OpenGL::GL
            X11::X11
            asound
            pthread
    )

endif ()

# =========================
# Windows (DX11 + Win32)
# =========================
if (WIN32)

    target_sources(${PROJECT_NAME} PRIVATE
            External/ImGui/imgui_impl_dx11.cpp
            External/ImGui/imgui_impl_win32.cpp
    )

    target_link_libraries(${PROJECT_NAME} PRIVATE
            d3d11
            dxgi
            d3dcompiler
    )

endif ()