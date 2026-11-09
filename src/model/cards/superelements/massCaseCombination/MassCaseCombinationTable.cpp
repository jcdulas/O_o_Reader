#include "oor/model/cards/superelements/massCaseCombination/MassCaseCombinationTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

// The terms (Si, IDi) follow S0 from field 3 without a break between the lines.
constexpr int FIRST_TERM_FIELD = 3;

} // namespace

std::optional<MassCaseCombinationCard> MassCaseCombinationFormat::read(FieldReader& reader)
{
    MassCaseCombinationCard card;
    card.id = reader.requiredInteger(1, "id");
    card.overallScale = reader.real(2, "overallScale", 0.0);
    const int last = reader.lastUsedField();
    for (int field = FIRST_TERM_FIELD; field <= last && !reader.failed(); field += 2)
    {
        if (reader.isBlank(field) && reader.isBlank(field + 1))
            continue; // padding
        MassCaseCombinationTerm term;
        term.scale = reader.real(field, "caseScales", 0.0);
        term.massId = reader.requiredInteger(field + 1, "caseIds");
        card.terms.push_back(term);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> MassCaseCombinationFormat::check(const MassCaseCombinationCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.overallScale, "overallScale", RealRule::NonNegative))
        return problem;
    for (const MassCaseCombinationTerm& term : card.terms)
    {
        if (const auto problem = checkReal(term.scale, "caseScales", RealRule::NonNegative))
            return problem;
        if (const auto problem = checkOptionalId(term.massId, "caseIds"))
            return problem;
    }
    return std::nullopt;
}

void MassCaseCombinationFormat::write(const MassCaseCombinationCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.realUnlessDefault(2, card.overallScale, 0.0);
    int field = FIRST_TERM_FIELD;
    for (const MassCaseCombinationTerm& term : card.terms)
    {
        // A scale of 0.0 is the default, but a term whose scale and number are both blank would read as padding: the
        // number of the case is written, even when it is 0.
        writer.realUnlessDefault(field, term.scale, 0.0);
        writer.integer(field + 1, term.massId);
        field += 2;
    }
}

std::uint32_t MassCaseCombinationTable::append(const MassCaseCombinationCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_overallScale.push(card.overallScale);
    m_cards.push(card);
    return row;
}

void MassCaseCombinationTable::store(std::uint32_t row, const MassCaseCombinationCard& card)
{
    m_id.set(row, card.id);
    m_overallScale.set(row, card.overallScale);
    m_cards.set(row, card);
}

void MassCaseCombinationTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_overallScale.keepRows(rows);
    m_cards.keepRows(rows);
}

void MassCaseCombinationTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void MassCaseCombinationTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    for (MassCaseCombinationTerm& term : m_cards.edit(row).terms)
        visitId(visitor, "id", ReferenceTarget::MassIncrement, term.massId);
}

} // namespace oor::model
