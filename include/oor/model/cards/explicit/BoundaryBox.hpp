#pragma once

#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/FieldWriter.hpp"

#include <optional>

// The one value that the barrier and flow cards of the explicit solver share: the box that picks faces of an Euler mesh
// by their coordinates (XMIN to ZMAX of BARRIER, FLOW, FLOWC, FLOWT and FLOWUDS). Where the six fields are on a card,
// and every other field of the card, belong to the card: it reads, writes and checks them itself.
namespace oor::model
{

// The box of a boundary. The format reference gives each axis the default range -1E20 to 1E20, and says that a MAX that
// is blank while its MIN is given is the MIN (a box that is a plane of the mesh boundary). That default depends on
// another field, so the card keeps the values resolved: a box that was never given is the default box, which is also
// what a blank box means, and any other box is written with its six values.
struct BoundaryBox
{
    static constexpr double LOWEST = -1.0E20;
    static constexpr double HIGHEST = 1.0E20;

    double minX = LOWEST;
    double maxX = HIGHEST;
    double minY = LOWEST;
    double maxY = HIGHEST;
    double minZ = LOWEST;
    double maxZ = HIGHEST;

    [[nodiscard]] bool operator==(const BoundaryBox&) const noexcept = default;
};

// The six fields of a card as they are given: a blank is nullopt.
struct BoundaryBoxFields
{
    std::optional<double> minX;
    std::optional<double> maxX;
    std::optional<double> minY;
    std::optional<double> maxY;
    std::optional<double> minZ;
    std::optional<double> maxZ;
};

// The box the fields stand for, with the defaults of the format reference applied axis by axis.
[[nodiscard]] BoundaryBox resolveBoundaryBox(const BoundaryBoxFields& fields) noexcept;

// Reads the six fields from `firstField` on (XMIN, XMAX, YMIN, YMAX, ZMIN, ZMAX) and resolves them.
[[nodiscard]] BoundaryBox readBoundaryBox(FieldReader& reader, int firstField);

// Writes the six fields from `firstField` on. The default box is left blank when the writer omits defaults.
void writeBoundaryBox(FieldWriter& writer, int firstField, const BoundaryBox& box);

// The problem with a box, if there is one: a bound that is not a finite number, or a MAX below its MIN.
[[nodiscard]] std::optional<CardProblem> checkBoundaryBox(const BoundaryBox& box);

} // namespace oor::model
