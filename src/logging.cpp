#include "logging.h"

#include "thread_communication.h"
#include "AudioFile.h"

namespace Logging
{

std::pair<
    std::reference_wrapper<std::vector<float>>,
    std::reference_wrapper<std::vector<float>>>
GetLogBuffers()
{
    static std::vector<float> logBufferLeft;
    static std::vector<float> logBufferRight;
    return { logBufferLeft, logBufferRight };
}

void WriteToLogBuffer(const std::vector<float>& samplesLeft, const std::vector<float>& samplesRight)
{
    auto [logBufferLeft, logBufferRight] = GetLogBuffers();

    for (auto& sample : samplesLeft)
        logBufferLeft.get().push_back(sample);

    for (auto& sample : samplesRight)
        logBufferRight.get().push_back(sample);
}

void WriteSessionToFile()
{
    auto [logBufferLeft, logBufferRight] = GetLogBuffers();

    AudioFile<float>::AudioBuffer buffer;
    buffer.resize(2);
    buffer.at(0) = std::move(logBufferLeft.get());
    buffer.at(1) = std::move(logBufferRight.get());

    AudioFile<float> audioFile;
    audioFile.setNumChannels(2);
    audioFile.setSampleRate(44100);
    audioFile.setAudioBuffer(buffer);
    audioFile.save("test.wav");
}

void CopyBufferAndDefer(const float* out, unsigned long framesPerBuffer)
{
    std::vector<float> floatsLeft;
    std::vector<float> floatsRight;
    floatsLeft.reserve(framesPerBuffer); // allocating on the realtime thread! bad!
    floatsRight.reserve(framesPerBuffer); // allocating on the realtime thread! bad!
    for (unsigned index = 0; index < framesPerBuffer; ++index)
    {
        floatsLeft.push_back(*out++);
        floatsRight.push_back(*out++);
    }

    ThreadCommunication::deferToNonRealtimeThread(
        [samplesLeft = std::move(floatsLeft), samplesRight = std::move(floatsRight)]() {
            WriteToLogBuffer(samplesLeft, samplesRight);
        });
}

}
