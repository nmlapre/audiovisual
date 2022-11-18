#pragma once

#include "portaudio.h"

using PaCallbackT = int (*)(
    const void*,
    void*,
    unsigned long,
    const PaStreamCallbackTimeInfo*,
    PaStreamCallbackFlags, void*);

PaStream* InitializePAStream(PaCallbackT paCallback);

void TearDownPAStream(PaStream* stream);
