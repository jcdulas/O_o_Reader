#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nodes/ModuleIds.hpp"

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

namespace oor::model
{

// How the boundary points that an MDCONCT finds are connected (the TYPE field). The enumerators are numbered in the
// order of MODULE_CONNECTION_TYPE_WORDS.
enum class ModuleConnectionType : std::uint8_t
{
    Merge, // MERGE: the points are moved to the location and tied by an RBE2
    Rigid, // RIGID: the points stay where they are and are tied by RBE2 elements
    Rrbe2, // RRBE2: they stay, and only the independent points of the RBE2 elements of the model are used
    Mrbe2  // MRBE2 (the default): they are moved, and only the independent points of the RBE2 elements are used
};

inline constexpr std::array<std::string_view, 4> MODULE_CONNECTION_TYPE_WORDS{"Merge", "Rigid", "RigidStarReleased",
                                                                              "ModuleRigidStar"};

// One MDCONCT card: how the boundary points of the modules are connected at a location (SOLs 101, 103 to 112).
//
// Card layout: MDCONCT  BID  TYPE  TOL  X    Y     Z   .   CID               fields 1 to 8
//                      MID1 GID1  MID2 GID2 MID3  GID3 MID4 GID4            fields 9 to 16, four pairs to a line
//                      MID5 GID5  -etc.-
// alternate:  MDCONCT  BID  TYPE  TOL  GRID .     .   MODID .
//                      MID1 GID1  ...
// The search location is either the point X, Y, Z in the coordinate system CID of module 0 (fields 4 to 6 and 8) or the
// grid point GRID of module MODID (fields 4 and 7): the card has both layouts, told by field 4, which is an integer
// above zero for a GRID and a real for X (any other number, including an integer, is read as X, as for the CBUSH). A
// card with a GRID has no X, Y, Z or CID, and a MODID needs a GRID; the card keeps `grid` 0 for the first layout. When
// GRID is used it is the independent point of the RBE2.
//
// The (MIDi, GIDi) pairs say which points of which module are tested against the location within TOL: GIDi blank or
// zero is every grid point of the module, so a pair keeps GID 0 and is written with its MID, which a pair of
// module 0 with no GID needs to be there at all (a pair of two blanks is padding). The continuation lines are optional
// (not needed in the automatic search). MIDi may repeat. Scalar points go in GIDi with GRID, X, Y,
// Z, MODID and CID left blank ("should", not checked).
//
// BID numbers the boundary and no two MDCONCT cards have the same one. TOL 0.0 is the default: the tolerance of the
// MDBULK of the module, then the value of the PARAM CONFAC. The module numbers are plain integers; the
// points of module 0 are looked up in this model (a grid or scalar point), the points of the other modules are not, and
// CID names a coordinate system of module 0. The card belongs in the main Bulk Data and is meaningful only
// when the deck has modules; the reader keeps it wherever it stands.
struct ModuleConnectionCard
{
    base::EntityId id = 0;
    ModuleConnectionType type = ModuleConnectionType::Mrbe2;
    double tolerance = 0.0; // 0.0: the tolerance of the MDBULK, else PARAM,CONFAC

    // The location as a point: X, Y, Z in the system CID of module 0 (0 is the basic system).
    double xCoord = 0.0;
    double yCoord = 0.0;
    double zCoord = 0.0;
    base::EntityId frameId = 0;

    // The location as a grid point of a module: GRID is 0 when the location is the point X, Y, Z.
    base::EntityId locationNode = 0;
    base::EntityId moduleId = 0; // the module that holds GRID; 0 is the main Bulk Data

    std::vector<ModuleId> points; // (MIDi, GIDi): GIDi 0 is every grid point of the module

    [[nodiscard]] bool usesGrid() const noexcept { return locationNode != 0; }
    [[nodiscard]] bool operator==(const ModuleConnectionCard&) const noexcept = default;
};

} // namespace oor::model
