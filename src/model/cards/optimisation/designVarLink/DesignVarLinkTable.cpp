#include "oor/model/cards/optimisation/designVarLink/DesignVarLinkTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_TERM_FIELD = 5;

} // namespace

std::optional<DesignVarLinkCard> DesignVarLinkFormat::read(FieldReader& reader)
{
    DesignVarLinkCard card;
    card.id = reader.requiredInteger(1, "id");
    card.dependent = reader.requiredInteger(2, "dependentVarId");
    card.constant0 = reader.real(3, "constant0", 0.0);
    card.multiplier = reader.real(4, "multiplier", 1.0);
    bool unused = false;
    card.terms = readDesignTerms(reader, FIRST_TERM_FIELD, "independentIds", "coefficients", false, unused);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> DesignVarLinkFormat::check(const DesignVarLinkCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.dependent, "dependentVarId"))
        return problem;
    if (const auto problem = checkReal(card.constant0, "constant0", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.multiplier, "multiplier", RealRule::Finite))
        return problem;
    return checkDesignTerms(card.terms, false, true, "independentIds", "coefficients");
}

void DesignVarLinkFormat::write(const DesignVarLinkCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.dependent);
    writer.realUnlessDefault(3, card.constant0, 0.0);
    writer.realUnlessDefault(4, card.multiplier, 1.0);
    writeDesignTerms(writer, FIRST_TERM_FIELD, card.terms, false);
}

std::uint32_t DesignVarLinkTable::append(const DesignVarLinkCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void DesignVarLinkTable::store(std::uint32_t row, const DesignVarLinkCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void DesignVarLinkTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void DesignVarLinkTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void DesignVarLinkTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    DesignVarLinkCard& card = m_cards.edit(row);
    visitId(visitor, "dependentVarId", ReferenceTarget::DesignVariable, card.dependent);
    visitDesignTerms(visitor, card.terms, "independentIds");
}

} // namespace oor::model
