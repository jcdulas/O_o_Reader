#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nonlinear_contact/ContactSections.hpp"
#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/FieldWriter.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

// The control data of a NURBS curve or surface of a rigid contact body, as BCNURB2, BCNURBS, BCTRIM and the NURBS forms
// of BCBODY lay it out: keyword lists on lines of their own. One kind of value used inside one family.
//
//   GRID  G1 G2 G3 ...        the grid points of the control points      (or)
//   COORD X1 Y1 [Z1] X2 ...   the coordinates of the control points, with no grid points
//   HOMO  Homo1 Homo2 ...     the homogeneous coordinates (0.0 to 1.0), one for each control point
//   KNOT  Knot1 Knot2 ...     the knot vector(s) (0.0 to 1.0)
// The list of values after a keyword goes on over the next lines (seven to a line, the first field of those lines
// blank).
struct ContactNurbsData
{
    bool byCoordinates = false;        // COORD in place of GRID
    std::vector<base::EntityId> grids; // GRID
    std::vector<double> coordinates;   // COORD
    std::vector<double> homogeneous;   // HOMO
    std::vector<double> knots;         // KNOT

    [[nodiscard]] bool operator==(const ContactNurbsData&) const = default;
};

// The keywords of the lists, for the section splitter of the card: GRID, COORD, HOMO and KNOT.
inline constexpr std::string_view NURBS_GRID = "Node";
inline constexpr std::string_view NURBS_COORD = "Coordinates";
inline constexpr std::string_view NURBS_HOMO = "HomogeneousCoordinates";
inline constexpr std::string_view NURBS_KNOT = "KnotVector";

// Reads the GRID or COORD, HOMO and KNOT sections of a card that were found by splitContactSections. A list may not
// be given twice, GRID and COORD cannot coexist.
[[nodiscard]] ContactNurbsData readContactNurbsData(FieldReader& reader, const std::vector<ContactSection>& sections);

// Writes the lists, each on lines of its own, from the line that starts at field `lineStart`; returns the first field
// of the next free line. A list that is empty is not written.
int writeContactNurbsData(FieldWriter& writer, int lineStart, const ContactNurbsData& data);

// The first problem of the data, given how many control points the card says (`points`), how many values each has when
// it is given by coordinates (`coordinatesPerPoint`: 2 or 3) and how many knots. `name` of the card is in the messages.
[[nodiscard]] std::optional<CardProblem> checkContactNurbsData(const ContactNurbsData& data, std::int64_t points,
                                                               int coordinatesPerPoint, std::int64_t knots);

} // namespace oor::model
