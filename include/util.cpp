#include "util.h"

void ShowDebugInfo(PaStream* stream)
{
    const double cpuLoad = Pa_GetStreamCpuLoad(stream);
    ImGui::Text("Audio stream CPU load: %f", cpuLoad);

    const PaStreamInfo* streamInfo = Pa_GetStreamInfo(stream);
    if (streamInfo != nullptr)
    {
        ImGui::Text("Input latency: %f seconds", streamInfo->inputLatency);
        ImGui::Text("Output latency: %f seconds", streamInfo->outputLatency);
        ImGui::Text("Sample rate: %.1f", streamInfo->sampleRate);
    }

    PaTime streamTime = Pa_GetStreamTime(stream);
    ImGui::Text("Stream time: %f", streamTime);

    PaError isStreamStopped = Pa_IsStreamStopped(stream);
    ImGui::Text("stream stopped: %d", isStreamStopped);
    PaError isStreamActive = Pa_IsStreamActive(stream);
    ImGui::Text("stream active: %d", isStreamActive);

    const PaHostErrorInfo* lastHostError = Pa_GetLastHostErrorInfo();
    if (lastHostError)
    {
        ImGui::Text("\nLast error from API %d:\nerror code: %d\nerror text: %s\n",
            lastHostError->hostApiType,
            lastHostError->errorCode,
            lastHostError->errorText);
    }

    const PaVersionInfo* portaudioVersionInfo = Pa_GetVersionInfo();
    if (portaudioVersionInfo)
        ImGui::Text("portaudio version: %s", portaudioVersionInfo->versionText);
}

