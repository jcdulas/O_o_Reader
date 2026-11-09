#include "oor/model/cards/nodes/moduleEntryExclusion/ModuleEntryExclusionTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<ModuleEntryExclusionCard> ModuleEntryExclusionFormat::read(FieldReader& reader)
{
    ModuleEntryExclusionCard card;
    // The non-blank fields are the names: a blank one is the padding of a line.
    const int last = reader.lastUsedField();
    for (int field = 1; field <= last; ++field)
    {
        if (!reader.isBlank(field))
            card.entries.emplace_back(reader.text(field));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ModuleEntryExclusionFormat::check(const ModuleEntryExclusionCard& card)
{
    if (card.entries.empty())
        return CardProblem{"card.missing-field", "the card names at least one Bulk Data entry", "entry1"};
    for (const std::string& entry : card.entries)
    {
        if (const auto problem = checkCardTypeWord(entry, "entry"))
            return problem;
        // The grid points of the primary module cannot be ignored or replaced.
        if (entry == "Node")
            return CardProblem{"card.bad-value", "Node cannot be excluded (a fatal error)", "entry"};
    }
    return std::nullopt;
}

void ModuleEntryExclusionFormat::write(const ModuleEntryExclusionCard& card, FieldWriter& writer)
{
    int field = 1;
    for (const std::string& entry : card.entries)
        writer.text(field++, entry);
}

std::uint32_t ModuleEntryExclusionTable::append(const ModuleEntryExclusionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_cards.push(card);
    return row;
}

} // namespace oor::model
