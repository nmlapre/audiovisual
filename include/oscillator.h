#pragma once

#include "constants.h"

#include <algorithm>
#include <functional>

constexpr phase_t hz_to_delta(frequency_t hz)
{
    return static_cast<phase_t>(hz * MAX_PHASE_OVER_SAMPLE_RATE + 0.5);
}

enum class OscillatorType
{
    Sine,
    Square,
    Triangle,
    Saw
};

enum class OscillatorState
{
    Uninitialized,
    Active,
    Deactivated,
    // TODO: This fading logic needs to be encapsulated somehow
    FadingOutDeactivate,
    FadingOutRemove,
    FadingIn
};

struct OscillatorSettings
{
    OscillatorSettings() = default;
    OscillatorSettings(OscillatorType type, frequency_t frequency, volume_t volume)
        : state(OscillatorState::Active)
        , type(type)
        , frequency(frequency)
        , volume(volume)
    { }

    OscillatorState state{ OscillatorState::Uninitialized };
    OscillatorType  type{ OscillatorType::Sine };
    frequency_t     frequency{ 0 };
    volume_t        volume{ 0 };     // out of 1.0
    pan_t           pan{ 0.0f };  // in range [-1.0, 1.0]
};

// A Fader is a helper class to lerp between a start and target point.
// Smoothstep also works in lieu of lerp, but I think lerp sounds better.
template <class T, uint16_t FadeLength>
struct Fader
{
    Fader(T initial_value)
        : fade_steps_left(0)
        , start(initial_value)
        , target(initial_value)
    { }

    void fade(T from, T to)
    {
        fade_steps_left = FadeLength;
        start = from;
        target = to;
    }

    T update(std::function<void()> onFadeEnd = nullptr)
    {
        if (fade_steps_left > 0)
        {
            fade_steps_left--;
            if (onFadeEnd && fade_steps_left == 0)
                onFadeEnd();

            return getValue();
        }
        else
        {
            return target;
        }
    }

    T getValue() const
    {
        constexpr float invFadeLength = 1.0f / FadeLength;
        const float t = float(FadeLength - fade_steps_left) * invFadeLength;
        return T(std::lerp(start, target, t));
    }

private:
    uint16_t fade_steps_left{ 0 };
    T start{};
    T target{};
};

// An Oscillator, when activated, outputs an oscillating signal via an updating
// phase step, which is used as an index into a wave table for the specified wave type.
// The Oscillator is responsible for its state, including frequency, phase step, volume,
// pan, and fade. It supports smoothly transitioning between states in a realtime context.
struct Oscillator
{
    Oscillator(OscillatorSettings settings = OscillatorSettings())
        : m_settings(std::move(settings))
        , m_phase_step(hz_to_delta(m_settings.frequency))
        , m_volume_fader(m_settings.volume)
        , m_phase_step_fader(m_phase_step)
        , m_left_pan_fader(1.0f)
        , m_right_pan_fader(1.0f)
        //m_wave_type_fader(1.0f)
    {
        assert(m_settings.volume >= 0 && m_settings.volume <= 1.0);

        // First invocation of updatePhase should return zero. Go back one to allow that.
        m_phase_counter -= m_phase_step;
    }

    // Designed to be called in a loop...
    __forceinline phase_t updatePhase()
    {
        m_phase_step = m_phase_step_fader.update();
        m_phase_counter += m_phase_step;
        return m_phase_counter == TABLE_SIZE ? 0 : m_phase_counter;
    }

    // Designed to be called in a loop...
    __forceinline volume_t updateVolume()
    {
        auto onVolumeFadeEnd = [this]
        {
            if (m_settings.state == OscillatorState::FadingIn)
                m_settings.state = OscillatorState::Active;
            else if (m_settings.state == OscillatorState::FadingOutDeactivate)
                m_settings.state = OscillatorState::Deactivated;
            else if (m_settings.state == OscillatorState::FadingOutRemove)
                reset();
        };

        return m_settings.volume = m_volume_fader.update(onVolumeFadeEnd);
    }

    std::tuple<float, float> updatePan()
    {
        return { m_left_pan_fader.update(), m_right_pan_fader.update() };
    }

    void activate(volume_t volume) { fadeIn(volume); }
    void deactivate(bool remove) { fadeOut(remove); }

    void setFrequency(frequency_t frequency)
    {
        m_phase_step_fader.fade(m_phase_step_fader.getValue(), hz_to_delta(frequency));
        m_settings.frequency = frequency;
    }

    void setVolume(volume_t volume)
    {
        if (isActive())
            fade(m_volume_fader.getValue(), volume, OscillatorState::Active);
        else
            m_settings.volume = volume;
    }

    void setPan(pan_t pan)
    {
        // Calculate current pan, target pan for L and R.
        // Fade each of them so we don't get a jarring discontinuity.
        float currentLeftPan = m_left_pan_fader.getValue();
        float currentRightPan = m_right_pan_fader.getValue();

        float targetLeftPan = 1.0f;
        float targetRightPan = 1.0f;
        if (pan < 0.0f)
            targetRightPan = 1.0f - std::abs(pan);
        else if (pan > 0.0f)
            targetLeftPan = 1.0f - pan;

        m_left_pan_fader.fade(currentLeftPan, targetLeftPan);
        m_right_pan_fader.fade(currentRightPan, targetRightPan);
        m_settings.pan = pan;
    }

    void setType(OscillatorType type)
    {
        m_settings.type = type;
    }

    __forceinline OscillatorState getState()      const { return m_settings.state; }
    __forceinline OscillatorType  getType()       const { return m_settings.type; }
    __forceinline frequency_t     getFrequency()  const { return m_settings.frequency; }
    __forceinline volume_t        getVolume()     const { return m_settings.volume; }
    __forceinline pan_t           getPan()        const { return m_settings.pan; }
    __forceinline phase_t         getPhaseStep()  const { return m_phase_step; }
    __forceinline bool            isInitialized() const { return m_settings.state != OscillatorState::Uninitialized; }
    __forceinline bool            isActive()      const { return m_settings.state == OscillatorState::Active              ||
                                                                 m_settings.state == OscillatorState::FadingIn            ||
                                                                 m_settings.state == OscillatorState::FadingOutDeactivate ||
                                                                 m_settings.state == OscillatorState::FadingOutRemove; }

    void reset() { *this = Oscillator(); }

    void fade(volume_t start, volume_t target, OscillatorState state)
    {
        m_settings.state = state;
        m_volume_fader.fade(start, target);
        m_settings.volume = start;
    }

    void fadeIn(volume_t target)
    {
        // Set up the volume fade, starting at 0 and targeting the existing volume.
        fade(0.0f, target, OscillatorState::FadingIn);
    }

    void fadeOut(bool remove)
    {
        // Set up the volume fade, starting at the current volume and targeting 0.
        if (remove)
            fade(m_settings.volume, 0.0f, OscillatorState::FadingOutRemove);
        else
            fade(m_settings.volume, 0.0f, OscillatorState::FadingOutDeactivate);
    }

private:
    OscillatorSettings m_settings;

    // Counter will wrap around at UINT16_MAX back to 0.
    phase_t m_phase_counter{ 0 };
    phase_t m_phase_step{ 0 };

    // Automatically fade volume after a volume change.
    // This helps avoid discontinuities at sample chunk boundaries.
    // The fade length should probably be configurable.
    static constexpr uint16_t VolumeFadeLength{ 256 };
    Fader<volume_t, VolumeFadeLength> m_volume_fader;

    // Automatically fade frequency after a frequency change.
    static constexpr uint16_t PhaseFadeLength{ 256 };
    Fader<phase_t, PhaseFadeLength> m_phase_step_fader;

    // Automatically fade the pan values after a pan change.
    static constexpr uint16_t PanFadeLength{ 256 };
    Fader<pan_t, PanFadeLength> m_left_pan_fader;
    Fader<pan_t, PanFadeLength> m_right_pan_fader;

    // TODO: support lerp between waves
    //static constexpr uint16_t WaveTypeFadeLength {256};
    //Fader<float, WaveTypeFadeLength> m_wave_type_fader;
};

// A collection of oscillators, this represents the state of a single generator.
// It supports adding, removing, [de]activating, and changing settings on its member
// oscillators. The oscillator count is fixed to avoid any allocation, as these
// settings are modified by the realtime thread. Oscillator ids provide a handle
// for the UI thread to use in identifying oscillators.
template<uint8_t MAX_OSCILLATORS>
struct Oscillators
{
    std::optional<OscillatorId> addOscillator(OscillatorSettings settings)
    {
        return addOscillator(Oscillator(std::move(settings)));
    }

    std::optional<OscillatorId> addOscillator(Oscillator oscillator)
    {
        auto const id = getNextOscillatorId();
        if (!id.has_value())
            return std::nullopt;

        oscillator.fadeIn(oscillator.getVolume());
        m_oscillators.at(id.value()) = std::move(oscillator);
        return id;
    }

    // Remove the oscillator at the given id.
    // Returns false if the given oscillator id doesn't exist.
    bool removeOscillator(OscillatorId id)
    {
        auto& oscillator = m_oscillators.at(id);
        if (!oscillator.isInitialized())
            return false;

        oscillator.deactivate(true);
        return true;
    }

    void removeAllOscillators()
    {
        for (auto& oscillator : m_oscillators)
            oscillator.deactivate(true);
    }

    // Activate the oscillator at the given id.
    // Returns false if the given oscillator id doesn't exist.
    bool activateOscillator(OscillatorId id, volume_t volume)
    {
        auto& oscillator = m_oscillators.at(id);
        if (!oscillator.isInitialized())
            return false;

        oscillator.activate(volume);
        return true;
    }

    // Deactivate the oscillator at the given id.
    // Returns false if the given oscillator id doesn't exist.
    bool deactivateOscillator(OscillatorId id)
    {
        auto& oscillator = m_oscillators.at(id);
        if (!oscillator.isInitialized())
            return false;

        oscillator.deactivate(false);
        return true;
    }

    bool setFrequency(OscillatorId id, frequency_t frequency)
    {
        auto& oscillator = m_oscillators.at(id);
        if (!oscillator.isInitialized())
            return false;

        oscillator.setFrequency(frequency);
        return true;
    }

    bool setVolume(OscillatorId id, volume_t volume)
    {
        auto& oscillator = m_oscillators.at(id);
        if (!oscillator.isInitialized())
            return false;

        oscillator.setVolume(volume);
        return true;
    }

    bool setPan(OscillatorId id, pan_t pan)
    {
        auto& oscillator = m_oscillators.at(id);
        if (!oscillator.isInitialized())
            return false;

        oscillator.setPan(pan);
        return true;
    }

    bool setType(OscillatorId id, OscillatorType type)
    {
        auto& oscillator = m_oscillators.at(id);
        if (!oscillator.isInitialized())
            return false;

        oscillator.setType(type);
        return true;
    }

    size_t getMaxSize() const { return m_oscillators.max_size(); }
    size_t countActiveOscillators() const
    {
        return std::count_if(m_oscillators.cbegin(), m_oscillators.cend(),
            [](auto const& osc) { return osc.isActive(); });
    }

    auto begin() { return m_oscillators.begin(); }
    auto end() { return m_oscillators.end(); }
    auto cbegin() const { return m_oscillators.cbegin(); }
    auto cend()   const { return m_oscillators.cend(); }

private:
    // Returns the lowest index possible that contains an uninitialized oscillator.
    std::optional<OscillatorId> getNextOscillatorId() const
    {
        // Return the next key that isn't in use already.
        for (uint8_t oscIndex = 0; oscIndex < m_oscillators.size(); ++oscIndex)
        {
            auto const& oscillator = m_oscillators.at(oscIndex);
            if (oscillator.getState() != OscillatorState::Uninitialized)
                continue;

            return oscIndex;
        }
        return std::nullopt;
    }

    bool oscillatorIsActive(OscillatorId id) const
    {
        if (m_oscillators.at(id).getState() != OscillatorState::Uninitialized)
            return true;

        return false;
    }

    std::array<Oscillator, MAX_OSCILLATORS> m_oscillators;
};
