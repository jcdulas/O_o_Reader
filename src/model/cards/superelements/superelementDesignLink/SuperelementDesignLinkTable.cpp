#include "oor/model/cards/superelements/superelementDesignLink/SuperelementDesignLinkTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/superelements/SuperelementSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIELDS_PER_LINE = 8;
constexpr int FIRST_TERM_FIELD = 9;
constexpr int TERMS_PER_LINE = 2;
constexpr int FIELDS_PER_TERM = 3;

// The first field of term `index`: two to a line, from the first field of each continuation line.
int termField(std::size_t index) noexcept
{
    const int position = static_cast<int>(index);
    return FIRST_TERM_FIELD + FIELDS_PER_LINE * (position / TERMS_PER_LINE) +
           FIELDS_PER_TERM * (position % TERMS_PER_LINE);
}

} // namespace

std::optional<SuperelementDesignLinkCard> SuperelementDesignLinkFormat::read(FieldReader& reader)
{
    SuperelementDesignLinkCard card;
    card.id = reader.requiredInteger(1, "id");
    card.dependentSuperelementId = reader.requiredInteger(2, "dependentSuperelementId");
    card.dependentVarId = reader.requiredInteger(3, "dependentVarId");
    card.constant0 = reader.real(4, "constant0", 0.0);
    card.multiplier = reader.real(5, "multiplier", 1.0);
    if (reader.lastUsedField() >= 6 && reader.lastUsedField() < FIRST_TERM_FIELD)
        reader.error("card.bad-value",
                     "the terms of an " + std::string(reader.cardName()) + " start on the first continuation line",
                     "independentSuperelementIds");
    const int last = reader.lastUsedField();
    for (std::size_t index = 0; termField(index) <= last && !reader.failed(); ++index)
    {
        const int field = termField(index);
        if (reader.isBlank(field) && reader.isBlank(field + 1) && reader.isBlank(field + 2))
            continue; // padding
        SuperelementDesignLinkTerm term;
        term.superelementId = reader.requiredInteger(field, "independentSuperelementId");
        term.designVarId = reader.requiredInteger(field + 1, "independentVarIds");
        term.coefficient = reader.requiredReal(field + 2, "coefficients");
        card.terms.push_back(term);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SuperelementDesignLinkFormat::check(const SuperelementDesignLinkCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkSuperelementId(card.dependentSuperelementId, "dependentSuperelementId", false))
        return problem;
    if (const auto problem = checkRequiredId(card.dependentVarId, "dependentVarId"))
        return problem;
    if (const auto problem = checkReal(card.constant0, "constant0", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.multiplier, "multiplier", RealRule::Finite))
        return problem;
    if (card.terms.empty())
        return CardProblem{"card.missing-field",
                           "the card needs at least one term " + std::string(CardProblem::FIELD) + " IDV1 C1",
                           "independentSuperelementIds"};
    for (const SuperelementDesignLinkTerm& term : card.terms)
    {
        if (const auto problem = checkSuperelementId(term.superelementId, "independentSuperelementId", false))
            return problem;
        if (const auto problem = checkRequiredId(term.designVarId, "independentVarIds"))
            return problem;
        if (const auto problem = checkReal(term.coefficient, "coefficients", RealRule::Finite))
            return problem;
    }
    return std::nullopt;
}

void SuperelementDesignLinkFormat::write(const SuperelementDesignLinkCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.dependentSuperelementId);
    writer.integer(3, card.dependentVarId);
    writer.realUnlessDefault(4, card.constant0, 0.0);
    writer.realUnlessDefault(5, card.multiplier, 1.0);
    std::size_t index = 0;
    for (const SuperelementDesignLinkTerm& term : card.terms)
    {
        const int field = termField(index++);
        writer.integer(field, term.superelementId);
        writer.integer(field + 1, term.designVarId);
        writer.real(field + 2, term.coefficient);
    }
}

std::uint32_t SuperelementDesignLinkTable::append(const SuperelementDesignLinkCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void SuperelementDesignLinkTable::store(std::uint32_t row, const SuperelementDesignLinkCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void SuperelementDesignLinkTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void SuperelementDesignLinkTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void SuperelementDesignLinkTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    SuperelementDesignLinkCard& card = m_cards.edit(row);
    visitSuperelement(visitor, "dependentSuperelementId", card.dependentSuperelementId);
    SuperelementScope dependent(visitor, card.dependentSuperelementId);
    visitId(dependent, "dependentVarId", ReferenceTarget::DesignVariable, card.dependentVarId);
    for (SuperelementDesignLinkTerm& term : card.terms)
    {
        visitSuperelement(visitor, "independentSuperelementId", term.superelementId);
        SuperelementScope independent(visitor, term.superelementId);
        visitId(independent, "independentVarIds", ReferenceTarget::DesignVariable, term.designVarId);
    }
}

} // namespace oor::model
