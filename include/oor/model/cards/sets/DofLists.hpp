#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/FieldWriter.hpp"
#include "oor/model/references/ReferenceVisitor.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace oor::model
{

// What the cards that list points share: the degree-of-freedom sets (ASET, BSET, CSET, OMIT, QSET, USET and their
// "1" forms, SUPORT, SUPORT1, SPCOFF, SPCOFF1, BNDGRID) and the lists of IDs (SET1, SET3). Two kinds of list, each used
// by a dozen cards of the constraints and sets families: a list of (point, components) pairs, and a list of IDs with
// "Through" between two of them. Each card still has its own fields, layout and rules.

// A run of IDs in the list of a card: a single ID (first == last) or "first Through last".
struct IdRun
{
    base::EntityId first = 0;
    base::EntityId last = 0;

    [[nodiscard]] bool operator==(const IdRun&) const noexcept = default;
};

// Reads the list in fields `first` to the last used one: IDs, blank internal slots skipped,
// and "a Through b" making a run (Through follows a single ID; the end is not below the start). Reports what it finds wrong.
[[nodiscard]] std::vector<IdRun> readIdRuns(FieldReader& reader, int first, const char* name);

// Writes the runs from slot `first` and returns the next available slot. With `compress`, consecutive IDs become
// Through ranges. A range stays together within the internal slot mapping.
int writeIdRuns(FieldWriter& writer, int first, const std::vector<IdRun>& runs, bool compress);

// Which IDs a list accepts: positive ones (the points of the DOF sets), any but zero (SET1: the ZONA decks list
// negative IDs), or zero and above (SET3).
enum class IdSign : std::uint8_t
{
    Positive,
    NotZero,
    NotNegative
};

// The first problem of a list of runs (IDs of the right sign, a range going up), reported on field `name`. A list with
// no ID is a problem too, unless `mayBeEmpty` (SET1 with SKIN).
[[nodiscard]] std::optional<CardProblem> checkIdRuns(const std::vector<IdRun>& runs, const char* name,
                                                     IdSign sign = IdSign::Positive, bool mayBeEmpty = false);

// The runs as the pairs a table column keeps, and back.
[[nodiscard]] std::vector<std::array<base::EntityId, 2>> toPairs(const std::vector<IdRun>& runs);
[[nodiscard]] std::vector<IdRun> toIdRuns(std::span<const std::array<base::EntityId, 2>> pairs);

// Visits the IDs of a column of runs: a single ID with `single`, the two ends of a range with `range` (the IDs between
// them keep their numbers, as for the nodes of a Through list).
void visitIdRuns(VariableLengthColumn<std::array<base::EntityId, 2>>& column, std::uint32_t row,
                 ReferenceVisitor& visitor, const char* name, ReferenceTarget single, ReferenceTarget range);

// Visits the IDs of a column of lists of IDs.
void visitIdList(VariableLengthColumn<base::EntityId>& column, std::uint32_t row, ReferenceVisitor& visitor,
                 const char* name, ReferenceTarget target);

// A grid point or scalar point and the components of it that a card names: the digits of C as one number in ascending
// order (123 for "321"), 0 for a scalar point.
struct DofPair
{
    base::EntityId node = 0;
    std::int32_t components = 0;

    [[nodiscard]] bool operator==(const DofPair&) const noexcept = default;
};

// Where the pairs of a card are: the field of the first point ID, and how many pairs the first line holds. The pairs
// that do not fit go on the continuation lines, from field 9 on, four to a line (ASET: ID1 C1 ... ID4 C4, so the next
// pair is in field 9; USET: SNAME ID1 C1 ID2 C2 ID3 C3, field 8 is blank, the next pair is in field 9).
struct DofPairLayout
{
    int firstField = 1;
    int firstLinePairs = 4;
};

// The field of the point ID of pair `index`; its components are in the next field.
[[nodiscard]] int dofPairField(const DofPairLayout& layout, std::size_t index) noexcept;

// Reads the pairs: a pair is there when its point or its components field is not blank (a blank pair is padding), the
// point is required, blank components are 0.
[[nodiscard]] std::vector<DofPair> readDofPairs(FieldReader& reader, const DofPairLayout& layout);
void writeDofPairs(FieldWriter& writer, const DofPairLayout& layout, const std::vector<DofPair>& pairs);
// The first problem of a list of pairs: at least one, points positive, components 0 or digits 1 to 6 once each.
[[nodiscard]] std::optional<CardProblem> checkDofPairs(const std::vector<DofPair>& pairs);

// The pairs as the two columns a table keeps (the points, the components), and back.
void splitDofPairs(const std::vector<DofPair>& pairs, std::vector<base::EntityId>& nodes,
                   std::vector<std::int32_t>& components);
[[nodiscard]] std::vector<DofPair> joinDofPairs(std::span<const base::EntityId> nodes,
                                                std::span<const std::int32_t> components);

} // namespace oor::model
