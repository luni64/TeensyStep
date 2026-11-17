// Comparison of floating point values in the tests: approx(x).epsilon(e) accepts values within the fraction e
// of x. (doctest::Approx alone adds an absolute tolerance of e * 1, which makes comparisons of small values,
// such as times in seconds, far too generous.)
#pragma once

#include "doctest.h"

inline doctest::Approx approx(double value)
{
    return doctest::Approx(value).scale(1e-30); // not 0, so that 0 compares equal to 0
}
