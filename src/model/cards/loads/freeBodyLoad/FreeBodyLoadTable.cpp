#include "oor/model/cards/loads/freeBodyLoad/FreeBodyLoadTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/loads/FreeBodySupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"
#include "oor/model/core/Model.hpp"

#include <set>

namespace oor::model
{

std::optional<FreeBodyLoadCard> FreeBodyLoadFormat::read(FieldReader& reader)
{
    FreeBodyLoadCard card;
    card.name = std::string(reader.text(1));
    card.subsystemName = std::string(reader.text(2));
    // The cells of the first line after FBODYSB are blank in the layout of the format; the label is the second line.
    requireBlankFields(reader, {3, 4, 5, 6, 7, 8}, FREE_BODY_LABEL_LAST_FIELD);
    card.label = readFreeBodyLabel(reader);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FreeBodyLoadFormat::check(const FreeBodyLoadCard& card)
{
    if (const auto problem = checkFreeBodyName(card.name, "name"))
        return problem;
    if (const auto problem = checkFreeBodyName(card.subsystemName, "subsystemName"))
        return problem;
    return checkFreeBodyLabel(card.label);
}

void FreeBodyLoadFormat::write(const FreeBodyLoadCard& card, FieldWriter& writer)
{
    writer.text(1, card.name);
    writer.text(2, card.subsystemName);
    writeFreeBodyLabel(writer, card.label);
}

std::uint32_t FreeBodyLoadTable::append(const FreeBodyLoadCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_name.push(card.name);
    m_subsystemName.push(card.subsystemName);
    m_label.push(card.label);
    return row;
}

FreeBodyLoadCard FreeBodyLoadTable::card(std::uint32_t row) const
{
    FreeBodyLoadCard card;
    card.name = m_name.get(row);
    card.subsystemName = m_subsystemName.get(row);
    card.label = m_label.get(row);
    return card;
}

void FreeBodyLoadTable::store(std::uint32_t row, const FreeBodyLoadCard& card)
{
    m_name.set(row, card.name);
    m_subsystemName.set(row, card.subsystemName);
    m_label.set(row, card.label);
}

void FreeBodyLoadTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_name.keepRows(rows);
    m_subsystemName.keepRows(rows);
    m_label.keepRows(rows);
}

void FreeBodyLoadTable::finishRead(Model&, const ReadContext& context)
{
    // The name of a load case is compared without regard to the case, like every name a card gives another card.
    std::set<std::string> names;
    std::set<std::string> reported;
    for (std::uint32_t row = 0; row < m_rows.rowCount(); ++row)
    {
        if (!m_rows.isAlive(row))
            continue;
        const std::string name = m_name.get(row);
        const std::string key = std::string(name);
        if (!names.insert(key).second && reported.insert(key).second)
            context.sink.error("deck.duplicate-free-body-load",
                               "the " + std::string(NAME) + " name " + name + " is used by more than one card");
    }
}

} // namespace oor::model
