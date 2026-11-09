#pragma once

#include <cstdint>
#include <string_view>

namespace oor::analysis
{

// What an element card does in the structure, for the definition of a component: the elements
// that make a component and the ones that connect components.
enum class ElementRole : std::uint8_t
{
    None,      // not an element of the structure: a plot element, an element the library cannot place
    Shell,     // two-dimensional elements: shells, plane and axisymmetric elements, shear panels
    Solid,     // three-dimensional elements
    Line,      // structural 1D elements: rods, tubes, bars, beams, bends
    Mass,      // concentrated masses (CONM1, CONM2, CMASS1 to CMASS4)
    Connector, // springs, dampers, bushes, gaps, fasteners, welds, general elements: connections
    Rigid      // rigid elements (RBE1, RBE2, RBE3, RBAR, ...): connections
};

// The role of the elements of a card type (by the type index of the registry). `name` is the card name.
[[nodiscard]] ElementRole roleOfCard(std::string_view name) noexcept;
[[nodiscard]] ElementRole roleOfType(std::uint16_t typeIndex) noexcept;

// True for the roles that act as a connection: they belong to no component.
[[nodiscard]] constexpr bool isConnection(ElementRole role) noexcept
{
    return role == ElementRole::Connector || role == ElementRole::Rigid;
}

} // namespace oor::analysis
