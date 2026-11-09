#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace oor::model
{

// A set of elements of an ELSET or SPOTW line and the fatigue properties they have (ELSIDi and PFTGIDi).
struct FatigueDefinitionPair
{
    base::EntityId elemSetId = 0;     // a SET1, SET3 or SET4 of elements
    base::EntityId fatiguePropId = 0; // a PFTG, 0 when blank (default properties)

    [[nodiscard]] bool operator==(const FatigueDefinitionPair&) const noexcept = default;
};

// The kind of a seam weld (WELDi): a label only, it changes nothing of the calculation.
enum class FatigueDefinitionWeld : std::uint8_t
{
    Fillet,
    Overlap,
    Laser,
    Edge,
    Generic
};
inline constexpr std::array<std::string_view, 5> FATIGUE_DEFINITION_WELD_WORDS{"FilletWeld", "OverlapWeld", "LaserWeld",
                                                                               "Edge", "Generic"};

// Where on the weld the elements of a SEAMW group are (TYPEi): the toe, the root or the throat.
enum class FatigueDefinitionWeldLocation : std::uint8_t
{
    Toe,
    Root,
    Throat
};
inline constexpr std::array<std::string_view, 3> FATIGUE_DEFINITION_LOCATION_WORDS{"ToeWeld", "RootWeld",
                                                                                   "ThroatThickness"};

// The default TYPEi of a seam weld when the field is blank: the root of a laser weld, the toe of the others.
[[nodiscard]] constexpr FatigueDefinitionWeldLocation defaultWeldLocation(FatigueDefinitionWeld weld) noexcept
{
    return weld == FatigueDefinitionWeld::Laser ? FatigueDefinitionWeldLocation::Root
                                                : FatigueDefinitionWeldLocation::Toe;
}

// One group of a SEAMW line: the elements of one part of a seam weld.
struct FatigueDefinitionSeam
{
    base::EntityId elemSetId = 0;     // the elements of the toe, the root or the throat
    base::EntityId fatiguePropId = 0; // a PFTG, 0 when blank
    base::EntityId nodeSetId = 0;     // the grids that make the seam line, 0 when blank (all the grids of the elements)
    FatigueDefinitionWeld weld = FatigueDefinitionWeld::Generic;
    FatigueDefinitionWeldLocation location =
        FatigueDefinitionWeldLocation::Toe; // resolved: its default depends on the weld

    [[nodiscard]] bool operator==(const FatigueDefinitionSeam&) const noexcept = default;
};

// One FTGDEF card: the elements of a fatigue analysis and the fatigue properties each has, and how much of the result
// is kept (the top percentage of the entities, the number of hot spots).
//
// Card layout, SOL 101, 103, 112:
//   FTGDEF  ID  TOPSTR  PFTGID  TOPDMG  NENTS  maxENTS  NHS  HSGATE                       fields 1 to 8
//           ELSET   ELSID1 PFTGID1  ELSID2 PFTGID2  ELSID3 PFTGID3                        a line of three pairs, then
//           more lines SPOTW   the same pairs                                                         the elements of
//           the spot welds SEAMW   ELSID1 PFTGID1 NDSID1 WELD1 TYPE1                                      a group on
//           each line XELSET  XELSID1 ... XELSID7                                                    seven sets on each
//           line
// Card layout, SOL 108, 111:
//   FTGDEF  ID  TOPRMS                                                                     fields 1 and 2
//           ELSET   ELSID1 (blank) ELSID2 (blank) ELSID3                                   the same lines with no
//           property XELSET  XELSID1 ... XELSID7
// The sections start with their keyword in field 2 of a line, their data from field 3, and a section goes on over the
// lines below it that have a blank field 2. The format of SOL 108 and 111 is the other one with fewer fields in the
// same places: TOPRMS is TOPSTR (the same field, in another range: a percentage up to 100 against one between -100 and
// 100) and the ELSET line with the property fields blank is the pair line with a blank PFTGIDi, so one card reads both,
// and the card cannot (and need not) know which solution it is for. TOPSTR (or TOPRMS) holds -100 to 100.
//
// Defaults. A blank TOPSTR, PFTGID, TOPDMG, NENTS and NHS stays blank (the format reference gives no value: all the
// entities are used). maxENTS is 100 and HSGATE 0, static defaults. A blank WELDi is GENERIC and a blank TYPEi is TOE,
// or ROOT when WELDi is LASER: it depends on another field, so TYPEi is stored resolved and always written. A section
// appears once and has at least one entry; the card writes the sections in the order of the format (ELSET, SPOTW,
// SEAMW, XELSET).
//
// References. PFTGID and PFTGIDi are PFTG cards (FatigueProperty), ELSIDi, NDSIDi and XELSIDi are sets (SET1, SET3 or
// SET4: the Set target). The ID is unique among the FTGDEF cards (IdSpace::PerCardType, category Other: a FATIGUE
// command of the Case Control selects it).
struct FatigueDefinitionCard
{
    base::EntityId id = 0;
    std::optional<double> topStressCount; // TOPSTR, or TOPRMS (SOL 108, 111): a percentage
    base::EntityId fatiguePropIds =
        0; // the properties of every element when no ELSET line says otherwise, 0 when blank
    std::optional<double> topDamageCount; // the top percentage of the damage
    std::optional<int> maxEntities;  // the number of entities to report: positive by damage, negative by stress range
    int maxOutputEntities = 100;     // the most entities the output of STROUT=4 may have
    std::optional<int> hotSpotCount; // the number of hot spots
    int hotSpotGate = 0;             // the layers of elements kept around a hot spot
    std::vector<FatigueDefinitionPair> elemSets;
    std::vector<FatigueDefinitionPair> spotWelds;
    std::vector<FatigueDefinitionSeam> seamWelds;
    std::vector<base::EntityId> excludedElemSetIds; // the sets of elements that are left out
};

} // namespace oor::model
