#include "oor/model/cards/nodes/moduleContactCandidates/ModuleContactCandidatesTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/core/Model.hpp"

#include <string>

namespace oor::model
{

namespace
{

constexpr int MODULE_SET_FIELD = 2;
constexpr int FIRST_PAIR_FIELD = 9; // the first pair of the second line; a line holds two pairs, four fields apart

// The field of the first module of the pair number `index`: the format reference draws two pairs on a line, in cells 1
// and 2 and 5 and 6 of the line.
int pairField(std::size_t index)
{
    const int line = static_cast<int>(index) / 2;
    const int place = static_cast<int>(index) % 2;
    return FIRST_PAIR_FIELD + 8 * line + 4 * place;
}

} // namespace

std::optional<ModuleContactCandidatesCard> ModuleContactCandidatesFormat::read(FieldReader& reader)
{
    ModuleContactCandidatesCard card;
    if (!reader.isBlank(1))
        reader.error("card.bad-value",
                     "field 1 of an " + std::string(reader.cardName()) + " is blank: " + std::string("moduleSetId") +
                         " is in field 2",
                     "moduleSetId");
    card.moduleSetId = reader.integer(MODULE_SET_FIELD, "moduleSetId", -1);
    // The modules of the pairs, whichever fields they are in: every field after SET3ID that is not blank holds one.
    std::vector<base::EntityId> modules;
    for (int field = MODULE_SET_FIELD + 1; field <= reader.lastUsedField(); ++field)
    {
        if (!reader.isBlank(field))
            modules.push_back(reader.requiredInteger(field, "moduleId"));
    }
    if (modules.size() % 2 != 0)
        reader.error("card.missing-field", "the last module has no partner: the modules come in pairs", "moduleId");
    for (std::size_t index = 0; index + 1 < modules.size(); index += 2)
        card.pairs.emplace_back(modules[index], modules[index + 1]);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ModuleContactCandidatesFormat::check(const ModuleContactCandidatesCard& card)
{
    if (card.moduleSetId < -1)
        return CardProblem{"card.bad-id",
                           "{field} must be -1, 0 or the ID of a " + CardProblem::cardNamed("TypedListSet3"),
                           "moduleSetId"};
    for (const auto& [first, second] : card.pairs)
    {
        if (first < 0 || second < 0)
            return CardProblem{"card.bad-id", "a module number must not be negative", "moduleId"};
    }
    return std::nullopt;
}

void ModuleContactCandidatesFormat::write(const ModuleContactCandidatesCard& card, FieldWriter& writer)
{
    writer.integerUnlessDefault(MODULE_SET_FIELD, card.moduleSetId, -1);
    for (std::size_t index = 0; index < card.pairs.size(); ++index)
    {
        writer.integer(pairField(index), card.pairs[index].first);
        writer.integer(pairField(index) + 1, card.pairs[index].second);
    }
}

std::uint32_t ModuleContactCandidatesTable::append(const ModuleContactCandidatesCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_cards.push(card);
    return row;
}

void ModuleContactCandidatesTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // A SET3ID of -1 (every module) or 0 (none) names no set: visitId leaves it alone.
    visitId(visitor, "moduleSetId", ReferenceTarget::Set, m_cards.edit(row).moduleSetId);
}

void ModuleContactCandidatesTable::finishRead(Model&, const ReadContext& context)
{
    if (m_rows.aliveCount() > 1)
        context.sink.error("deck.duplicate-module-contact-candidates",
                           "only one " + std::string(NAME) + " may appear in the Bulk Data");
}

} // namespace oor::model
