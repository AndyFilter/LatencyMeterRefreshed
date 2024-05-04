cmake_minimum_required(VERSION 3.27)
project(LatencyMeterRefreshed)

set(CMAKE_CXX_STANDARD 17)

add_executable(${PROJECT_NAME} main.cpp
        serial.cpp
        helperJson.cpp
        Audio/Sound_Helper.cpp
        External/nlohmann/json.hpp
        External/ImGui/imgui_extensions.cpp
        External/ImGui/imgui_widgets.cpp
        External/ImGui/imgui_tables.cpp
        External/ImGui/imgui_impl_opengl3.cpp
        External/ImGui/imgui_demo.cpp
        External/ImGui/imgui_demo.cpp
        External/ImGui/imgui_draw.cpp
        External/ImGui/imgui_impl_glfw.cpp
        External/ImGui/imgui.cpp
        gui.cpp
        helper.cpp
        helper.h)

include_directories(${CMAKE_SOURCE_DIR})

message(${CMAKE_SOURCE_DIR}/Dependencies/GLFW/lib-vc2022)

target_link_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/Dependencies/GLFW/lib-vc2022)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/Dependencies/GLFW/Include)

find_package(glfw3 3.3 REQUIRED)
target_link_libraries(${PROJECT_NAME} glfw) # Possibly glfw3 for Windows

find_package(OpenGL REQUIRED)
target_link_libraries(${PROJECT_NAME} OpenGL::GL)

if (UNIX)
    # Can be installed with "sudo apt-get install libasound2-dev"
    find_package(ALSA REQUIRED)
    include_directories(${ALSA_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} ${ALSA_LIBRARIES})
endif (UNIX)