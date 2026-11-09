#include "oor/model/cards/optimisation/designMatRelation2/DesignMatRelation2Table.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/optimisation/DesignField.hpp"
#include "oor/model/cards/optimisation/DesignTargets.hpp"
#include "oor/model/cards/optimisation/RelationVariables.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_LINE_END = 8; // field 8 of the first line is blank in the Card layout
constexpr int FIRST_GROUP_FIELD = 9;

} // namespace

std::optional<DesignMatRelation2Card> DesignMatRelation2Format::read(FieldReader& reader)
{
    DesignMatRelation2Card card;
    card.id = reader.requiredInteger(1, "id");
    card.kind = std::string(reader.text(2));
    if (card.kind.empty())
        reader.error("card.missing-field", std::string("kind") + " is required", "kind");
    card.matId = reader.requiredInteger(3, "matId");
    card.name = std::string(reader.text(4));
    if (card.name.empty())
        reader.error("card.missing-field", std::string("fieldName") + " is required", "fieldName");
    card.minValue = reader.optionalReal(5, "minValue");
    card.maxValue = reader.real(6, "maxValue", DesignMatRelation2Card::NO_UPPER_LIMIT);
    card.equationId = reader.requiredInteger(7, "equationId");
    if (!reader.isBlank(FIRST_LINE_END))
        reader.error("card.bad-value",
                     "field 8 of a " + std::string(reader.cardName()) + " is blank: " + std::string("DesignVar") +
                         " and " + std::string("DesignConstants") + " start on the next line",
                     "");
    readRelationVariables(reader, FIRST_GROUP_FIELD, card.designVarIds, card.labels);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> DesignMatRelation2Format::check(const DesignMatRelation2Card& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkNeutralName(card.kind, "kind"))
        return problem;
    if (const auto problem = checkRequiredId(card.matId, "matId"))
        return problem;
    if (const auto problem = checkDesignFieldName(card.name, "fieldName"))
        return problem;
    if (const auto problem = checkOptionalReal(card.minValue, "minValue", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.maxValue, "maxValue", RealRule::Finite))
        return problem;
    if (const auto problem = checkRequiredId(card.equationId, "equationId"))
        return problem;
    return checkRelationVariables(card.designVarIds, card.labels);
}

void DesignMatRelation2Format::write(const DesignMatRelation2Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(2, card.kind);
    writer.integer(3, card.matId);
    writer.text(4, card.name);
    if (card.minValue)
        writer.real(5, *card.minValue);
    writer.realUnlessDefault(6, card.maxValue, DesignMatRelation2Card::NO_UPPER_LIMIT);
    writer.integer(7, card.equationId);
    writeRelationVariables(writer, FIRST_GROUP_FIELD, card.designVarIds, card.labels);
}

std::uint32_t DesignMatRelation2Table::append(const DesignMatRelation2Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void DesignMatRelation2Table::store(std::uint32_t row, const DesignMatRelation2Card& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void DesignMatRelation2Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void DesignMatRelation2Table::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void DesignMatRelation2Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    DesignMatRelation2Card& card = m_cards.edit(row);
    if (const auto target = targetOfDesignedCard(card.kind))
        visitId(visitor, "matId", *target, card.matId);
    visitId(visitor, "equationId", ReferenceTarget::DesignEquation, card.equationId);
    for (base::EntityId& designVarId : card.designVarIds)
        visitId(visitor, "designVarIds", ReferenceTarget::DesignVariable, designVarId);
}

} // namespace oor::model
