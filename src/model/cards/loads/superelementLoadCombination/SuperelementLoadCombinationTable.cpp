#include "oor/model/cards/loads/superelementLoadCombination/SuperelementLoadCombinationTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_TERM_FIELD = 3;

} // namespace

std::optional<SuperelementLoadCombinationCard> SuperelementLoadCombinationFormat::read(FieldReader& reader)
{
    SuperelementLoadCombinationCard card;
    card.id = reader.requiredInteger(1, "id");
    card.scaleFactor = reader.requiredReal(2, "scaleFactor");
    // The pairs (Si, IDVi) follow each other from field 3. A pair with both fields blank is the padding of a line.
    const int last = reader.lastUsedField();
    for (int field = FIRST_TERM_FIELD; field <= last; field += 2)
    {
        if (reader.isBlank(field) && reader.isBlank(field + 1))
            continue;
        SuperelementLoadCombinationTerm term;
        term.scale = reader.requiredReal(field, "scaleFactor1");
        term.loadId = reader.requiredInteger(field + 1, "loadIds");
        card.terms.push_back(term);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SuperelementLoadCombinationFormat::check(const SuperelementLoadCombinationCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.scaleFactor, "scaleFactor", RealRule::Finite))
        return problem;
    if (card.terms.empty())
        return CardProblem{"card.missing-field",
                           "the card needs at least one load vector (" + std::string(CardProblem::FIELD) + " and " +
                               CardProblem::fieldNamed("loadIds") + ")",
                           "scaleFactor1"};
    for (const SuperelementLoadCombinationTerm& term : card.terms)
    {
        if (const auto problem = checkReal(term.scale, "scaleFactor1", RealRule::Finite))
            return problem;
        if (const auto problem = checkRequiredId(term.loadId, "loadIds"))
            return problem;
    }
    return std::nullopt;
}

void SuperelementLoadCombinationFormat::write(const SuperelementLoadCombinationCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.scaleFactor);
    int field = FIRST_TERM_FIELD;
    for (const SuperelementLoadCombinationTerm& term : card.terms)
    {
        writer.real(field, term.scale);
        writer.integer(field + 1, term.loadId);
        field += 2;
    }
}

std::uint32_t SuperelementLoadCombinationTable::append(const SuperelementLoadCombinationCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void SuperelementLoadCombinationTable::store(std::uint32_t row, const SuperelementLoadCombinationCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void SuperelementLoadCombinationTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void SuperelementLoadCombinationTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void SuperelementLoadCombinationTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    for (SuperelementLoadCombinationTerm& term : m_cards.edit(row).terms)
        visitId(visitor, "loadIds", ReferenceTarget::ExcitationSet, term.loadId);
}

} // namespace oor::model
