add_library(implot STATIC)

target_include_directories(implot
                           PRIVATE
                               ${CMAKE_SOURCE_DIR}/lib/imgui
                           PUBLIC
                               ${CMAKE_SOURCE_DIR}/lib/implot)

target_sources(implot
               PRIVATE
                   implot_demo.cpp
                   implot_items.cpp
                   implot.cpp)

