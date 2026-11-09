#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nonlinear_contact/ContactRuns.hpp"

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

namespace oor::model
{

// What the IDs of a ROTORAX card are (LTYPE): elements, properties or grid points.
enum class RotorAxisymListType : std::uint8_t
{
    Element,  // ELEM
    Property, // PROP
    Grid      // GRID
};

// The words of LTYPE, in the order of RotorAxisymListType.
inline constexpr std::array<std::string_view, 3> ROTOR_AXISYM_LIST_WORDS{"Elements", "Property", "Node"};

// One ROTORAX card: the elements, the properties or the grid points that make an axisymmetric rotor.
//
// Card layout: ROTORAX  ROTORID  LTYPE  ID1  ID2  ID3  ...             the IDs go on over the continuation lines
//        or:  ROTORAX  ROTORID  LTYPE  ID1  THRU  ID2  BY  INC
// The IDs are single IDs and ascending ranges "a THRU b [BY n]". They are kept as `ContactRun`, the list of the contact
// cards (a range stores its increment resolved: 1 when BY is left out, and is written back without it). A rotor is made
// of several ROTORAX cards with the same ROTORID, one for each kind of ID, and at least one of them lists
// the GRID points of its axis of symmetry (a rule between cards, which the check `solution.rotor-axisym-axis`
// reports). A GRID list has at least two points (a fatal error of the solver); that they are collinear
// needs the geometry and is not checked here.
struct RotorAxisymCard
{
    base::EntityId id = 0;
    RotorAxisymListType listType = RotorAxisymListType::Element;
    std::vector<ContactRun> ids;

    [[nodiscard]] bool operator==(const RotorAxisymCard&) const noexcept = default;
};

} // namespace oor::model
