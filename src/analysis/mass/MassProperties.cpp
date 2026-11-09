#include "oor/analysis/mass/MassProperties.hpp"

#include <limits>

namespace oor::analysis
{

std::array<double, 6> inertiaProducts(const base::Mat3& tensor) noexcept
{
    return {tensor(0, 0), tensor(1, 1), tensor(2, 2), -tensor(0, 1), -tensor(0, 2), -tensor(1, 2)};
}

base::Mat3 pointMassInertia(double mass, const base::Vec3& offset) noexcept
{
    const double squared = base::dot(offset, offset);
    base::Mat3 tensor;
    for (std::size_t row = 0; row < 3; ++row)
    {
        for (std::size_t column = 0; column < 3; ++column)
            tensor(row, column) = mass * ((row == column ? squared : 0.0) - offset[row] * offset[column]);
    }
    return tensor;
}

base::Mat3 rotatedInertia(const base::Mat3& rotation, const base::Mat3& tensor) noexcept
{
    return rotation * tensor * rotation.transposed();
}

void MassSums::add(double mass, const base::Vec3& center, const base::Mat3* inertia) noexcept
{
    m_mass += mass;
    m_moment += mass * center;
    const base::Mat3 shifted = pointMassInertia(mass, center);
    for (std::size_t index = 0; index < 9; ++index)
    {
        m_inertia.values[index] += shifted.values[index];
        if (inertia != nullptr)
            m_inertia.values[index] += inertia->values[index];
    }
}

void MassSums::add(const MassSums& other) noexcept
{
    m_mass += other.m_mass;
    m_moment += other.m_moment;
    for (std::size_t index = 0; index < 9; ++index)
        m_inertia.values[index] += other.m_inertia.values[index];
}

void MassSums::scale(double factor) noexcept
{
    m_mass *= factor;
    m_moment *= factor;
    for (double& value : m_inertia.values)
        value *= factor;
}

base::Vec3 MassSums::centerOfGravity() const noexcept
{
    if (m_mass == 0.0)
    {
        constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();
        return base::Vec3{NOT_A_NUMBER, NOT_A_NUMBER, NOT_A_NUMBER};
    }
    return m_moment / m_mass;
}

base::Mat3 MassSums::inertiaAbout(const base::Vec3& point) const noexcept
{
    // The parallel axis theorem twice: from the origin to the centre of gravity (taking the point mass term out), then
    // from the centre of gravity to the point (putting it back for the new distance).
    if (m_mass == 0.0)
        return m_inertia;
    const base::Vec3 center = m_moment / m_mass;
    base::Mat3 atCenter = m_inertia;
    const base::Mat3 shift = pointMassInertia(m_mass, center);
    for (std::size_t index = 0; index < 9; ++index)
        atCenter.values[index] -= shift.values[index];
    const base::Mat3 toPoint = pointMassInertia(m_mass, center - point);
    for (std::size_t index = 0; index < 9; ++index)
        atCenter.values[index] += toPoint.values[index];
    return atCenter;
}

MassProperties MassSums::properties(const base::Vec3& reference) const noexcept
{
    MassProperties result;
    result.mass = m_mass;
    result.centerOfGravity = centerOfGravity();
    result.reference = reference;
    if (m_mass == 0.0)
    {
        result.inertiaAtCenter = m_inertia;
        result.inertiaAtReference = m_inertia;
        return result;
    }
    result.inertiaAtCenter = inertiaAbout(result.centerOfGravity);
    result.inertiaAtReference = inertiaAbout(reference);
    return result;
}

} // namespace oor::analysis
