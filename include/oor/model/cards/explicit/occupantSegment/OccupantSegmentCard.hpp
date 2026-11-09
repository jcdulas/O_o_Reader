#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace oor::model
{

// The COVER field of an ATBSEG: whether SOL 700 also generates the cover (the shell elements of PIDCOV) of the contact
// ellipsoid of the segment. The format gives no list of words; YES and NO are what the name of
// the field suggests, and the DYPARAM flags of the occupant output use the same two words.
enum class OccupantSegmentCover : std::uint8_t
{
    Yes,
    No
};
inline constexpr std::array<std::string_view, 2> OCCUPANT_SEGMENT_COVER_WORDS{"Yes", "No"};

// One ATBSEG card (SOL 700): the position and orientation of one segment of the dummy of an external occupant model,
// which replace those of the G.2 and G.3 entries of its input file, and the numbers SOL 700 uses to generate a Bulk
// Data file that draws the segment (grid points, the contact ellipsoid with its cover, the joints it hangs on).
//
// Card layout: ATBSEG  ID  NAME  COVER  NUMELM  GSTART  ESTART  MID  PIDCOV      fields 1 to 8
//             (line 2)  G0  G1  G2  G3  EID1  EID2  EID3  PIDCG                 fields 9 to 16
// The format describes ID, NAME and G0 to G3 only. NAME is the name of the segment in the first field of a B.2 entry of
// the occupant input file. G0 is the origin of the segment system, G1, G2 and G3 are on its x, y and z axes: they are
// grid points of the model and the format marks them as required. For the other fields the card is typed from their
// names and a common deck: NUMELM is a count of elements, GSTART and ESTART are the first numbers of the grid points
// and elements to generate, MID is the material, PIDCOV the property of the cover and PIDCG the property of the center
// of gravity element that all the generated elements get, EID1 to EID3 are numbers of generated elements. None of these
// names a card the deck must hold (SOL 700 creates them), so none is a reference, and any may be blank.
struct OccupantSegmentCard
{
    base::EntityId id = 0;                      // unique among the ATBSEG cards
    std::string name;                           // the segment in the occupant input file, upper case
    std::optional<OccupantSegmentCover> cover;  // blank: not given
    base::EntityId elemCount = 0;               // 0 when blank
    base::EntityId firstNodeId = 0;             // 0 when blank
    base::EntityId firstElemId = 0;             // 0 when blank
    base::EntityId matId = 0;                   // 0 when blank
    base::EntityId coverPropId = 0;             // 0 when blank
    std::array<base::EntityId, 4> grids = {};   // G0 to G3: grid points of the model
    std::array<base::EntityId, 3> elemIds = {}; // EID1 to EID3, 0 when blank
    base::EntityId centerOfGravityPropId = 0;   // 0 when blank
};

} // namespace oor::model
