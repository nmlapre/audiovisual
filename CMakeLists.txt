cmake_minimum_required(VERSION 3.25)

project(audiovisual)

if (!WIN32)
  # this really only works on windows right now. so many apis
  message(ERROR "This only works on Windows right now. sry")
endif()

# statically link the runtime libraries
add_compile_options(
    $<$<CONFIG:>:/MT>
    $<$<CONFIG:Debug>:/MTd>
    $<$<CONFIG:Release>:/MT>
)

# grab the headers
file(GLOB_RECURSE
     _header_list
     RELATIVE
     "${CMAKE_SOURCE_DIR}"
     "${CMAKE_SOURCE_DIR}/include/*.h")

# grab the private source files
file(GLOB_RECURSE
     _private_source_list
     RELATIVE
     "${CMAKE_SOURCE_DIR}"
     "${CMAKE_SOURCE_DIR}/src/*.cpp")

# gotta set WIN32 here or everything breaks
add_executable(audiovisual WIN32 main.cpp ${_private_source_list} ${_header_list})

# properties of various sorts
set_property(TARGET audiovisual PROPERTY CXX_STANDARD 20)
target_compile_options(audiovisual PRIVATE /W4 /WX)

# include the files for the top-level project.
# while we're at it let's get 'em in a source group. why not
source_group("Header Files" FILES ${_header_list})
target_include_directories(audiovisual PRIVATE include)

# add libraries for submodules
add_subdirectory(extern)

target_link_libraries(audiovisual
                      imgui implot PortAudio
                      AudioFile farbot gcem) # header-only libs (this propagates includes)

