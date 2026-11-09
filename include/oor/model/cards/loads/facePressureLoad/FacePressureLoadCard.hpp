#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>

namespace oor::model
{

// SORL: what the load acts on. Only the CQUADR and CTRIAR elements use LINE (edge loads); the others ignore the field.
enum class PressureKind : std::uint8_t
{
    Surface, // SURF (the default)
    Line     // LINE
};

// LDIR: the direction of a line load (SORL = LINE) when no vector (CID, N1, N2, N3) is given.
enum class LineLoadDirection : std::uint8_t
{
    Normal,  // NORM (the default)
    X,       // X in the element coordinate system
    Y,       // Y
    Z,       // Z
    Tangent, // TANG
};

// One PLOAD4 card: a pressure on a face of a shell or solid element, or on a range of shell elements.
//
// Card layout:
//   PLOAD4  SID  EID   P1  P2   P3  P4  G1      G3 or G4
//           CID  N1    N2  N3   SORL  LDIR
// or, for a range of shell elements:
//   PLOAD4  SID  EID1  P1  P2   P3  P4  "THRU"  EID2
//           CID  N1    N2  N3   SORL  LDIR
// The continuation line is optional: with CID and N1 to N3 blank the pressure acts normal to the face.
//
// P2, P3 and P4 default to P1, so they are stored resolved and always written. The exception is SORL = LINE, where a
// blank P has a meaning of its own (the edge has no load): a blank is then stored as NaN and written
// blank.
struct FacePressureLoadCard
{
    base::EntityId id = 0;
    base::EntityId elemId = 0;
    base::EntityId elemId2 = 0; // the last element of a "THRU" range, 0 for a single element
    std::array<double, 4> pressures{0.0, 0.0, 0.0, 0.0};
    base::EntityId node1 = 0;    // solid elements only; 0 = blank
    base::EntityId node3Or4 = 0; // G3 (CHEXA, CPENTA, CPYRAM) or G4 (CTETRA); 0 = blank
    base::EntityId frameId = 0;
    std::array<double, 3> direction{0.0, 0.0, 0.0}; // N1 to N3
    PressureKind kind = PressureKind::Surface;
    LineLoadDirection lineDirection = LineLoadDirection::Normal;

    [[nodiscard]] bool isRange() const noexcept { return elemId2 != 0; }
};

} // namespace oor::model
