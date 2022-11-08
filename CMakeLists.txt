cmake_minimum_required(VERSION 3.25)

project(audiovisual)

if (!WIN32)
  # this really only works on windows right now. so many apis
  message(ERROR "This only works on Windows right now. sry")
endif()

# grab the headers
file(GLOB_RECURSE
     _header_list
     RELATIVE
     "${CMAKE_SOURCE_DIR}"
     "${CMAKE_SOURCE_DIR}/include/*.h")

# gotta set WIN32 here or everything breaks. hey why not
add_executable(audiovisual WIN32 main.cpp ${_header_list})
# annoying, but so it goes
add_definitions(-DNOMINMAX)

# properties of various sorts
set_property(TARGET audiovisual PROPERTY CXX_STANDARD 20)
target_compile_options(audiovisual PRIVATE /W4 /WX)

# include the files for the top-level project.
# while we're at it let's get 'em in a source group. why not
source_group("Header Files" FILES ${_header_list})
target_include_directories(audiovisual PRIVATE include)

# add libraries
add_subdirectory(lib/AudioFile)
add_subdirectory(lib/farbot)
add_subdirectory(lib/gcem)
add_subdirectory(lib/imgui)
add_subdirectory(lib/implot)
add_subdirectory(lib/portaudio)

# suppress annoying portaudio lib warning (type conversion)
target_compile_options(portaudio PRIVATE /wd4312)
target_compile_options(portaudio_static PRIVATE /wd4312)

target_link_libraries(audiovisual
                      imgui implot portaudio_static
                      AudioFile farbot gcem) # header-only libs (this propagates includes)

