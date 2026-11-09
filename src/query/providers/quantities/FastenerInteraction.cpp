#include "oor/query/providers/quantities/FastenerInteraction.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// An exponent that is a small whole number (2 and 3 by default): its powers are products.
bool isWhole(double exponent) noexcept
{
    return exponent >= 1.0 && exponent <= 16.0 && exponent == std::floor(exponent);
}

double power(double value, int exponent) noexcept
{
    double result = 1.0;
    for (int index = 0; index < exponent; ++index)
        result *= value;
    return result;
}

// value^exponent, by products for a whole exponent.
double raised(double value, double exponent) noexcept
{
    return isWhole(exponent) ? power(value, static_cast<int>(exponent)) : std::pow(value, exponent);
}

} // namespace

double fastenerInteraction(double tension, double shear, double tensionAllowable, double shearAllowable,
                           double tensionExponent, double shearExponent) noexcept
{
    return raised(tension / tensionAllowable, tensionExponent) + raised(shear / shearAllowable, shearExponent);
}

double fastenerLoadMultiplier(double tension, double shear, double tensionAllowable, double shearAllowable,
                              double tensionExponent, double shearExponent) noexcept
{
    if (!(tension > 0.0) && !(shear > 0.0))
        return NOT_A_NUMBER;
    // The interaction at the factor k is A k^a + B k^b, A = x^a and B = y^b with x = T / Ta and y = S / Sa.
    const double x = tension / tensionAllowable;
    const double y = shear / shearAllowable;
    const double first = raised(x, tensionExponent);
    const double second = raised(y, shearExponent);
    if (std::isnan(first) || std::isnan(second) || std::isinf(first) || std::isinf(second))
        return std::isinf(first) || std::isinf(second) ? 0.0 : NOT_A_NUMBER;
    // one load alone, or equal exponents: closed forms
    if (!(second > 0.0))
        return std::pow(first, -1.0 / tensionExponent);
    if (!(first > 0.0))
        return std::pow(second, -1.0 / shearExponent);
    if (tensionExponent == shearExponent)
        return std::pow(first + second, -1.0 / tensionExponent);
    // Whole exponents: Newton on A k^a + B k^b - 1, a polynomial increasing and convex for k > 0, from the smaller of
    // the roots of the two terms alone, 1 / max(x, y), where it is positive: the iterates come down to the root, with
    // products only.
    if (isWhole(tensionExponent) && isWhole(shearExponent) && x > 0.0 && y > 0.0)
    {
        const int m = static_cast<int>(tensionExponent);
        const int n = static_cast<int>(shearExponent);
        double k = 1.0 / std::max(x, y);
        for (int iteration = 0; iteration < 60; ++iteration)
        {
            const double km = power(k, m - 1);
            const double kn = power(k, n - 1);
            const double value = first * km * k + second * kn * k - 1.0;
            const double slope = tensionExponent * first * km + shearExponent * second * kn;
            const double step = value / slope;
            k -= step;
            if (!(std::fabs(step) > 1.0e-15 * k))
                break;
        }
        return k;
    }
    // Newton on t = ln k: h(t) = ln(A e^(a t) + B e^(b t)) is increasing and convex, so from the smaller of the roots
    // of the two terms alone (where h > 0) the iterates come down to the root without passing it.
    const double logFirst = std::log(first);
    const double logSecond = std::log(second);
    double t = std::min(-logFirst / tensionExponent, -logSecond / shearExponent);
    for (int iteration = 0; iteration < 60; ++iteration)
    {
        const double termFirst = std::exp(logFirst + tensionExponent * t);
        const double termSecond = std::exp(logSecond + shearExponent * t);
        const double sum = termFirst + termSecond;
        const double slope = (tensionExponent * termFirst + shearExponent * termSecond) / sum;
        const double step = std::log(sum) / slope;
        t -= step;
        if (!(std::fabs(step) > 1.0e-15 * std::max(1.0, std::fabs(t))))
            break;
    }
    return std::exp(t);
}

} // namespace oor::query
