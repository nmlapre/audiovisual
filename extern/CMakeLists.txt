# AudioFile
add_library(AudioFile INTERFACE AudioFile/AudioFile.h)

target_include_directories(AudioFile INTERFACE AudioFile)

# farbot
add_library(farbot INTERFACE farbot/include/farbot/AsyncCaller.hpp
                             farbot/include/farbot/fifo.hpp
                             farbot/include/farbot/RealtimeObject.hpp
                             farbot/include/farbot/RealtimeTraits.hpp)

target_include_directories(farbot INTERFACE farbot/include)


# gcem
add_library(gcem INTERFACE gcem/include/gcem.hpp)

target_include_directories(gcem INTERFACE gcem/include)

# imgui
add_library(imgui STATIC)

target_include_directories(imgui
                           PUBLIC
                               imgui/backends
                               ${CMAKE_SOURCE_DIR}/extern/imgui)

target_sources(imgui
               PRIVATE
                   imgui/imgui_demo.cpp
                   imgui/imgui_draw.cpp
                   imgui/imgui_tables.cpp
                   imgui/imgui_widgets.cpp
                   imgui/imgui.cpp
                   imgui/backends/imgui_impl_dx11.cpp
                   imgui/backends/imgui_impl_win32.cpp)

target_link_libraries(imgui d3d11)

# implot
add_library(implot STATIC)

target_include_directories(implot
                           PRIVATE
                               ${CMAKE_SOURCE_DIR}/extern/imgui
                           PUBLIC
                               ${CMAKE_SOURCE_DIR}/extern/implot)

target_sources(implot
               PRIVATE
                   implot/implot_demo.cpp
                   implot/implot_items.cpp
                   implot/implot.cpp)

# portaudio
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
add_subdirectory(portaudio EXCLUDE_FROM_ALL) # exclude uninstall target

