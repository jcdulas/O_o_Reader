#include "oor/model/cards/superelements/superelementDesignResp3/SuperelementDesignResp3Table.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/optimisation/DesignValue.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_GROUP_FIELD = 9;

} // namespace

std::optional<SuperelementDesignResp3Card> SuperelementDesignResp3Format::read(FieldReader& reader)
{
    SuperelementDesignResp3Card card;
    card.id = reader.requiredInteger(1, "id");
    card.entryLabel = std::string(reader.text(2));
    if (card.entryLabel.empty())
        reader.error("card.missing-field", std::string("entryLabel") + " is required", "entryLabel");
    card.groupName = std::string(reader.text(3));
    card.kind = std::string(reader.text(4));
    card.regionId = reader.integer(5, "regionId", 0);
    card.groups = readSuperelementGroups(reader, FIRST_GROUP_FIELD, SUPERELEMENT_DESIGN_RESP3_GROUPS);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SuperelementDesignResp3Format::check(const SuperelementDesignResp3Card& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkDesignLabel(card.entryLabel, "entryLabel"))
        return problem;
    if (card.groupName.empty())
        return CardProblem{"card.missing-field", std::string(CardProblem::FIELD) + " is required", "groupName"};
    if (const auto problem = checkNameWord(card.groupName, "groupName"))
        return problem;
    if (card.kind.empty())
        return CardProblem{"card.missing-field", std::string(CardProblem::FIELD) + " is required", "kind"};
    if (const auto problem = checkNameWord(card.kind, "kind"))
        return problem;
    if (const auto problem = checkOptionalId(card.regionId, "regionId"))
        return problem;
    return checkSuperelementGroups(card.groups, SUPERELEMENT_DESIGN_RESP3_GROUPS);
}

void SuperelementDesignResp3Format::write(const SuperelementDesignResp3Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(2, card.entryLabel);
    writer.text(3, card.groupName);
    writer.text(4, card.kind);
    if (card.regionId != 0)
        writer.integer(5, card.regionId);
    writeSuperelementGroups(writer, FIRST_GROUP_FIELD, card.groups, SUPERELEMENT_DESIGN_RESP3_GROUPS);
}

std::uint32_t SuperelementDesignResp3Table::append(const SuperelementDesignResp3Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void SuperelementDesignResp3Table::store(std::uint32_t row, const SuperelementDesignResp3Card& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void SuperelementDesignResp3Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void SuperelementDesignResp3Table::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void SuperelementDesignResp3Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitSuperelementGroups(visitor, m_cards.edit(row).groups, SUPERELEMENT_DESIGN_RESP3_GROUPS);
}

} // namespace oor::model
