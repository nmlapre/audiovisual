#pragma once

#include "oscillator.h"
#include "thread_communication.h"

#include "portaudio.h"

struct UIOscillatorView
{
    // This function, called from the non-realtime thread, checks for responses
    // to requests sent in prior frames. Responses must come back in the same
    // order that they were sent, and they must have the expected response values.
    void HandleRealTimeResponse();

    // Draw the oscillators. Pushes events to the realtime thread if the user
    // changes settings.
    void Show();

private:
    // Get a request id suitable for identifying the next request event.
    RequestId GetNextRequestId();

    // Draw the settings for a single oscillator.
    void ShowOscillator(const OscillatorId& oscillatorId, const OscillatorSettings& settings);

    // Keep track of the request id that's next up. I think uint32 is Unique Enough.
    RequestId m_currentRequestId{ 0 };

    // Updated when a response comes back successfully (and only then).
    std::unordered_map<OscillatorId, OscillatorSettings> m_oscillators;
};
