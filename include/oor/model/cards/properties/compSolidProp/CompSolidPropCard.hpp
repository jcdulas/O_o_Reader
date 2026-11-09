#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace oor::model
{

// The stress output request of one ply (SOUT). The enumerators are numbered in the order of
// COMP_SOLID_PROP_OUTPUT_WORDS.
enum class CompSolidPropOutput : std::uint8_t
{
    No, // NO (the default)
    Yes
};

inline constexpr std::array<std::string_view, 2> COMP_SOLID_PROP_OUTPUT_WORDS{"No", "Yes"};

// The values of PSDIR, the direction of the ply stacking, that the card accepts.
inline constexpr std::array<std::int32_t, 6> COMP_SOLID_PROP_STACKING_DIRECTIONS{12, 13, 21, 23, 31, 32};

// One ply of a PCOMPS, one line of eight fields from field 9 on.
//
// FT and FTI are words (a MATDMG with the MID of the ply holds the progressive ply failure data of the plies of a
// PCOMPS, in SOLs 401 and 402; no source here lists the words, so any name is kept), blank when the ply has none.
// `tail` is the eighth field of the line: decks write ABS in it and no source here names it; it is kept as a word so
// that the card reads back as it was, blank when the deck leaves it blank.
struct CompSolidPropPly
{
    base::EntityId globalId = 0; // GPLYID, unique in the card
    base::EntityId matId = 0;
    double thickness = 0.0;
    double thetaInDegrees = 0.0;
    std::string failureTheory;      // FT, upper case
    std::string interlaminarTheory; // FTI, upper case
    CompSolidPropOutput output = CompSolidPropOutput::No;
    std::string tail; // the eighth field, upper case

    [[nodiscard]] bool operator==(const CompSolidPropPly&) const noexcept = default;
};

// One PCOMPS card (another implementation of the format): the property of a layered composite solid, which a solid
// element names.
//
// Layout (field 1 is the first field after the name; the plies are continuation lines of eight fields):
//   PCOMPS  PID      CORDM    PSDIR    SB       NB       TREF     GE       (field 8 blank)
//           GPLYID1  MID1     T1       THETA1   FT1      FTI1     SOUT1    (field 16: see CompSolidPropPly::tail)
//           GPLYID2  MID2     T2       THETA2   FT2      FTI2     SOUT2
//
// The layout is that of another implementation of the format; the card is not in the format reference list and the
// format reference says no more. CORDM is the material coordinate system: 0 for the basic system (the default), -1
// for the element system as for a PSOLID and a PCOMPLS, or the ID of a CORD2x. PSDIR is the stacking direction, one of
// 12, 13, 21, 23, 31 and 32 (13 when blank). SB and NB, which no source here describes beyond their position, have no
// default and a blank one is `std::nullopt`. TREF and GE are 0.0 when blank. A blank THETA is read as 0.0, as in every
// ply card of the format, and the writer always writes it. A card has at least one ply, with different global ply IDs.
//
// Rules between values and between cards (the thicknesses against the element, the kind of material of a ply, the
// elements that may name the property) are left to the checks of the model. This is a primary property: the card has
// the ID of the property space.
struct CompSolidPropCard
{
    base::EntityId id = 0;
    base::EntityId matFrameId = 0; // 0: basic, -1: the element system
    std::int32_t stackingDirection = 13;
    std::optional<double> bondAllowable;
    std::optional<double> normalBondAllowable;
    double referenceTemperature = 0.0;
    double structuralDamping = 0.0;
    std::vector<CompSolidPropPly> plies;

    [[nodiscard]] double totalThickness() const noexcept
    {
        double sum = 0.0;
        for (const CompSolidPropPly& ply : plies)
            sum += ply.thickness;
        return sum;
    }
};

} // namespace oor::model
