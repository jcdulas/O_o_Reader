#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/properties/DistributedPropertyType.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace oor::model
{

// One PDISTB card: a spatial distribution of one datum of a property. For each element it lists (EIDi) the value (Vi)
// that replaces the datum PNAME of the property PID, so one property can give each of its elements another thickness,
// density or section constant. The value an element gets is SMULT * Vi + DELTA.
//
// Card layout: PDISTB  ID  TYPE  PID  PNAME  LOCNUM  SMULT  DELTA            fields 1 to 7
//                     EID1  V1  EID2  V2  EID3  V3  EID4  V4                  fields 9 to 16
//                     EID5  V5  EID6  V6  ...                                 fields 17 on
// Four pairs make a line and a card goes on for as many lines as it has pairs; field 8 is a blank cell of the layout.
//
// ID is the number of the entry, unique among the PDISTB cards only: it is for the convenience of the user and no Case
// Control command selects it. TYPE is the card name of the property (PBAR, PBEAM, PBEAM3, PCOMP, PCOMPG,
// PROD, PSHEAR, PSHELL, PSOLID or PTUBE) and PID names a property of that kind; PNAME is the name the format
// gives the datum ("T", "I12", ...). The format lists the names each kind of property supports:
// the card refuses any other. LOCNUM picks the ply of a PCOMP or PCOMPG or the station of a PBEAM or PBEAM3 for the
// data that vary over them; it has no default, so a blank one stays blank (a common deck gives the damping GE of a
// PCOMPG with none). SMULT is 1.0 and DELTA 0.0 when blank.
//
// A PSOLID has the one name CORDM, the material coordinate system of the element: its values are integers, at least -2
//, and SMULT and DELTA stay blank (the format says to leave them). Every other kind has real
// values. A value is kept as a real in both cases and written as an integer for a PSOLID.
//
// A card with no pair gives the elements nothing, so the property keeps its own value; the card is kept. An
// element is listed at most once in a card; that the same element is not given a value by another PDISTB of
// the same PID and PNAME is a rule between cards that the card cannot see.
struct PropDistributionCard
{
    base::EntityId id = 0; // unique among the PDISTB cards
    DistributedPropertyType type = DistributedPropertyType::ShellProp;
    base::EntityId propId = 0;
    std::string fieldName;                      // upper case: one of the names the format reference lists for the TYPE
    std::optional<std::int32_t> locationNumber; // the ply or station, not negative
    double scaleFactor = 1.0;
    double valueOffset = 0.0;
    std::vector<std::pair<base::EntityId, double>> entries; // (EID, V)

    [[nodiscard]] bool operator==(const PropDistributionCard&) const noexcept = default;
};

} // namespace oor::model
