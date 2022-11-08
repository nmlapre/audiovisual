add_library(imgui STATIC)

target_include_directories(imgui
                           PUBLIC
                               backends
                               ${CMAKE_SOURCE_DIR}/lib/imgui)

target_sources(imgui
               PRIVATE
                   imgui_demo.cpp
                   imgui_draw.cpp
                   imgui_tables.cpp
                   imgui_widgets.cpp
                   imgui.cpp
                   backends/imgui_impl_dx11.cpp
                   backends/imgui_impl_win32.cpp)

target_link_libraries(imgui d3d11)

