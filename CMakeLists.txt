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
        External/ImGui/imgui_draw.cpp
        External/ImGui/imgui_impl_glfw.cpp
        External/ImGui/imgui.cpp
        gui.cpp
        helper.cpp
        helper.h)

include_directories(${CMAKE_SOURCE_DIR})

# Include GLFW headers and library
target_link_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/Dependencies/GLFW/lib-vc2022)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/Dependencies/GLFW/Include)

# Include PortAudio headers
include_directories(${CMAKE_SOURCE_DIR}/Dependencies/PortAudio/include)

if (UNIX)
    find_package(X11 REQUIRED)
    target_link_libraries(${PROJECT_NAME} ${X11_LIBRARIES})
    find_package(glfw3 3.3 REQUIRED)
    target_link_libraries(${PROJECT_NAME} glfw)
else ()
    target_sources(${PROJECT_NAME} PRIVATE
            External/ImGui/imgui_impl_dx11.cpp
            External/ImGui/imgui_impl_win32.cpp
    )
    target_link_libraries(${PROJECT_NAME} glfw3)

    # Link PortAudio libs
    if(CMAKE_SIZEOF_VOID_P EQUAL 8) # 64 bit
        if(CMAKE_BUILD_TYPE MATCHES Debug)
            target_link_libraries(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/Dependencies/PortAudio/lib/portaudio_x64_dbg.lib)
        else ()
            target_link_libraries(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/Dependencies/PortAudio/lib/portaudio_x64.lib)
        endif ()
    else () # 32 bit
        if(CMAKE_BUILD_TYPE MATCHES Debug)
            target_link_libraries(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/Dependencies/PortAudio/lib/portaudio_x86_dbg.lib)
        else ()
            target_link_libraries(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/Dependencies/PortAudio/lib/portaudio_x86.lib)
        endif ()
    endif ()
endif ()

find_package(OpenGL REQUIRED)
target_link_libraries(${PROJECT_NAME} OpenGL::GL)

if (UNIX)
    # Can be installed with "sudo apt-get install libasound2-dev"
    find_package(ALSA REQUIRED)
    include_directories(${ALSA_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} ${ALSA_LIBRARIES})

    # Link PortAudio lib
    target_link_libraries(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/Dependencies/PortAudio/lib/libportaudio.a rt m asound pthread)
#    add_link_options(-lrt -lm -lasound -ljack -pthread)
#    set (CMAKE_STATIC_LINKER_FLAGS "lasound")
endif (UNIX)