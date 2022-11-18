#pragma once

#include "logging.h"

namespace Plotting
{
    // Draw a live updating graph of L, R signals. Right now it moves too fast and I get dizzy lol
    // Requires log buffers (LOG_SESSION_TO_FILE), which should probably be moving windows of history.
    void DrawOscilatorPlot(const std::vector<float>& logBufferL, const std::vector<float>& logBufferR);
}

