#pragma once

#include "oor/model/cards/solution_control/InitialStateRange.hpp"

#include <array>
#include <cstdint>
#include <optional>

namespace oor::model
{

// How many stress components an ISTRESS has.
inline constexpr std::size_t INITIAL_STRESS_COMPONENTS = 7;

// One ISTRESS card (SOL 400): an initial stress, for the elements, integration points and layers of a range. It acts in
// SOL 400 only, and the stresses must be self-equilibrating and must not produce material nonlinearity.
// The format gives the meaning of the components for each kind of element (normal and shear stresses, in the
// basic system x, y, z or in the element system 1, 2, 3), which depends on the element and is not checked.
//
// Card layout: ISTRESS  EID1  EID2  INT1  INTN  LAY1  LAYN  STRESS1  STRESS2             fields 1 to 8
//                      STRESS3  STRESS4  STRESS5  STRESS6  STRESS7                       fields 9 to 13
//                      "COORD"  CID                                                      fields 17 and 18
// The six first fields are the range of `InitialStateRange`. Up to seven components are given and a blank one is 0.0.
// The third line has the word COORD in its first field and CID after it, and says in which system the components are
// evaluated: CID = -1 the element system, CID = 0 the basic system (the format allows only -1 and 0 and gives
// no other number, so a coordinate system card is not named: CID is not a reference, whatever the format's name for the
// field suggests). Without the line the system is the element's for a CQUAD4 and a CTRIA3 (the default of the format,
// which names these two elements only), and the format says nothing for the others. The card keeps the CID when the
// COORD line is given with one (`frameId`), and nothing when the line is not there or its CID is blank.
struct InitialStressCard
{
    InitialStateRange range;
    std::array<double, INITIAL_STRESS_COMPONENTS> stress{}; // STRESS1 to STRESS7, 0.0 when blank
    std::optional<std::int32_t> frameId;                    // -1 or 0 when the COORD line is given

    [[nodiscard]] bool operator==(const InitialStressCard&) const noexcept = default;
};

} // namespace oor::model
