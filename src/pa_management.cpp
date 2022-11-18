#include "pa_management.h"

#include "constants.h"

#include "pa_win_wasapi.h"

#include <memory>

// TODO: I think the stuff in this function should be broken up into an api
// that handles audio api selection. I'd really like to be able to reuse code
// for both initial audio api selection and dynamic switching between audio
// apis, particularly so that people can choose whatever api they like from some
// automatically-generated list. I think this might require splitting the
// initialization code from the api selection code, particularly so the code can
// stop and start portaudio streams at will. but idk honestly. is anything knowable?
PaStream* InitializePAStream(PaCallbackT paCallback)
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

void TearDownPAStream(PaStream* stream)
{
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
}
