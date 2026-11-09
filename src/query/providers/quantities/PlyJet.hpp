#pragma once

#include <array>
#include <cmath>
#include <cstddef>

// Forward-mode derivatives of a scalar function of a few variables (the stresses of a ply and, for the second
// derivatives of LaRC03, the angle of its fracture plane): a value with its first derivatives in `Count` variables and,
// with `Second`, its second derivatives (the upper triangle, row by row). The arithmetic is that of the chain rule
// written out, so a function written once for `double` gives its derivatives exactly (to rounding), with no difference.

namespace oor::query
{

template <std::size_t Count, bool Second>
struct PlyJet
{
    static constexpr std::size_t PAIRS = Second ? Count * (Count + 1) / 2 : 0;
    double v = 0.0;
    std::array<double, Count> g{};
    std::array<double, PAIRS> h{};

    PlyJet() = default;
    // A constant (no derivative), so that a literal mixes with a jet.
    PlyJet(double value) : v(value) {} // NOLINT(google-explicit-constructor)
    // The variable `index` at `value`, or a value whose derivatives are `direction` times those of the variables
    // (a directional derivative: Count = 1, `index` 0 and `slope` the rate along the direction).
    static PlyJet variable(double value, std::size_t index, double slope = 1.0)
    {
        PlyJet result(value);
        result.g[index] = slope;
        return result;
    }
    // The second derivative of the variables `i` and `j`.
    [[nodiscard]] double second(std::size_t i, std::size_t j) const noexcept
    {
        if (i > j)
            std::swap(i, j);
        return h[i * Count - i * (i + 1) / 2 + j];
    }
};

// The value of a number or of a jet, for the comparisons and the branches.
inline double valueOf(double x) noexcept
{
    return x;
}
template <std::size_t N, bool S>
double valueOf(const PlyJet<N, S>& x) noexcept
{
    return x.v;
}

// phi(u) for a jet u, given phi, phi' and phi'' at u.v.
template <std::size_t N, bool S>
PlyJet<N, S> composed(const PlyJet<N, S>& u, double value, double first, double second)
{
    PlyJet<N, S> result(value);
    for (std::size_t i = 0; i < N; ++i)
        result.g[i] = first * u.g[i];
    if constexpr (S)
    {
        std::size_t at = 0;
        for (std::size_t i = 0; i < N; ++i)
            for (std::size_t j = i; j < N; ++j, ++at)
                result.h[at] = first * u.h[at] + second * u.g[i] * u.g[j];
    }
    return result;
}

template <std::size_t N, bool S>
PlyJet<N, S> operator+(const PlyJet<N, S>& a, const PlyJet<N, S>& b)
{
    PlyJet<N, S> result(a.v + b.v);
    for (std::size_t i = 0; i < N; ++i)
        result.g[i] = a.g[i] + b.g[i];
    for (std::size_t k = 0; k < PlyJet<N, S>::PAIRS; ++k)
        result.h[k] = a.h[k] + b.h[k];
    return result;
}

template <std::size_t N, bool S>
PlyJet<N, S> operator-(const PlyJet<N, S>& a)
{
    PlyJet<N, S> result(-a.v);
    for (std::size_t i = 0; i < N; ++i)
        result.g[i] = -a.g[i];
    for (std::size_t k = 0; k < PlyJet<N, S>::PAIRS; ++k)
        result.h[k] = -a.h[k];
    return result;
}

template <std::size_t N, bool S>
PlyJet<N, S> operator-(const PlyJet<N, S>& a, const PlyJet<N, S>& b)
{
    return a + (-b);
}

template <std::size_t N, bool S>
PlyJet<N, S> operator*(const PlyJet<N, S>& a, const PlyJet<N, S>& b)
{
    PlyJet<N, S> result(a.v * b.v);
    for (std::size_t i = 0; i < N; ++i)
        result.g[i] = a.v * b.g[i] + b.v * a.g[i];
    if constexpr (S)
    {
        std::size_t at = 0;
        for (std::size_t i = 0; i < N; ++i)
            for (std::size_t j = i; j < N; ++j, ++at)
                result.h[at] = a.v * b.h[at] + b.v * a.h[at] + a.g[i] * b.g[j] + a.g[j] * b.g[i];
    }
    return result;
}

template <std::size_t N, bool S>
PlyJet<N, S> operator/(const PlyJet<N, S>& a, const PlyJet<N, S>& b)
{
    const double inverse = 1.0 / b.v;
    return a * composed(b, inverse, -inverse * inverse, 2.0 * inverse * inverse * inverse);
}

template <std::size_t N, bool S>
PlyJet<N, S> operator+(const PlyJet<N, S>& a, double b)
{
    PlyJet<N, S> result = a;
    result.v += b;
    return result;
}
template <std::size_t N, bool S>
PlyJet<N, S> operator+(double a, const PlyJet<N, S>& b)
{
    return b + a;
}
template <std::size_t N, bool S>
PlyJet<N, S> operator-(const PlyJet<N, S>& a, double b)
{
    return a + (-b);
}
template <std::size_t N, bool S>
PlyJet<N, S> operator-(double a, const PlyJet<N, S>& b)
{
    return (-b) + a;
}
template <std::size_t N, bool S>
PlyJet<N, S> operator*(const PlyJet<N, S>& a, double b)
{
    PlyJet<N, S> result(a.v * b);
    for (std::size_t i = 0; i < N; ++i)
        result.g[i] = a.g[i] * b;
    for (std::size_t k = 0; k < PlyJet<N, S>::PAIRS; ++k)
        result.h[k] = a.h[k] * b;
    return result;
}
template <std::size_t N, bool S>
PlyJet<N, S> operator*(double a, const PlyJet<N, S>& b)
{
    return b * a;
}
template <std::size_t N, bool S>
PlyJet<N, S> operator/(const PlyJet<N, S>& a, double b)
{
    return a * (1.0 / b);
}
template <std::size_t N, bool S>
PlyJet<N, S> operator/(double a, const PlyJet<N, S>& b)
{
    return PlyJet<N, S>(a) / b;
}

template <std::size_t N, bool S>
PlyJet<N, S> sin(const PlyJet<N, S>& u)
{
    const double s = std::sin(u.v);
    return composed(u, s, std::cos(u.v), -s);
}
template <std::size_t N, bool S>
PlyJet<N, S> cos(const PlyJet<N, S>& u)
{
    const double c = std::cos(u.v);
    return composed(u, c, -std::sin(u.v), -c);
}
template <std::size_t N, bool S>
PlyJet<N, S> sqrt(const PlyJet<N, S>& u)
{
    const double root = std::sqrt(u.v);
    return composed(u, root, 0.5 / root, -0.25 / (root * u.v));
}

// |u|, of the side of u >= 0 at zero.
inline double magnitude(double u) noexcept
{
    return std::fabs(u);
}
template <std::size_t N, bool S>
PlyJet<N, S> magnitude(const PlyJet<N, S>& u)
{
    return u.v >= 0.0 ? u : -u;
}
// max(u, 0), of the side of zero at zero.
inline double positivePart(double u) noexcept
{
    return u > 0.0 ? u : 0.0;
}
template <std::size_t N, bool S>
PlyJet<N, S> positivePart(const PlyJet<N, S>& u)
{
    return u.v > 0.0 ? u : PlyJet<N, S>(0.0);
}

} // namespace oor::query
