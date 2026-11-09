#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/optimisation/DesignTerms.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

// One DVLREL1 card: a value of a load as a linear function of design variables (SOL 200 with ANALYSIS = STATICS).
//
// Card layout: DVLREL1  ID  TYPE  SID  LNAME  LMIN  LMAX  C0                           fields 1 to 7
//                      ATT1  ATT2  ATT3  ATT4  ATT5                                   fields 9 to 13 (the second line)
//                      DVID1  COEF1  DVID2  COEF2  DVID3  COEF3  ...                  from field 17 (the third line)
//   load value = C0 + sum of COEFi * (design variable DVIDi)
// TYPE is the name of the load card (FORCE, LOAD or MOMENT: the format lists these three), SID the set of loads that
// holds it and LNAME the name of the value on that card: F, N1, N2 or N3 of a FORCE, M, N1, N2 or N3 of a MOMENT, S or
// Si of a LOAD (the scale factor, or the factor of the i-th load set). The ATTi tell which load of the set is meant:
// of a FORCE or a MOMENT, ATT1 is the grid point and ATT2 the coordinate system; of a LOAD, ATT1 is 0 when
// LNAME is S and the load set Li otherwise. They are optional (left out, the load that qualifies with the
// others is the one), so a blank is kept blank (`std::nullopt`) apart from a 0, which a LOAD with LNAME S asks for.
// LMIN, the lowest value the load may take, is -1.0E35 and LMAX 1.0E20 when blank, and C0 0.0: defaults that do not
// depend on another field, stored as they are and written as blanks (the format reference applies the limits only when
// the load is not a function of one variable and C0 = 0, a matter of the design model). The first coefficient
// may be the word PVAL, which takes it from the value the load has in the deck (only one coefficient may be
// PVAL, the first). LNAME is checked against the format's table for the three types, and kept as written for another
// TYPE. The ID is unique among the DVLREL1 cards.
struct DesignLoadRelation1Card
{
    static constexpr double NO_LOWER_LIMIT = -1.0e35;
    static constexpr double NO_UPPER_LIMIT = 1.0e20;
    static constexpr std::size_t ATTRIBUTE_COUNT = 5;

    base::EntityId id = 0;
    std::string kind; // the neutral name of the load card type ("PointForceLoad"), or the word in upper case
    base::EntityId setId = 0;
    std::string fieldName; // upper case
    double minValue = NO_LOWER_LIMIT;
    double maxValue = NO_UPPER_LIMIT;
    double constant0 = 0.0;
    std::array<std::optional<std::int32_t>, ATTRIBUTE_COUNT> attributes;
    std::vector<DesignTerm> terms; // at least one; the first coefficient is PVAL when `fromProperty`
    bool fromProperty = false;

    [[nodiscard]] bool operator==(const DesignLoadRelation1Card&) const noexcept = default;
};

// What an attribute of a DVLREL1 names, by the table of the format.
enum class LoadAttribute : std::uint8_t
{
    Unused,            // the attribute means nothing for this load (or the TYPE is none of the three)
    Grid,              // a grid point: ATT1 of a FORCE and a MOMENT
    Coordinate,        // a coordinate system: ATT2 of a FORCE and a MOMENT
    LoadCombinationSet // a set of loads: ATT1 of a LOAD whose LNAME is Si (not S: then it is 0)
};

// What the attribute number `index` (0 is ATT1) of the card names.
[[nodiscard]] LoadAttribute loadAttributeOf(const DesignLoadRelation1Card& card, std::size_t index);

} // namespace oor::model
