#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/FieldWriter.hpp"
#include "oor/model/references/ReferenceVisitor.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <optional>
#include <span>
#include <vector>

namespace oor::model
{

// The ID lists of the thermal cards (QVOL, QBDY1, QBDY3, QVECT, RADBC, RADLST and the grid points of TEMPBC): IDs, with
// "a Through b" and, on the cards that have it, "a Through b By n". One kind of value used inside one family: each card still
// reads and writes its own fields and says what its IDs refer to.

// One entry of the list: a single ID (first == last), "first Through last" (by == 1) or "first Through last By by".
struct ThermalRun
{
    base::EntityId first = 0;
    base::EntityId last = 0;
    base::EntityId increment = 1;

    [[nodiscard]] bool operator==(const ThermalRun&) const noexcept = default;
};

// Reads the list from field `first` to the last used one: IDs, blank internal slots skipped, "Through" and (when `allowBy`) "By". Reports what it finds wrong (Through or By without what they need). `name` is
// the field name of the IDs in the messages.
[[nodiscard]] std::vector<ThermalRun> readThermalRuns(FieldReader& reader, int first, const char* name, bool allowBy);

// Writes the runs from field `first` on; returns the next free field. A range stays together in its internal slot group.
int writeThermalRuns(FieldWriter& writer, int first, const std::vector<ThermalRun>& runs);

// The first problem of a list: at least one ID, no zero; a negative ID (the back face of a surface element) only where
// `allowNegative` says so; a range goes up, By is positive and goes with a range.
[[nodiscard]] std::optional<CardProblem> checkThermalRuns(const std::vector<ThermalRun>& runs, const char* name,
                                                          bool allowNegative);

// The IDs the runs stand for, in the order of the card (a range stands for every ID from the first in steps of By, the
// element or grid point need not exist). A negative single ID stays negative. More than base::MAX_EXPANDED_IDS IDs is a
// LimitExceededError.
[[nodiscard]] std::vector<base::EntityId> expandThermalRuns(const std::vector<ThermalRun>& runs);

// The runs as the triples a table column keeps, and back.
[[nodiscard]] std::vector<std::array<base::EntityId, 3>> toThermalTriples(const std::vector<ThermalRun>& runs);
[[nodiscard]] std::vector<ThermalRun> toThermalRuns(std::span<const std::array<base::EntityId, 3>> triples);

// Visits the IDs of a column of runs: a single ID is `single` (it must exist), the two ends of a range are `range` (the
// IDs between them keep their own, as for the nodes of a Through list). A negative ID is visited by its absolute value and
// keeps its sign.
void visitThermalRuns(VariableLengthColumn<std::array<base::EntityId, 3>>& column, std::uint32_t row,
                      ReferenceVisitor& visitor, const char* name, ReferenceTarget single, ReferenceTarget range);

} // namespace oor::model
