#pragma once

#include "gcem.hpp"

#include <array>
#include <cassert>
#include <optional>

// Math values
#pragma warning(suppress: 4244) // suppress gcem MVSC warning re: possible loss of data
static constexpr double const PI = gcem::acos(-1);
static constexpr double TWO_PI = 2 * PI;
static constexpr double ONE_OVER_UINT16_MAX = 1.0 / UINT16_MAX;

// My attempt to build wave tables at compile time. This can work, but MVSC
// just doesn't have the chops right now to compile this. On my underpowered
// laptop with 8GB of RAM, MVSC runs out of memory. GCC can handle it :)
constexpr auto csin(const double x)
{
    return gcem::sin(x);
};

template<class T, size_t N, class F>
constexpr auto make_table(F fn, size_t start)
{
    std::array<T, N> a{};
    for (size_t i = 0, val = start; i < N; ++i, ++val)
        a[i] = fn(val * TWO_PI * ONE_OVER_UINT16_MAX);

    return a;
}

// Out of heap space...need more than 8GB RAM to build this constexpr.
// The same compiles in 15 seconds on GCC10.2, and uses ~500MB RAM, lol.
//static constexpr auto SIN_TABLE = make_table<double, UINT16_MAX>(csin, 0);

// Notes
namespace Notes
{
    // TODO: Add ability to switch between just-intonated keys.
    static constexpr double C_4_JUST   = 261.63;
    static constexpr double E_4_JUST   = 327.03;
    static constexpr double G_4_JUST   = 392.44;
    static constexpr double B_4_JUST   = 490.55;

    static constexpr double C_0        = 16.35;
    static constexpr double C_SHARP_0  = 17.32;
    static constexpr double D_FLAT_0   = 17.32;
    static constexpr double D_0        = 18.35;
    static constexpr double D_SHARP_0  = 19.45;
    static constexpr double E_FLAT_0   = 19.45;
    static constexpr double E_0        = 20.6;
    static constexpr double F_0        = 21.83;
    static constexpr double F_SHARP_0  = 23.12;
    static constexpr double G_FLAT_0   = 23.12;
    static constexpr double G_0        = 24.5;
    static constexpr double G_SHARP_0  = 25.96;
    static constexpr double A_FLAT_0   = 25.96;
    static constexpr double A_0        = 27.5;
    static constexpr double A_SHARP_0  = 29.14;
    static constexpr double B_FLAT_0   = 29.14;
    static constexpr double B_0        = 30.87;
    static constexpr double C_1        = 32.7;
    static constexpr double C_SHARP_1  = 34.65;
    static constexpr double D_FLAT_1   = 34.65;
    static constexpr double D_1        = 36.71;
    static constexpr double D_SHARP_1  = 38.89;
    static constexpr double E_FLAT_1   = 38.89;
    static constexpr double E_1        = 41.2;
    static constexpr double F_1        = 43.65;
    static constexpr double F_SHARP_1  = 46.25;
    static constexpr double G_FLAT_1   = 46.25;
    static constexpr double G_1        = 49.0;
    static constexpr double G_SHARP_1  = 51.91;
    static constexpr double A_FLAT_1   = 51.91;
    static constexpr double A_1        = 55.0;
    static constexpr double A_SHARP_1  = 58.27;
    static constexpr double B_FLAT_1   = 58.27;
    static constexpr double B_1        = 61.74;
    static constexpr double C_2        = 65.41;
    static constexpr double C_SHARP_2  = 69.3;
    static constexpr double D_FLAT_2   = 69.3;
    static constexpr double D_2        = 73.42;
    static constexpr double D_SHARP_2  = 77.78;
    static constexpr double E_FLAT_2   = 77.78;
    static constexpr double E_2        = 82.41;
    static constexpr double F_2        = 87.31;
    static constexpr double F_SHARP_2  = 92.5;
    static constexpr double G_FLAT_2   = 92.5;
    static constexpr double G_2        = 98.0;
    static constexpr double G_SHARP_2  = 103.83;
    static constexpr double A_FLAT_2   = 103.83;
    static constexpr double A_2        = 110.0;
    static constexpr double A_SHARP_2  = 116.54;
    static constexpr double B_FLAT_2   = 116.54;
    static constexpr double B_2        = 123.47;
    static constexpr double C_3        = 130.81;
    static constexpr double C_SHARP_3  = 138.59;
    static constexpr double D_FLAT_3   = 138.59;
    static constexpr double D_3        = 146.83;
    static constexpr double D_SHARP_3  = 155.56;
    static constexpr double E_FLAT_3   = 155.56;
    static constexpr double E_3        = 164.81;
    static constexpr double F_3        = 174.61;
    static constexpr double F_SHARP_3  = 185.0;
    static constexpr double G_FLAT_3   = 185.0;
    static constexpr double G_3        = 196.0;
    static constexpr double G_SHARP_3  = 207.65;
    static constexpr double A_FLAT_3   = 207.65;
    static constexpr double A_3        = 220.0;
    static constexpr double A_SHARP_3  = 233.08;
    static constexpr double B_FLAT_3   = 233.08;
    static constexpr double B_3        = 246.94;
    static constexpr double C_4        = 261.63;
    static constexpr double C_SHARP_4  = 277.18;
    static constexpr double D_FLAT_4   = 277.18;
    static constexpr double D_4        = 293.66;
    static constexpr double D_SHARP_4  = 311.13;
    static constexpr double E_FLAT_4   = 311.13;
    static constexpr double E_4        = 329.63;
    static constexpr double F_4        = 349.23;
    static constexpr double F_SHARP_4  = 369.99;
    static constexpr double G_FLAT_4   = 369.99;
    static constexpr double G_4        = 392.0;
    static constexpr double G_SHARP_4  = 415.3;
    static constexpr double A_FLAT_4   = 415.3;
    static constexpr double A_4        = 440.0;
    static constexpr double A_SHARP_4  = 466.16;
    static constexpr double B_FLAT_4   = 466.16;
    static constexpr double B_4        = 493.88;
    static constexpr double C_5        = 523.25;
    static constexpr double C_SHARP_5  = 554.37;
    static constexpr double D_FLAT_5   = 554.37;
    static constexpr double D_5        = 587.33;
    static constexpr double D_SHARP_5  = 622.25;
    static constexpr double E_FLAT_5   = 622.25;
    static constexpr double E_5        = 659.25;
    static constexpr double F_5        = 698.46;
    static constexpr double F_SHARP_5  = 739.99;
    static constexpr double G_FLAT_5   = 739.99;
    static constexpr double G_5        = 783.99;
    static constexpr double G_SHARP_5  = 830.61;
    static constexpr double A_FLAT_5   = 830.61;
    static constexpr double A_5        = 880.0;
    static constexpr double A_SHARP_5  = 932.33;
    static constexpr double B_FLAT_5   = 932.33;
    static constexpr double B_5        = 987.77;
    static constexpr double C_6        = 1046.5;
    static constexpr double C_SHARP_6  = 1108.73;
    static constexpr double D_FLAT_6   = 1108.73;
    static constexpr double D_6        = 1174.66;
    static constexpr double D_SHARP_6  = 1244.51;
    static constexpr double E_FLAT_6   = 1244.51;
    static constexpr double E_6        = 1318.51;
    static constexpr double F_6        = 1396.91;
    static constexpr double F_SHARP_6  = 1479.98;
    static constexpr double G_FLAT_6   = 1479.98;
    static constexpr double G_6        = 1567.98;
    static constexpr double G_SHARP_6  = 1661.22;
    static constexpr double A_FLAT_6   = 1661.22;
    static constexpr double A_6        = 1760.0;
    static constexpr double A_SHARP_6  = 1864.66;
    static constexpr double B_FLAT_6   = 1864.66;
    static constexpr double B_6        = 1975.53;
    static constexpr double C_7        = 2093.0;
    static constexpr double C_SHARP_7  = 2217.46;
    static constexpr double D_FLAT_7   = 2217.46;
    static constexpr double D_7        = 2349.32;
    static constexpr double D_SHARP_7  = 2489.02;
    static constexpr double E_FLAT_7   = 2489.02;
    static constexpr double E_7        = 2637.02;
    static constexpr double F_7        = 2793.83;
    static constexpr double F_SHARP_7  = 2959.96;
    static constexpr double G_FLAT_7   = 2959.96;
    static constexpr double G_7        = 3135.96;
    static constexpr double G_SHARP_7  = 3322.44;
    static constexpr double A_FLAT_7   = 3322.44;
    static constexpr double A_7        = 3520.0;
    static constexpr double A_SHARP_7  = 3729.31;
    static constexpr double B_FLAT_7   = 3729.31;
    static constexpr double B_7        = 3951.07;
    static constexpr double C_8        = 4186.01;
    static constexpr double C_SHARP_8  = 4434.92;
    static constexpr double D_FLAT_8   = 4434.92;
    static constexpr double D_8        = 4698.63;
    static constexpr double D_SHARP_8  = 4978.03;
    static constexpr double E_FLAT_8   = 4978.03;
    static constexpr double E_8        = 5274.04;
    static constexpr double F_8        = 5587.65;
    static constexpr double F_SHARP_8  = 5919.91;
    static constexpr double G_FLAT_8   = 5919.91;
    static constexpr double G_8        = 6271.93;
    static constexpr double G_SHARP_8  = 6644.88;
    static constexpr double A_FLAT_8   = 6644.88;
    static constexpr double A_8        = 7040.0;
    static constexpr double A_SHARP_8  = 7458.62;
    static constexpr double B_FLAT_8   = 7458.62;
    static constexpr double B_8        = 7902.13;
}

// Constants
constexpr unsigned int CHANNEL_COUNT_MONO = 1;
constexpr unsigned int CHANNEL_COUNT_STEREO = 2;
constexpr unsigned int SAMPLE_RATE_44_1_KHZ = 44100; // 44.1 kHz
constexpr double MAX_PHASE = static_cast<double>(UINT16_MAX);
constexpr double ONE_OVER_PI = 1. / PI;
constexpr double TWO_OVER_PI = 2. / PI;

// Options
constexpr unsigned int CHANNEL_COUNT = CHANNEL_COUNT_MONO;
constexpr unsigned int SAMPLE_RATE = SAMPLE_RATE_44_1_KHZ;

// Constants derived from options
constexpr double ONE_OVER_SAMPLE_RATE = 1. / SAMPLE_RATE;
constexpr double ONE_OVER_MAX_PHASE = 1. / MAX_PHASE;
constexpr double MAX_PHASE_OVER_SAMPLE_RATE = MAX_PHASE / SAMPLE_RATE;
constexpr double ONE_OVER_MAX_PHASE_X_TWO_PI = ONE_OVER_MAX_PHASE * TWO_PI;

// Vocabulary types
struct Oscillator;

using frequency_t  = float;
using volume_t     = float;
using pan_t        = float;
using phase_t      = uint16_t;
using time_step_t  = size_t;
using OscillatorId = uint8_t;

// Wave tables: these need multiplying by amplitude at runtime.
constexpr size_t TABLE_SIZE = UINT16_MAX;
struct WaveTables
{
    // Call on startup to fill up the wave tables above.
    static void Initialize();

    static std::array<float, TABLE_SIZE>& getSine();
    static std::array<float, TABLE_SIZE>& getSquare();
    static std::array<float, TABLE_SIZE>& getTriangle();
    static std::array<float, TABLE_SIZE>& getSaw();
};
