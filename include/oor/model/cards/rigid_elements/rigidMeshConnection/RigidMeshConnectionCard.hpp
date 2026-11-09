#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace oor::model
{

enum class RigidMeshConnectionSecondary : std::uint8_t
{
    Surface, // SURF (the default): the faces of a BSURF, which SID names
    Grid     // GRID: grid points listed by a SET1 or a BCGRID, which SID names
};

inline constexpr std::array<std::string_view, 2> RIGID_MESH_CONNECTION_SECONDARY_WORDS{"Surface", "Node"};

// MTYPE of an RCONN: what the primary side is made of. The format lists one kind only, the faces of a BSURF; the
// enumeration has the one enumerator so that a second kind is one more word.
enum class RigidMeshConnectionPrimary : std::uint8_t
{
    Surface // SURF (the default): the faces of the BSURF that MID names
};

inline constexpr std::array<std::string_view, 1> RIGID_MESH_CONNECTION_PRIMARY_WORDS{"Surface"};

// OPTION of an RCONN: how grid points are tied when the secondary side is a list of grid points (STYPE = GRID).
enum class RigidMeshConnectionOption : std::uint8_t
{
    Normal, // NORMAL (the default): the points are tied to the primary surface
    Shell   // SHELL: the points are on the edge of shell or beam elements, which are tied to a shell surface
};

inline constexpr std::array<std::string_view, 2> RIGID_MESH_CONNECTION_OPTION_WORDS{"Normal", "Shell"};

// CLSGAP of an RCONN: whether the gaps between the two sides are closed.
enum class RigidMeshConnectionGapClosing : std::uint8_t
{
    No, // NO (the default)
    Yes // YES: the secondary side is moved onto the primary one
};

inline constexpr std::array<std::string_view, 2> RIGID_MESH_CONNECTION_GAP_CLOSING_WORDS{"No", "Yes"};

// GAPDIS of an RCONN: what GAPDISV means.
enum class RigidMeshConnectionGapMeasure : std::uint8_t
{
    Distance // DISTANCE (the default): GAPDISV is a length
};

inline constexpr std::array<std::string_view, 1> RIGID_MESH_CONNECTION_GAP_MEASURE_WORDS{"Distance"};

// One RCONN card (SOL 700): a rigid connection that ties two parts of Lagrangian meshes together (two surfaces, grid
// points to a surface, or the edge of shells to a shell surface).
//
// Card layout: RCONN  CID    STYPE   MTYPE    SID  MID  OPTION
//                    (a blank line)
//                    CLSGAP GAPDIS  GAPDISV
// The format draws a blank line between the two that have fields, so CLSGAP, GAPDIS and GAPDISV are fields 17 to 19 of
// the card, and fields 7 to 16 are blank: a deck that puts CLSGAP in field 9 is refused. A card whose last three fields
// are all the default is written on its first line alone.
//
// CID is the number of the connection, unique among the RCONN cards (a category of its own, no other card names it).
// The defaults are all static, so the card stores the words and the number as the format gives them: SURF, SURF,
// NORMAL, NO, DISTANCE and 1.0E20. SID is the ID of a BSURF when STYPE is SURF, and of a SET1 or a BCGRID when it is
// GRID (the card cannot tell which of the last two it is, so that ID is not a reference); MID is always the ID of a
// BSURF. OPTION only counts for STYPE = GRID and the card keeps it whatever STYPE is.
struct RigidMeshConnectionCard
{
    static constexpr double DEFAULT_GAP_VALUE = 1.0E20;

    base::EntityId id = 0;
    RigidMeshConnectionSecondary secondaryType = RigidMeshConnectionSecondary::Surface;
    RigidMeshConnectionPrimary primaryType = RigidMeshConnectionPrimary::Surface;
    base::EntityId secondaryId = 0;
    base::EntityId primaryId = 0;
    RigidMeshConnectionOption option = RigidMeshConnectionOption::Normal;
    RigidMeshConnectionGapClosing closeGaps = RigidMeshConnectionGapClosing::No;
    RigidMeshConnectionGapMeasure gapMeasure = RigidMeshConnectionGapMeasure::Distance;
    double gapValue = DEFAULT_GAP_VALUE;
};

} // namespace oor::model
