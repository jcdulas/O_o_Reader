#include "oor/model/cards/sets/dofSetRenames/DofSetRenamesTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <set>
#include <string>

namespace oor::model
{

namespace
{

// The field of the old name of pair `index`; the new name is the next one.
int oldNameField(std::size_t index)
{
    return 1 + 2 * static_cast<int>(index);
}

// The names of the fields of the diagnostics, one for each pair (a field name is a literal).
constexpr const char* OLD_NAMES[DOF_SET_MAX_RENAMES] = {"oldName1", "oldName2", "oldName3", "oldName4"};
constexpr const char* NEW_NAMES[DOF_SET_MAX_RENAMES] = {"newName1", "newName2", "newName3", "newName4"};

std::optional<CardProblem> nameProblem(const std::string& name, const char* field)
{
    return checkNameWord(name, field);
}

} // namespace

std::optional<DofSetRenamesCard> DofSetRenamesFormat::read(FieldReader& reader)
{
    DofSetRenamesCard card;
    for (std::size_t index = 0; index < DOF_SET_MAX_RENAMES; ++index)
    {
        const int oldField = oldNameField(index);
        if (reader.isBlank(oldField) && reader.isBlank(oldField + 1))
            continue; // a pair left out
        if (reader.isBlank(oldField))
            reader.error("card.missing-field", std::string(OLD_NAMES[index]) + " is required", OLD_NAMES[index]);
        if (reader.isBlank(oldField + 1))
            reader.error("card.missing-field", std::string(NEW_NAMES[index]) + " is required", NEW_NAMES[index]);
        card.renames.push_back(
            DofSetRenamesRename{std::string(reader.text(oldField)), std::string(reader.text(oldField + 1))});
    }
    if (reader.lastUsedField() > 2 * static_cast<int>(DOF_SET_MAX_RENAMES))
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has four pairs of names", "newName4");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> DofSetRenamesFormat::check(const DofSetRenamesCard& card)
{
    if (card.renames.empty())
        return CardProblem{"card.missing-field", "the card needs at least one pair of names", "oldName1"};
    if (card.renames.size() > DOF_SET_MAX_RENAMES)
        return CardProblem{"card.bad-value", "the card has at most four pairs of names", "oldName4"};
    std::set<std::string> newNames;
    for (std::size_t index = 0; index < card.renames.size(); ++index)
    {
        const DofSetRenamesRename& rename = card.renames[index];
        if (const auto problem = nameProblem(rename.oldName, OLD_NAMES[index]))
            return problem;
        if (const auto problem = nameProblem(rename.newName, NEW_NAMES[index]))
            return problem;
        // a new name is unique among all the set names, so at least not repeated on the card.
        if (!newNames.insert(rename.newName).second)
            return CardProblem{"card.bad-value", "the new names of the card must be different", NEW_NAMES[index]};
    }
    return std::nullopt;
}

void DofSetRenamesFormat::write(const DofSetRenamesCard& card, FieldWriter& writer)
{
    for (std::size_t index = 0; index < card.renames.size(); ++index)
    {
        writer.text(oldNameField(index), card.renames[index].oldName);
        writer.text(oldNameField(index) + 1, card.renames[index].newName);
    }
}

std::uint32_t DofSetRenamesTable::append(const DofSetRenamesCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_renameCount.push(static_cast<std::int32_t>(card.renames.size()));
    m_cards.push(card);
    return row;
}

void DofSetRenamesTable::store(std::uint32_t row, const DofSetRenamesCard& card)
{
    m_renameCount.set(row, static_cast<std::int32_t>(card.renames.size()));
    m_cards.set(row, card);
}

void DofSetRenamesTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_renameCount.keepRows(rows);
    m_cards.keepRows(rows);
}

} // namespace oor::model
