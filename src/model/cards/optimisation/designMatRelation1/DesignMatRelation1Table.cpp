#include "oor/model/cards/optimisation/designMatRelation1/DesignMatRelation1Table.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/optimisation/DesignField.hpp"
#include "oor/model/cards/optimisation/DesignTargets.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_TERM_FIELD = 9;
constexpr int LAST_FIRST_LINE_FIELD = 8; // field 8 of the first line is blank in the Card layout

} // namespace

std::optional<DesignMatRelation1Card> DesignMatRelation1Format::read(FieldReader& reader)
{
    DesignMatRelation1Card card;
    card.id = reader.requiredInteger(1, "id");
    card.kind = std::string(reader.text(2));
    if (card.kind.empty())
        reader.error("card.missing-field", std::string("kind") + " is required", "kind");
    card.matId = reader.requiredInteger(3, "matId");
    card.name = std::string(reader.text(4));
    if (card.name.empty())
        reader.error("card.missing-field", std::string("fieldName") + " is required", "fieldName");
    card.minValue = reader.optionalReal(5, "minValue");
    card.maxValue = reader.real(6, "maxValue", DesignMatRelation1Card::NO_UPPER_LIMIT);
    card.constant0 = reader.real(7, "constant0", 0.0);
    if (!reader.isBlank(LAST_FIRST_LINE_FIELD))
        reader.error(
            "card.bad-value",
            "field 8 of a " + std::string(reader.cardName()) + " is blank: the design variables start on the next line",
            "");
    card.terms = readDesignTerms(reader, FIRST_TERM_FIELD, "designVarIds", "coefficients", true, card.fromProperty);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> DesignMatRelation1Format::check(const DesignMatRelation1Card& card)
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
    if (const auto problem = checkReal(card.constant0, "constant0", RealRule::Finite))
        return problem;
    return checkDesignTerms(card.terms, card.fromProperty, false, "designVarIds", "coefficients");
}

void DesignMatRelation1Format::write(const DesignMatRelation1Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(2, card.kind);
    writer.integer(3, card.matId);
    writer.text(4, card.name);
    if (card.minValue)
        writer.real(5, *card.minValue);
    writer.realUnlessDefault(6, card.maxValue, DesignMatRelation1Card::NO_UPPER_LIMIT);
    writer.realUnlessDefault(7, card.constant0, 0.0);
    writeDesignTerms(writer, FIRST_TERM_FIELD, card.terms, card.fromProperty);
}

std::uint32_t DesignMatRelation1Table::append(const DesignMatRelation1Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void DesignMatRelation1Table::store(std::uint32_t row, const DesignMatRelation1Card& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void DesignMatRelation1Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void DesignMatRelation1Table::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void DesignMatRelation1Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    DesignMatRelation1Card& card = m_cards.edit(row);
    // A card type that the library does not type has no cards to find.
    if (const auto target = targetOfDesignedCard(card.kind))
        visitId(visitor, "matId", *target, card.matId);
    visitDesignTerms(visitor, card.terms, "designVarIds");
}

} // namespace oor::model
