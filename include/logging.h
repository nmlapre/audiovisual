#pragma once

#include <vector>

// Logging functionality - if enabled, logs all samples to a file for later review.
// This ends up being extremely useful if there are issues to investigate.
#define LOG_SESSION_TO_FILE true

namespace Logging
{
    // Get the buffers that hold left and right channels for the session. Ultimately,
    // these buffers are written to the wav file produced on save.
    std::pair<
        std::reference_wrapper<std::vector<float>>,
        std::reference_wrapper<std::vector<float>>>
        GetLogBuffers();

    // Logging functionality - if enabled, logs all samples to a file for later review.
    // This ends up being extremely useful if there are issues to investigate.
    void WriteToLogBuffer(const std::vector<float>& samplesLeft, const std::vector<float>& samplesRight);

    // Write the history of the session to a wav file.
    void WriteSessionToFile();

    // To be called from the realtime thread, this copies the current samples to the
    // log buffers. This function allocates, which is Not Good for real use, but is
    // a necessary evil for debugging.
    void CopyBufferAndDefer(const float* out, unsigned long framesPerBuffer);
}
