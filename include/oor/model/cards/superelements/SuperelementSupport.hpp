#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/sets/DofLists.hpp"
#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/FieldWriter.hpp"
#include "oor/model/references/ReferenceVisitor.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

// What the cards of the superelement family have in common. Each helper moves one kind of value: the number of a
// superelement, a number that may be the word ALL, a label, and the way the grid points of a card go to the model of
// the superelement they belong to. No helper describes the fields of a card: every card still lays out, reads, writes
// and checks its own fields by hand.
namespace oor::model
{

// ---------------------------------------------------------------------------------------------------------------------
// The number of a superelement
// ---------------------------------------------------------------------------------------------------------------------

// A superelement number is a positive integer. The residual structure is superelement 0, which only the cards that
// say so accept (SECONCT SEIDB, SEBULK, SEELT with a BEGIN SUPER, SETREE).
[[nodiscard]] std::optional<CardProblem> checkSuperelementId(base::EntityId id, const char* field,
                                                             bool residualAllowed);

// Visits a field that holds the number of a superelement (ReferenceTarget::Superelement) and stores what the visitor
// returns. A blank field and the residual structure (0) are not references to a card, so they are not visited.
inline void visitSuperelement(ReferenceVisitor& visitor, std::string_view field, base::EntityId& id)
{
    if (id > 0)
        id = visitor.visit(field, ReferenceTarget::Superelement, id);
}

// A field that holds the number of a superelement or the word ALL: SEIDB of SEBNDRY and SEEXCLD (blank means ALL), SEID
// of SENQSET. Nothing is ALL.
using SuperelementOrAll = std::optional<base::EntityId>;

// Reads the field: the word ALL (any case) or an integer; blank gives `fallback`.
[[nodiscard]] SuperelementOrAll readSuperelementOrAll(FieldReader& reader, int field, const char* name,
                                                      SuperelementOrAll fallback);

// Writes it. With `blankIsAll` the word ALL is a blank where defaults are left out (SEBNDRY and SEEXCLD: ALL by
// default), otherwise it is always written.
void writeSuperelementOrAll(FieldWriter& writer, int field, const SuperelementOrAll& value, bool blankIsAll);

// ---------------------------------------------------------------------------------------------------------------------
// Points of another superelement
// ---------------------------------------------------------------------------------------------------------------------

// Hands every reference it receives to the model of one superelement (ReferenceVisitor::visitInSuperelement), so that a
// card that names the grid points of superelement SEIDA can use the same helpers for its lists as the cards that name
// its own (`visitIdRuns`, `visitIdList`): it visits them through a scope instead of through the visitor.
class SuperelementScope final : public ReferenceVisitor
{
public:
    SuperelementScope(ReferenceVisitor& visitor, base::EntityId superelement) noexcept
        : m_visitor(visitor), m_superelement(superelement)
    {
    }

    base::EntityId visit(std::string_view field, ReferenceTarget target, base::EntityId id) override
    {
        return m_visitor.visitInSuperelement(field, target, id, m_superelement);
    }

private:
    ReferenceVisitor& m_visitor;
    base::EntityId m_superelement;
};

// Visits one run, in place: a single ID with `single`, the two ends of a THRU range with `range` (the IDs between them
// keep their numbers, as for the nodes of a THRU list).
void visitRun(ReferenceVisitor& visitor, std::string_view field, IdRun& run, ReferenceTarget single,
              ReferenceTarget range);

// Visits the IDs of a list kept as a vector of runs, in place (visitRun for each).
void visitRunList(ReferenceVisitor& visitor, std::string_view field, std::vector<IdRun>& runs, ReferenceTarget single,
                  ReferenceTarget range);

// ---------------------------------------------------------------------------------------------------------------------
// Labels
// ---------------------------------------------------------------------------------------------------------------------

// The longest label of SELABEL: the format reference gives it fields 3 to 9, seven fields of eight characters.

// The problem with a label that is written over several fields, if there is one: at most `maxLength` characters, not
// beginning or ending with a blank (the lexer trims the fields), and no '$', comma, tab or line break (the line would
// not read back as it was written).
[[nodiscard]] std::optional<CardProblem> superelementLabelProblem(std::string_view label, const char* field);

} // namespace oor::model
