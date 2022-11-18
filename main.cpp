// AudioVisual.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "audiovisual.h"
#include "logging.h"
#include "oscillator_ui.h"
#include "pa_management.h"
#include "plotting.h"
#include "windowing.h"

// This function runs on the realtime thread provided by portaudio.
// It should not make any system calls (incl. allocation). It should primarily
// process requests to change its settings, enqueue responses to those requests,
// and write the next section of samples to the audio device.
static int paCallback(const void*                     /*inputBuffer*/,
                      void*                           outputBuffer,
                      unsigned long                   framesPerBuffer,
                      const PaStreamCallbackTimeInfo* /*timeInfo*/,
                      PaStreamCallbackFlags           /*statusFlags*/,
                      void*                           /*userData*/)
{
    ProcessModifyGeneratorRequests();

    Generator<>& generator = GeneratorAccess::getInstance();
    float*       out       = static_cast<float*>(outputBuffer);

    generator.writeSamples(std::span<float>(out, framesPerBuffer * 2ul));

#if LOG_SESSION_TO_FILE
    Logging::CopyBufferAndDefer(out, framesPerBuffer);
#endif

    return paContinue;
}

int APIENTRY wWinMain(_In_ HINSTANCE    /*hInstance*/,
                     _In_opt_ HINSTANCE /*hPrevInstance*/,
                     _In_ LPWSTR        /*lpCmdLine*/,
                     _In_ int           /*nCmdShow*/)
{
    if (Pa_Initialize() != paNoError)
        return -1;

    PaStream* stream = InitializePAStream(paCallback);
    if (stream == nullptr)
        return -1;

    WaveTables::Initialize();

    if (!InitImGuiRendering())
        return 1;

    // Main loop
    bool done = false;
    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        RenderFrame(
        [stream]()
        {
            // Handle communication from realtime thread
            (void)ThreadCommunication::processDeferredActions();
            GetUIOscillatorView().HandleRealTimeResponse();

            ImGui::Begin("Generator Settings");
            GetUIOscillatorView().Show();
            ImGui::End();

            ImGui::Begin("Debug Info");
            ShowDebugInfo(stream);
            ImGui::End();

#if LOG_SESSION_TO_FILE
            ImGui::Begin("Oscillator Plot");
            auto [logBufferLeft, logBufferRight] = Logging::GetLogBuffers();
            Plotting::DrawOscilatorPlot(logBufferLeft.get(), logBufferRight.get());
            ImGui::End();
#endif
        });
    }

    TearDownWindowRendering();
    TearDownPAStream(stream);

#if LOG_SESSION_TO_FILE
    Logging::WriteSessionToFile();
#endif

    return 0;
}
