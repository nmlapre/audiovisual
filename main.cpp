// AudioVisual.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "audiovisual.h"
#include "logging.h"
#include "oscillator_ui.h"
#include "plotting.h"
#include "windowing.h"

// Forward declarations (this lib)
static int paCallback(const void*                     /*inputBuffer*/,
                      void*                           outputBuffer,
                      unsigned long                   framesPerBuffer,
                      const PaStreamCallbackTimeInfo* /*timeInfo*/,
                      PaStreamCallbackFlags           /*statusFlags*/,
                      void*                           /*userData*/);

PaStream* InitializePAStream()
{
    PaHostApiIndex const numAPIs = Pa_GetHostApiCount();
    if (numAPIs < 0)
        return nullptr;

    PaHostApiInfo const* hostApiInfo;
    PaDeviceIndex devIndex = 0;
    for (PaHostApiIndex i = 0; i < numAPIs; i++)
    {
        hostApiInfo = Pa_GetHostApiInfo(i);
        //if (hostApiInfo && hostApiInfo->type == paASIO)
        if (hostApiInfo && hostApiInfo->type == paWASAPI)
        {
            devIndex = hostApiInfo->defaultOutputDevice;
            break;
        }
    }

    // WASAPI-specific stream parameters
    auto wasapiStreamInfo = std::make_unique<PaWasapiStreamInfo>();
    {
        wasapiStreamInfo->size = sizeof(PaWasapiStreamInfo);
        wasapiStreamInfo->hostApiType = PaHostApiTypeId::paWASAPI;
        wasapiStreamInfo->version = 1;
        //wasapiStreamInfo->flags = paWinWasapiExclusive; // requires software limiting. watch your ears :)
        wasapiStreamInfo->flags = paWinWasapiThreadPriority;
        wasapiStreamInfo->threadPriority = PaWasapiThreadPriority::eThreadPriorityProAudio;
        wasapiStreamInfo->streamCategory = PaWasapiStreamCategory::eAudioCategoryGameMedia;
        wasapiStreamInfo->streamOption = PaWasapiStreamOption::eStreamOptionRaw;
    }

    // ASIO-specific stream parameters. Use if using ASIO.
    //auto asioStreamInfo = std::make_unique<PaAsioStreamInfo>();
    //struct IntTwo { int data[2]{ 0, 1 }; };
    //auto channelSelectors = std::make_unique<IntTwo>();
    //{
    //    asioStreamInfo->size = sizeof(PaAsioStreamInfo);
    //    asioStreamInfo->hostApiType = paASIO;
    //    asioStreamInfo->version = 1;
    //    asioStreamInfo->flags = paAsioUseChannelSelectors;
    //    asioStreamInfo->channelSelectors = (int*)&channelSelectors->data;
    //}

    PaStreamParameters outputParameters;
    outputParameters.channelCount = 2;
    outputParameters.device = devIndex;
    outputParameters.hostApiSpecificStreamInfo = wasapiStreamInfo.get();
    //outputParameters.hostApiSpecificStreamInfo = asioStreamInfo.get();
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;

    PaError err;
    PaStream* stream;
    err = Pa_OpenStream(&stream,
        NULL,
        &outputParameters,
        SAMPLE_RATE,
        paFramesPerBufferUnspecified,
        paClipOff,
        paCallback,
        nullptr);
    assert(err == PaErrorCode::paNoError);
    err = Pa_StartStream(stream);
    assert(err == PaErrorCode::paNoError);
    
    return stream;
}

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

    WaveTables::Initialize();

    PaStream* stream = InitializePAStream();
    if (stream == nullptr)
        return -1;

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

    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();

#if LOG_SESSION_TO_FILE
    Logging::WriteSessionToFile();
#endif

    return 0;
}
