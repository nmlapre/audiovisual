#pragma once

#include <span>

#include "oscillator.h"
#include "util.h"

template<size_t MAX_OSCILLATORS = 8>
struct Generator
{
    void writeSamples(std::span<float> outputView)
    {
        // Zero out the buffer before adding any sample values.
        for (float& sample : outputView)
            sample = 0.0f;

        for (Oscillator& oscillator : m_oscillators)
        {
            if (!oscillator.isActive()) continue;

            // Write all samples for a given oscillator at once.
            switch (oscillator.getType())
            {
                case OscillatorType::Sine:
                    generateOscillatorValues(outputView, oscillator, sine);
                    continue;
                case OscillatorType::Square:
                    generateOscillatorValues(outputView, oscillator, square);
                    continue;
                case OscillatorType::Triangle:
                    generateOscillatorValues(outputView, oscillator, triangle);
                    continue;
                case OscillatorType::Saw:
                    generateOscillatorValues(outputView, oscillator, saw);
                    continue;
                default:
                    assert(false); // Unknown oscillator type!
                    break;
            }
        }
        
        // Hard clipping - useful for saving ears during testing.
        for (float& sample : outputView)
        {
            sample = std::min(sample, 1.0f);
            sample = std::max(sample, -1.0f);
        }
    }

    __forceinline Oscillators<MAX_OSCILLATORS>& getOscillators() { return m_oscillators; }

private:
    void generateOscillatorValues(std::span<float>& output, Oscillator& oscillator, float(&table)[TABLE_SIZE])
    {
        for (size_t index = 0; index < output.size(); index += 2)
        {
            const auto [leftPan, rightPan] = oscillator.updatePan();
            const phase_t phase = oscillator.updatePhase();
            const volume_t volume = oscillator.updateVolume();
            output[index]     += table[phase] * volume * leftPan;  // left channel
            output[index + 1] += table[phase] * volume * rightPan; // right channel
        }
    }

    Oscillators<MAX_OSCILLATORS> m_oscillators;
};
