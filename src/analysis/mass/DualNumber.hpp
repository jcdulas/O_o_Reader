#pragma once

#include <array>
#include <cmath>
#include <cstddef>

// Forward-mode derivatives for the kernels of the sections and laminates: a number carries its
// derivatives along N directions. Over a scalar S = double it gives first derivatives; over S = Dual<M> (forward over
// forward) the derivatives of the value and of each first derivative along M other directions, that is the second
// derivatives d2 f / dx_i dx_j of the inner direction i and the outer direction j. The kernels written for a scalar T
// (double, or one of these) compute their values with T = double exactly as they did with doubles: the operators of
// Dual are only used by the instantiations that differentiate.

namespace oor::analysis::detail
{

template <std::size_t N, typename S = double>
struct Dual
{
    S v{};
    std::array<S, N> d{};

    Dual() = default;
    // A constant (no derivative).
    Dual(double value) : v(value) {} // NOLINT(google-explicit-constructor): a constant mixes with numbers
};

// The value of a number, its derivatives dropped.
inline double valueOf(double value) noexcept
{
    return value;
}

template <std::size_t N, typename S>
double valueOf(const Dual<N, S>& value) noexcept
{
    return valueOf(value.v);
}

// True when the number and all its derivatives are zero.
inline bool isZero(double value) noexcept
{
    return value == 0.0;
}

template <std::size_t N, typename S>
bool isZero(const Dual<N, S>& value) noexcept
{
    if (!isZero(value.v))
        return false;
    for (const S& entry : value.d)
        if (!isZero(entry))
            return false;
    return true;
}

template <std::size_t N, typename S>
Dual<N, S>& operator+=(Dual<N, S>& left, const Dual<N, S>& right)
{
    left.v += right.v;
    for (std::size_t i = 0; i < N; ++i)
        left.d[i] += right.d[i];
    return left;
}

template <std::size_t N, typename S>
Dual<N, S>& operator-=(Dual<N, S>& left, const Dual<N, S>& right)
{
    left.v -= right.v;
    for (std::size_t i = 0; i < N; ++i)
        left.d[i] -= right.d[i];
    return left;
}

template <std::size_t N, typename S>
Dual<N, S>& operator+=(Dual<N, S>& left, double right)
{
    left.v += right;
    return left;
}

template <std::size_t N, typename S>
Dual<N, S>& operator-=(Dual<N, S>& left, double right)
{
    left.v -= right;
    return left;
}

template <std::size_t N, typename S>
Dual<N, S>& operator*=(Dual<N, S>& value, double factor)
{
    value.v *= factor;
    for (S& entry : value.d)
        entry *= factor;
    return value;
}

template <std::size_t N, typename S>
Dual<N, S>& operator/=(Dual<N, S>& value, double divisor)
{
    value.v /= divisor;
    for (S& entry : value.d)
        entry /= divisor;
    return value;
}

template <std::size_t N, typename S>
Dual<N, S> operator+(Dual<N, S> left, const Dual<N, S>& right)
{
    left += right;
    return left;
}

template <std::size_t N, typename S>
Dual<N, S> operator-(Dual<N, S> left, const Dual<N, S>& right)
{
    left -= right;
    return left;
}

template <std::size_t N, typename S>
Dual<N, S> operator-(Dual<N, S> value)
{
    value *= -1.0;
    return value;
}

template <std::size_t N, typename S>
Dual<N, S> operator*(const Dual<N, S>& left, const Dual<N, S>& right)
{
    Dual<N, S> result;
    result.v = left.v * right.v;
    for (std::size_t i = 0; i < N; ++i)
        result.d[i] = left.d[i] * right.v + left.v * right.d[i];
    return result;
}

template <std::size_t N, typename S>
Dual<N, S> operator*(double factor, Dual<N, S> value)
{
    value *= factor;
    return value;
}

template <std::size_t N, typename S>
Dual<N, S> operator*(Dual<N, S> value, double factor)
{
    value *= factor;
    return value;
}

template <std::size_t N, typename S>
Dual<N, S> operator/(const Dual<N, S>& left, const Dual<N, S>& right)
{
    Dual<N, S> result;
    result.v = left.v / right.v;
    const S square = right.v * right.v;
    for (std::size_t i = 0; i < N; ++i)
        result.d[i] = (left.d[i] * right.v - left.v * right.d[i]) / square;
    return result;
}

template <std::size_t N, typename S>
Dual<N, S> operator/(Dual<N, S> value, double divisor)
{
    value /= divisor;
    return value;
}

template <std::size_t N, typename S>
Dual<N, S> operator/(double left, const Dual<N, S>& right)
{
    return Dual<N, S>(left) / right;
}

template <std::size_t N, typename S>
Dual<N, S> operator+(Dual<N, S> left, double right)
{
    left.v += right;
    return left;
}

template <std::size_t N, typename S>
Dual<N, S> operator+(double left, Dual<N, S> right)
{
    right.v += left;
    return right;
}

template <std::size_t N, typename S>
Dual<N, S> operator-(Dual<N, S> left, double right)
{
    left.v -= right;
    return left;
}

template <std::size_t N, typename S>
Dual<N, S> operator-(double left, const Dual<N, S>& right)
{
    return Dual<N, S>(left) - right;
}

// value += factor * term, in place (factor a double or a scalar of the number).
template <std::size_t N, typename S, typename F>
void addScaled(Dual<N, S>& value, const F& factor, const Dual<N, S>& term)
{
    value.v += factor * term.v;
    for (std::size_t i = 0; i < N; ++i)
        value.d[i] += factor * term.d[i];
}

// The comparisons compare the values.
template <std::size_t N, typename S>
bool operator<(const Dual<N, S>& left, const Dual<N, S>& right) noexcept
{
    return valueOf(left) < valueOf(right);
}

template <std::size_t N, typename S>
bool operator>(const Dual<N, S>& left, const Dual<N, S>& right) noexcept
{
    return valueOf(left) > valueOf(right);
}

template <std::size_t N, typename S>
bool operator<=(const Dual<N, S>& left, const Dual<N, S>& right) noexcept
{
    return valueOf(left) <= valueOf(right);
}

template <std::size_t N, typename S>
bool operator>=(const Dual<N, S>& left, const Dual<N, S>& right) noexcept
{
    return valueOf(left) >= valueOf(right);
}

template <std::size_t N, typename S>
bool operator<(const Dual<N, S>& left, double right) noexcept
{
    return valueOf(left) < right;
}

template <std::size_t N, typename S>
bool operator>(const Dual<N, S>& left, double right) noexcept
{
    return valueOf(left) > right;
}

template <std::size_t N, typename S>
bool operator<=(const Dual<N, S>& left, double right) noexcept
{
    return valueOf(left) <= right;
}

template <std::size_t N, typename S>
bool operator>=(const Dual<N, S>& left, double right) noexcept
{
    return valueOf(left) >= right;
}

template <std::size_t N, typename S>
bool operator<(double left, const Dual<N, S>& right) noexcept
{
    return left < valueOf(right);
}

template <std::size_t N, typename S>
bool operator>(double left, const Dual<N, S>& right) noexcept
{
    return left > valueOf(right);
}

template <std::size_t N, typename S>
bool operator<=(double left, const Dual<N, S>& right) noexcept
{
    return left <= valueOf(right);
}

template <std::size_t N, typename S>
bool operator>=(double left, const Dual<N, S>& right) noexcept
{
    return left >= valueOf(right);
}

// The functions of a number: its value through the function of its scalar, its derivatives by the chain rule (the
// derivative of the function evaluated on the scalar, so that a nested number gets its second derivatives).
template <std::size_t N, typename S>
Dual<N, S> sqrt(const Dual<N, S>& value)
{
    using std::sqrt;
    Dual<N, S> result;
    result.v = sqrt(value.v);
    const S twice = 2.0 * result.v;
    for (std::size_t i = 0; i < N; ++i)
        result.d[i] = value.d[i] / twice;
    return result;
}

template <std::size_t N, typename S>
Dual<N, S> pow(const Dual<N, S>& value, double exponent)
{
    using std::pow;
    Dual<N, S> result;
    result.v = pow(value.v, exponent);
    const S slope = exponent * pow(value.v, exponent - 1.0);
    for (std::size_t i = 0; i < N; ++i)
        result.d[i] = slope * value.d[i];
    return result;
}

template <std::size_t N, typename S>
Dual<N, S> hypot(const Dual<N, S>& first, const Dual<N, S>& second)
{
    return sqrt(first * first + second * second);
}

template <std::size_t N, typename S>
Dual<N, S> atan2(const Dual<N, S>& y, const Dual<N, S>& x)
{
    using std::atan2;
    Dual<N, S> result;
    result.v = atan2(y.v, x.v);
    const S square = x.v * x.v + y.v * y.v;
    for (std::size_t i = 0; i < N; ++i)
        result.d[i] = (x.v * y.d[i] - y.v * x.d[i]) / square;
    return result;
}

// The second-order number of forward over forward: Hessian<N>::d[i].d[j] is d2 f / dx_i dx_j for the inner
// direction i and the outer direction j.
template <std::size_t N>
using Hessian = Dual<N, Dual<N>>;

} // namespace oor::analysis::detail
