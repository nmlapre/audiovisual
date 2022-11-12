#pragma once

#include "imgui.h"
#include "portaudio.h"

#include <vector>

// adapted from implot: utility structure for realtime plot
template <size_t MaxSize = 2000>
struct ScrollingBuffer
{
    ScrollingBuffer() { buffer.reserve(MaxSize); }
    
    void addPoint(float x, float y)
    {
        if (buffer.size() < MaxSize)
            buffer.push_back(ImVec2(x, y));
        else {
            buffer[offset] = ImVec2(x, y);
            offset = (offset + 1) % MaxSize;
        }
    }

    void erase()
    {
        if (buffer.size() > 0) {
            buffer.shrink(0);
            offset = 0;
        }
    }

    size_t offset = 0;
    std::vector<ImVec2> buffer;
};

template<class Ts>
void unused(Ts...)
{ }

constexpr bool isPowerOf2(int n) { return (n & (n - 1)) == 0; }

constexpr float clamp(float x, float lowerlimit, float upperlimit) {
    if (x < lowerlimit)
        x = lowerlimit;
    if (x > upperlimit)
        x = upperlimit;
    return x;
}

constexpr float smoothstep(float edge0, float edge1, float x) {
    // Scale, bias and saturate x to 0..1 range
    x = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    // Evaluate polynomial
    return x * x * (3 - 2 * x);
}

// Show portaudio debug info. Possibly useful for debugging.
void ShowDebugInfo(PaStream* stream);
