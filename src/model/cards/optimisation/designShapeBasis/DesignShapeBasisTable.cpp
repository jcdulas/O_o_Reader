#include "oor/model/cards/optimisation/designShapeBasis/DesignShapeBasisTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIELD_COUNT = 7;
constexpr int FIRST_TERM_FIELD = 2;

} // namespace

std::optional<DesignShapeBasisCard> DesignShapeBasisFormat::read(FieldReader& reader)
{
    DesignShapeBasisCard card;
    card.designVarId = reader.requiredInteger(1, "designVarId");
    card.terms = readShapeTerms(reader, FIRST_TERM_FIELD);
    if (reader.lastUsedField() > FIELD_COUNT)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 7 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> DesignShapeBasisFormat::check(const DesignShapeBasisCard& card)
{
    if (const auto problem = checkRequiredId(card.designVarId, "designVarId"))
        return problem;
    return checkShapeTerms(card.terms);
}

void DesignShapeBasisFormat::write(const DesignShapeBasisCard& card, FieldWriter& writer)
{
    writer.integer(1, card.designVarId);
    writeShapeTerms(writer, FIRST_TERM_FIELD, card.terms);
}

std::uint32_t DesignShapeBasisTable::append(const DesignShapeBasisCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_cards.push(card);
    return row;
}

void DesignShapeBasisTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitId(visitor, "designVarId", ReferenceTarget::DesignVariable, m_cards.edit(row).designVarId);
}

} // namespace oor::model
