#pragma once

#include "oor/base/math/Mat3.hpp"
#include "oor/base/math/Vec3.hpp"

#include <array>

namespace oor::analysis
{

// The mass, centre of gravity and inertia of a body or of a set of elements, in the basic system.
//
//   mass                    the total mass
//   centerOfGravity         the centre of gravity (NaN when the mass is zero)
//   reference               the point `inertiaAtReference` is taken about
//   inertiaAtCenter         the inertia tensor about the centre of gravity
//   inertiaAtReference      the inertia tensor about the reference point
//
// The inertia is the tensor: Ixx = integral of (y^2 + z^2) dm on the diagonal and the negative products of inertia off
// the diagonal (-Ixy = -integral of x y dm). `products` gives the six numbers (Ixx, Iyy, Izz, Ixy, Ixz, Iyz) with the
// products positive, the order and the signs of the inertia fields of CONM2.
struct MassProperties
{
    double mass = 0.0;
    base::Vec3 centerOfGravity;
    base::Vec3 reference;
    base::Mat3 inertiaAtCenter{{0, 0, 0, 0, 0, 0, 0, 0, 0}};
    base::Mat3 inertiaAtReference{{0, 0, 0, 0, 0, 0, 0, 0, 0}};
};

// The six numbers (Ixx, Iyy, Izz, Ixy, Ixz, Iyz) of an inertia tensor, the products with the sign of the integral.
[[nodiscard]] std::array<double, 6> inertiaProducts(const base::Mat3& tensor) noexcept;

// The inertia tensor of a point mass m at `offset` from the point it is taken about: m (|d|^2 I - d d^T).
[[nodiscard]] base::Mat3 pointMassInertia(double mass, const base::Vec3& offset) noexcept;

// The inertia tensor `tensor` given in the axes of `rotation` (whose columns are the axes in the basic system), in the
// basic system: R T R^T.
[[nodiscard]] base::Mat3 rotatedInertia(const base::Mat3& rotation, const base::Mat3& tensor) noexcept;

// A running total of bodies: each has a mass, a centre and an inertia tensor about its own centre (a point mass has
// none). The sums are the mass, the first moment and the inertia about the origin, which add; a result about any
// point follows by the parallel axis theorem.
class MassSums
{
public:
    // A body of mass `mass` at `center`; `inertia`, when given, is its own inertia tensor about `center`.
    void add(double mass, const base::Vec3& center, const base::Mat3* inertia = nullptr) noexcept;
    void add(const MassSums& other) noexcept;
    void scale(double factor) noexcept;

    [[nodiscard]] double mass() const noexcept { return m_mass; }
    [[nodiscard]] bool empty() const noexcept
    {
        return m_mass == 0.0 && m_inertia == base::Mat3{{0, 0, 0, 0, 0, 0, 0, 0, 0}};
    }
    // The centre of gravity; NaN when the mass is zero.
    [[nodiscard]] base::Vec3 centerOfGravity() const noexcept;
    // The inertia tensor about a point.
    [[nodiscard]] base::Mat3 inertiaAbout(const base::Vec3& point) const noexcept;
    [[nodiscard]] MassProperties properties(const base::Vec3& reference) const noexcept;

private:
    double m_mass = 0.0;
    base::Vec3 m_moment;                               // the sum of mass times centre
    base::Mat3 m_inertia{{0, 0, 0, 0, 0, 0, 0, 0, 0}}; // the inertia tensor about the origin
};

} // namespace oor::analysis
