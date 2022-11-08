#pragma once

// PortAudio includes
#include <portaudio.h>
#include <pa_asio.h>
#include <pa_win_wasapi.h>

// AudioFile include
#include <AudioFile.h>

// STL
#include <chrono>
#include <iostream>
#include <filesystem>
#include <queue>
#include <set>
#include <string_view>
#include <unordered_map>
#include <variant>

// imgui
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

// implot
#include "implot.h"

// directx (for imgui)
#include <d3d11.h>

// Resources
#include "constants.h"

// Project files
#include "generator.h"
#include "thread_communication.h"
