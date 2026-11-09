#include "oor/model/cards/optimisation/designBoundaryShape/DesignBoundaryShapeTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIELD_COUNT = 8;
constexpr int FIRST_TERM_FIELD = 3;

} // namespace

std::optional<DesignBoundaryShapeCard> DesignBoundaryShapeFormat::read(FieldReader& reader)
{
    DesignBoundaryShapeCard card;
    card.designVarId = reader.requiredInteger(1, "designVarId");
    card.auxiliaryModified = reader.requiredInteger(2, "auxiliaryModified");
    card.terms = readShapeTerms(reader, FIRST_TERM_FIELD);
    if (reader.lastUsedField() > FIELD_COUNT)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 8 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> DesignBoundaryShapeFormat::check(const DesignBoundaryShapeCard& card)
{
    if (const auto problem = checkRequiredId(card.designVarId, "designVarId"))
        return problem;
    if (const auto problem = checkRequiredId(card.auxiliaryModified, "auxiliaryModified"))
        return problem;
    return checkShapeTerms(card.terms);
}

void DesignBoundaryShapeFormat::write(const DesignBoundaryShapeCard& card, FieldWriter& writer)
{
    writer.integer(1, card.designVarId);
    writer.integer(2, card.auxiliaryModified);
    writeShapeTerms(writer, FIRST_TERM_FIELD, card.terms);
}

std::uint32_t DesignBoundaryShapeTable::append(const DesignBoundaryShapeCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_cards.push(card);
    return row;
}

void DesignBoundaryShapeTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitId(visitor, "designVarId", ReferenceTarget::DesignVariable, m_cards.edit(row).designVarId);
}

} // namespace oor::model
