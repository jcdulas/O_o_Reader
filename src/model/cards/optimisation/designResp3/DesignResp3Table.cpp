#include "oor/model/cards/optimisation/designResp3/DesignResp3Table.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/optimisation/DesignValue.hpp"
#include "oor/model/cards/optimisation/ResponseArguments.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_GROUP_FIELD = 9;
constexpr int LAST_FIRST_LINE_FIELD = 8; // fields 6 to 8 of the first line are blank in the Card layout

} // namespace

std::optional<DesignResp3Card> DesignResp3Format::read(FieldReader& reader)
{
    DesignResp3Card card;
    card.id = reader.requiredInteger(1, "id");
    card.entryLabel = std::string(reader.text(2));
    if (card.entryLabel.empty())
        reader.error("card.missing-field", std::string("entryLabel") + " is required", "entryLabel");
    card.groupName = std::string(reader.text(3));
    card.type = std::string(reader.text(4));
    card.regionId = reader.integer(5, "regionId", 0);
    for (int field = 6; field <= LAST_FIRST_LINE_FIELD; ++field)
    {
        if (!reader.isBlank(field))
            reader.error("card.bad-value",
                         "fields 6 to 8 of a " + std::string(reader.cardName()) +
                             " are blank: the arguments start on the next line",
                         "");
    }
    card.groups = readDesignGroups(reader, FIRST_GROUP_FIELD, DESIGN_RESP3_GROUPS);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> DesignResp3Format::check(const DesignResp3Card& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkDesignLabel(card.entryLabel, "entryLabel"))
        return problem;
    if (const auto problem = checkNameWord(card.groupName, "groupName"))
        return problem;
    if (const auto problem = checkNameWord(card.type, "kind"))
        return problem;
    if (const auto problem = checkOptionalId(card.regionId, "regionId"))
        return problem;
    return checkResponseArguments(card.groups, DESIGN_RESP3_GROUPS);
}

void DesignResp3Format::write(const DesignResp3Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(2, card.entryLabel);
    writer.text(3, card.groupName);
    writer.text(4, card.type);
    if (card.regionId != 0)
        writer.integer(5, card.regionId);
    writeDesignGroups(writer, FIRST_GROUP_FIELD, card.groups, DESIGN_RESP3_GROUPS);
}

std::uint32_t DesignResp3Table::append(const DesignResp3Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void DesignResp3Table::store(std::uint32_t row, const DesignResp3Card& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void DesignResp3Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void DesignResp3Table::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void DesignResp3Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitResponseArguments(visitor, m_cards.edit(row).groups);
}

} // namespace oor::model
