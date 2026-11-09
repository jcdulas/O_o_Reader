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

// The ID lists of the contact cards (BSURF, BLSEG, BOUTPUT, BCPROP, BCGRID, BCTABL1, BCBMRAD): IDs, with "a THRU b"
// and, on the cards that have it, "a THRU b BY n". Unlike the lists of the heat transfer cards, a range may go down:
// the format reference gives BSURF, BLSEG and BOUTPUT the increment 1 for ascending numbers and -1 for descending ones
// when BY is left out. One kind of value used inside one family: each card still reads and writes its own fields and
// says what its IDs refer to.

// One entry of the list: a single ID (first == last), "first THRU last" or "first THRU last BY by". `by` is stored
// resolved (1 or -1 when BY is blank) and written only when it is not that default.
struct ContactRun
{
    base::EntityId first = 0;
    base::EntityId last = 0;
    base::EntityId increment = 1;

    [[nodiscard]] bool isRange() const noexcept { return first != last; }
    [[nodiscard]] bool operator==(const ContactRun&) const noexcept = default;
};

// What a card allows in its list.
struct ContactRunRules
{
    bool allowBy = true;
    bool allowDescending = true;
};

// Reads the list in fields `first` to `last`: IDs, blank fields skipped, "THRU" and (with `rules.allowBy`) "BY".
[[nodiscard]] std::vector<ContactRun> readContactRuns(FieldReader& reader, int first, int last, const char* name,
                                                      const ContactRunRules& rules);

// Writes the runs from field `first` on, one field after the other; returns the next free field. A range stays on one
// line (the solver reads the THRU form field by field).
int writeContactRuns(FieldWriter& writer, int first, const std::vector<ContactRun>& runs);

// The first problem of a list: no zero or negative ID, a range goes the way its increment says, BY is not zero.
[[nodiscard]] std::optional<CardProblem> checkContactRuns(const std::vector<ContactRun>& runs, const char* name,
                                                          const ContactRunRules& rules);

// The IDs the runs stand for, in the order of the card. More than base::MAX_EXPANDED_IDS IDs is a LimitExceededError.
[[nodiscard]] std::vector<base::EntityId> expandContactRuns(const std::vector<ContactRun>& runs);

// Visits the IDs of the runs: a single ID is `single` (it must exist), the two ends of a range are `range`.
void visitContactRuns(std::vector<ContactRun>& runs, ReferenceVisitor& visitor, const char* name,
                      ReferenceTarget single, ReferenceTarget range);

// The runs as the triples a table column keeps, and back; and the visit of a column of them.
[[nodiscard]] std::vector<std::array<base::EntityId, 3>> toContactTriples(const std::vector<ContactRun>& runs);
[[nodiscard]] std::vector<ContactRun> toContactRuns(std::span<const std::array<base::EntityId, 3>> triples);
void visitContactTriples(VariableLengthColumn<std::array<base::EntityId, 3>>& column, std::uint32_t row,
                         ReferenceVisitor& visitor, const char* name, ReferenceTarget single, ReferenceTarget range);

} // namespace oor::model
