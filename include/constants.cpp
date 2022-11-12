#include "constants.h"

#include <cmath>

void WaveTables::Initialize()
{
    auto& sine = getSine();
    auto& square = getSquare();
    auto& triangle = getTriangle();
    auto& saw = getSaw();
    for (size_t i = 0; i < TABLE_SIZE; i++)
    {
        double const phaseAtTime = std::sin(double(i) * ONE_OVER_MAX_PHASE_X_TWO_PI);
        sine[i] = float(phaseAtTime);
        square[i] = phaseAtTime >= 0 ? .5f : -.5f;
        triangle[i] = float(TWO_OVER_PI * std::asin(phaseAtTime));
        saw[i] = float(i * ONE_OVER_MAX_PHASE * 2. - 1.);  // not sure this is exactly the right shape yet lol. it's pretty loud

        // saw that starts at 0:
        //float const progress = i * ONE_OVER_MAX_PHASE;
        //saw     [i] = (progress * 2.f) <= 1.0f ? (progress * 2.0f) : -2.0f + (progress * 2.0f);
    }
}

std::array<float, TABLE_SIZE>& WaveTables::getSine()
{
    static std::array<float, TABLE_SIZE> sine;
    return sine;
}

std::array<float, TABLE_SIZE>& WaveTables::getSquare()
{
    static std::array<float, TABLE_SIZE> square;
    return square;
}

std::array<float, TABLE_SIZE>& WaveTables::getTriangle()
{
    static std::array<float, TABLE_SIZE> triangle;
    return triangle;
}

std::array<float, TABLE_SIZE>& WaveTables::getSaw()
{
    static std::array<float, TABLE_SIZE> saw;
    return saw;
}
