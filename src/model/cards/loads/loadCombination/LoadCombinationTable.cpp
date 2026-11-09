#include "oor/model/cards/loads/loadCombination/LoadCombinationTable.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_TERM_FIELD = 3;

} // namespace

std::optional<LoadCombinationCard> LoadCombinationFormat::read(FieldReader& reader)
{
    LoadCombinationCard card;
    card.id = reader.requiredInteger(1, "id");
    card.scale = reader.requiredReal(2, "scaleFactor");
    // The pairs (Si, Li) follow each other from field 3. A pair with both fields blank is padding.
    const int last = reader.lastUsedField();
    for (int field = FIRST_TERM_FIELD; field <= last && !reader.failed(); field += 2)
    {
        if (reader.isBlank(field) && reader.isBlank(field + 1))
            continue;
        LoadCombinationTerm term;
        term.scale = reader.requiredReal(field, "scaleFactor1");
        term.loadCombinationId = reader.requiredInteger(field + 1, "loadSetId1");
        card.terms.push_back(term);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> LoadCombinationFormat::check(const LoadCombinationCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the load set ID must be a positive integer", "id"};
    if (card.terms.empty())
        return CardProblem{"card.missing-field", "the card needs at least one load set", "scaleFactor1"};
    for (const LoadCombinationTerm& term : card.terms)
    {
        if (term.loadCombinationId <= 0)
            return CardProblem{"card.bad-id", "a load set ID must be a positive integer", "loadSetId1"};
        // a LOAD entry may not reference itself.
        if (term.loadCombinationId == card.id)
            return CardProblem{"card.bad-value", "the card cannot refer to its own set ID", "loadSetId1"};
    }
    return std::nullopt;
}

void LoadCombinationFormat::write(const LoadCombinationCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.scale);
    int field = FIRST_TERM_FIELD;
    for (const LoadCombinationTerm& term : card.terms)
    {
        writer.real(field, term.scale);
        writer.integer(field + 1, term.loadCombinationId);
        field += 2;
    }
}

std::uint32_t LoadCombinationTable::append(const LoadCombinationCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_scale.push(card.scale);
    std::vector<double> scales;
    std::vector<base::EntityId> loads;
    for (const LoadCombinationTerm& term : card.terms)
    {
        scales.push_back(term.scale);
        loads.push_back(term.loadCombinationId);
    }
    m_termScale.push(scales);
    m_termLoad.push(loads);
    return row;
}

LoadCombinationCard LoadCombinationTable::card(std::uint32_t row) const
{
    LoadCombinationCard card;
    card.id = m_id.get(row);
    card.scale = m_scale.get(row);
    const auto scales = m_termScale.get(row);
    const auto loads = m_termLoad.get(row);
    for (std::size_t index = 0; index < scales.size(); ++index)
        card.terms.push_back(LoadCombinationTerm{scales[index], loads[index]});
    return card;
}

void LoadCombinationTable::store(std::uint32_t row, const LoadCombinationCard& card)
{
    m_id.set(row, card.id);
    m_scale.set(row, card.scale);
    std::vector<double> scales;
    std::vector<base::EntityId> loads;
    for (const LoadCombinationTerm& term : card.terms)
    {
        scales.push_back(term.scale);
        loads.push_back(term.loadCombinationId);
    }
    m_termScale.set(row, scales);
    m_termLoad.set(row, loads);
}

void LoadCombinationTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_scale.keepRows(rows);
    m_termScale.keepRows(rows);
    m_termLoad.keepRows(rows);
}

void LoadCombinationTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    std::vector<base::EntityId> loads(m_termLoad.get(row).begin(), m_termLoad.get(row).end());
    bool changed = false;
    for (base::EntityId& load : loads)
    {
        const base::EntityId updated = visitor.visit("loadSetId1", ReferenceTarget::LoadCombinationSet, load);
        changed = changed || updated != load;
        load = updated;
    }
    if (changed)
        m_termLoad.set(row, loads);
}

} // namespace oor::model
