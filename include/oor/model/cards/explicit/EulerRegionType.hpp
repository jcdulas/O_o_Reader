#pragma once

#include "oor/model/references/ReferenceVisitor.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace oor::model
{

// The kind of region of an Eulerian mesh that a region card describes (the TYPE field of an EULFREG, and of the TICREG
// that gives initial conditions in the same way): the region is a closed surface, a sphere, a cylinder, a box or a list
// of elements. The enumerators are numbered in the order of EULER_REGION_WORDS. The words are those of the format
// reference: the sphere is SPHERE, but the cylinder is CYLINDER while the card that defines it is CYLINDR.
enum class EulerRegionType : std::uint8_t
{
    Surface,  // SURF: inside or outside a closed multifaceted surface (a SURFINI)
    Sphere,   // SPHERE: inside a sphere (a SPHERE card)
    Cylinder, // CYLINDER: inside a cylinder (a CYLINDR card)
    Box,      // BOX: inside a box (a BCBOX card)
    Element   // ELEM: a list of Euler elements (a SET1)
};

inline constexpr std::array<std::string_view, 5> EULER_REGION_WORDS{"Surface", "Sphere", "Cylinder", "Box", "Elements"};

// What the VID of a region of this type names. The shapes SURFINI, SPHERE and CYLINDR are in one category; the box is a
// BCBOX, which the library keeps with the contact surfaces; a list of elements is a SET1 (a set).
[[nodiscard]] constexpr ReferenceTarget regionShapeTarget(EulerRegionType type) noexcept
{
    switch (type)
    {
    case EulerRegionType::Surface:
    case EulerRegionType::Sphere:
    case EulerRegionType::Cylinder:
        return ReferenceTarget::EulerRegionShape;
    case EulerRegionType::Box:
        return ReferenceTarget::ContactSurface;
    case EulerRegionType::Element:
        return ReferenceTarget::Set;
    }
    return ReferenceTarget::Other;
}

} // namespace oor::model
