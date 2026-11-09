#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/dynamics/FrfComponentReference.hpp"

#include <array>
#include <cstdint>

namespace oor::model
{

// A property of a flexible connection: a number, or the ID of a TABLED that gives it as a function of frequency (the
// KVALUE/KTABID, BVALUE/BTABID and GEVALUE/GETABID fields of an FRFFLEX). The two live in separate members and at most
// one is set. The value must be positive, so 0.0 stands for "not given" and a field that is blank has neither: the
// reader refuses an explicit 0.0 or 0 rather than reading it as blank.
struct FrfConnectionValue
{
    double value = 0.0;
    base::EntityId table = 0;

    [[nodiscard]] bool isBlank() const noexcept { return value == 0.0 && table == 0; }
    [[nodiscard]] bool operator==(const FrfConnectionValue&) const noexcept = default;
};

// One FRFFLEX card: the stiffness, damping and structural damping of a flexible connection between a point of one FRF
// component and a point of another, in an FRF based assembly (FBA).
//
// Card layout: FRFFLEX  FLEXID  C  COMPID1/COMPNAM1  POINT1  COMPID2/COMPNAM2  POINT2  KVALUE/KTABID  BVALUE/BTABID
//                      GEVALUE/GETABID
// FLEXID is the unique number of the card; C is one component (1 to 6 for a grid point, 0 or blank for a scalar point).
// Each component is the ID of an FRFCOMP or its name (a whole number is an ID, anything else a name) and each POINTi a
// plain number of that component's own model. KVALUE/KTABID is the elastic property (force per unit displacement),
// BVALUE/BTABID the damping (force per unit velocity) and GEVALUE/GETABID the damping coefficient (twice the critical
// damping ratio): an integer in the field is the ID of a TABLED, a real is the value (each pair is read on its own,
// so one card may mix the two forms). The rules of the format: K and B are not both blank, and GE is given only with
// K; the continuation line is needed only when GE is there. Each value is a positive real.
struct FrfFlexibleConnectionCard
{
    base::EntityId id = 0;
    std::int32_t components = 0;
    std::array<FrfComponentPoint, 2> points;
    FrfConnectionValue stiffness;
    FrfConnectionValue damping;
    FrfConnectionValue structuralDamping;

    [[nodiscard]] bool operator==(const FrfFlexibleConnectionCard&) const noexcept = default;
};

} // namespace oor::model
