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

// A run of elements in the list of a load card: a single element (first == last) or "first Through last".
struct ElementRun
{
    base::EntityId first = 0;
    base::EntityId last = 0;

    [[nodiscard]] bool operator==(const ElementRun&) const noexcept = default;
};

// The element lists of PLOAD2, TEMPP1 and TEMPRB, which all read and write a list of IDs with "Through" between two of
// them as the format reference shows it. One kind of value used by three cards of the family: each card still has its
// own fields.

// Reads the list in fields `first` to the last used one: IDs, blank internal slots skipped,
// and "a Through b" making a run. Reports the problems it finds (Through without a start, a range that runs backwards).
[[nodiscard]] std::vector<ElementRun> readElementRuns(FieldReader& reader, int first, const char* name);

// Writes the runs from slot `first` and returns the next available slot. With `compress`, consecutive IDs become
// Through ranges. A range stays together within the internal slot mapping.
int writeElementRuns(FieldWriter& writer, int first, const std::vector<ElementRun>& runs, bool compress);

// True when the elements, taken in order, are one run of consecutive IDs (a list that a Through can stand for entirely).
[[nodiscard]] bool isOneRun(const std::vector<ElementRun>& runs) noexcept;

// The first problem of a list of runs (IDs positive, a range going up), reported on field `name`.
[[nodiscard]] std::optional<CardProblem> checkElementRuns(const std::vector<ElementRun>& runs, const char* name);

// The runs as the pairs a table column keeps, and back.
[[nodiscard]] std::vector<std::array<base::EntityId, 2>> toPairs(const std::vector<ElementRun>& runs);
[[nodiscard]] std::vector<ElementRun> toRuns(std::span<const std::array<base::EntityId, 2>> pairs);

// Visits the elements of a column of runs: a single element is an Element reference (it must exist), the two ends of a
// range are ElementInRange (the elements between them keep their IDs, as for the nodes of a Through list).
void visitElementRuns(VariableLengthColumn<std::array<base::EntityId, 2>>& column, std::uint32_t row,
                      ReferenceVisitor& visitor, const char* name);

} // namespace oor::model
