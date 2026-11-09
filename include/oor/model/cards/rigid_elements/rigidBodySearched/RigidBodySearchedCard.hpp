#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/sets/DofLists.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace oor::model
{

// TYPE of an RBE2GS: how the two grid points of the RBE2 it generates are chosen. A blank TYPE (std::nullopt in the
// card) searches the whole model, which is the format reference default. The enumerators are numbered in the order of
// RIGID_BODY_SEARCHED_TYPE_WORDS.
enum class RigidBodySearchedType : std::uint8_t
{
    Nmflip,  // NMFLIP: the independent and the dependent grid are exchanged after the search
    Iirbe2,  // IIRBE2: the grids are the independent grids of the two closest existing RBE2 elements
    Nmiirbe2 // NMIIRBE2: IIRBE2 with the independent and the dependent grid exchanged
};

inline constexpr std::array<std::string_view, 3> RIGID_BODY_SEARCHED_TYPE_WORDS{"Swapped", "ClosestRigidBodyStars",
                                                                                "ClosestRigidBodyStarsSwapped"};

// One RBE2GS card: an RBE2 that the solver builds itself. It searches the model for the two grid points closest to a
// search location (a POINT or a GRID named by GS, or the coordinates XS, YS, ZS); the closest becomes the independent
// grid GN and the next one the dependent grid GM of an RBE2 that has the ID of the card. The card names no grid point
// of the rigid body: the library has no connectivity for it, only the search point.
//
// Card layout: RBE2GS  EID  GS  TYPE        TREF  R   CM  ALPHA
//                          XS  YS  ZS  GNi ... ENDL  GMj ...
// Field 4 (the blank cell after TYPE) is not used. The second line starts the search point in its first three fields;
// the lists start in the fourth and go on over the continuation lines. The format draws the same card in several ways
// (the lists written in other places of the lines, a list that starts at the first field of the line, no search point
// when GS is given): they are all this layout, because the lists have no fixed position, only an order and an ENDL.
//
// GNi lists the grids that may not be GN, GMj the grids that may not be GM (the exclusion lists), each as single IDs
// and "a THRU b" ranges that need not all exist. A GNi list must be closed by the word ENDL, and a GMj list comes after
// that ENDL, so a card with a GMj list and no GNi list starts its lists with ENDL. The card keeps the two lists as
// written (runs) and writes the ENDL whenever one of them is not empty; a card with neither list has no ENDL.
//
// R is the search radius. The format reference takes it positive or negative (a negative R makes the two
// grids coincident with the search location), so zero is refused and a blank R is an error. CM, the dependent
// components of GM, is 123456 when blank. TREF and ALPHA are the thermal reference temperature and expansion
// coefficient, 0.0 when blank (the format reference gives the default 0.0 for TREF and a real or blank for ALPHA; the
// RBE2 card stores both as 0.0 and so does this one).
//
// GS and the coordinates are two ways to say where to search, so exactly one of them is given. The
// coordinates count as given when at least one of XS, YS and ZS is not blank (a blank one is 0.0 when the point is
// used); each is kept as written, so a blank is not a zero.
struct RigidBodySearchedCard
{
    static constexpr std::int32_t DEFAULT_COMPONENTS = 123456;

    base::EntityId id = 0;       // an element ID: the space of all the elements
    base::EntityId spotNode = 0; // a POINT or GRID; 0 when blank (the search location is XS, YS, ZS)
    std::optional<RigidBodySearchedType> type;
    double referenceTemperature = 0.0;
    double radius = 0.0;
    std::int32_t dependentComponents = DEFAULT_COMPONENTS; // the digits of the dependent components, ascending
    double alphaCoefficient = 0.0;
    std::optional<double> xStation;
    std::optional<double> yStation;
    std::optional<double> zStation;
    std::vector<IdRun> independentNodes; // GNi: grids that may not be the independent grid
    std::vector<IdRun> dependentNodes;   // GMj: grids that may not be the dependent grid
};

} // namespace oor::model
