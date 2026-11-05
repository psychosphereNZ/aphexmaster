#pragma once
#include <cmath>

// Simple double-precision biquad (transposed direct form II), one channel.
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    double z1 = 0, z2 = 0;

    void reset() { z1 = z2 = 0; }

    inline double process (double x)
    {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

    void setHighPass (double fs, double f, double q = 0.7071067811865476)
    {
        const double w = 2.0 * 3.141592653589793 * f / fs;
        const double cw = std::cos (w), sw = std::sin (w);
        const double alpha = sw / (2.0 * q);
        const double a0 = 1.0 + alpha;
        b0 = ((1.0 + cw) * 0.5) / a0;
        b1 = -(1.0 + cw) / a0;
        b2 = ((1.0 + cw) * 0.5) / a0;
        a1 = (-2.0 * cw) / a0;
        a2 = (1.0 - alpha) / a0;
    }
};
