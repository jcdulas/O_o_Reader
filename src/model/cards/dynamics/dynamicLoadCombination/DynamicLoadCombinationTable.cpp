#include "oor/model/cards/dynamics/dynamicLoadCombination/DynamicLoadCombinationTable.hpp"

#include <algorithm>

namespace oor::model
{

namespace
{

constexpr int FIRST_TERM_FIELD = 3;

} // namespace

std::optional<DynamicLoadCombinationCard> DynamicLoadCombinationFormat::read(FieldReader& reader)
{
    DynamicLoadCombinationCard card;
    card.id = reader.requiredInteger(1, "id");
    card.scaleFactor = reader.requiredReal(2, "scaleFactor");
    // The pairs (Si, Li) follow each other from field 3. A pair with both fields blank is padding.
    const int last = reader.lastUsedField();
    for (int field = FIRST_TERM_FIELD; field <= last && !reader.failed(); field += 2)
    {
        if (reader.isBlank(field) && reader.isBlank(field + 1))
            continue;
        DynamicLoadCombinationTerm term;
        term.scaleFactor = reader.requiredReal(field, "scaleFactor1");
        term.loadSetId = reader.requiredInteger(field + 1, "loadSetId1");
        card.terms.push_back(term);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> DynamicLoadCombinationFormat::check(const DynamicLoadCombinationCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the dynamic load set ID must be a positive integer", "id"};
    if (card.terms.empty())
        return CardProblem{"card.missing-field", "the card needs at least one dynamic load set", "scaleFactor1"};
    for (std::size_t index = 0; index < card.terms.size(); ++index)
    {
        const base::EntityId loadSetId = card.terms[index].loadSetId;
        if (loadSetId <= 0)
            return CardProblem{"card.bad-id", "a dynamic load set ID must be a positive integer", "loadSetId1"};
        // The SID is the ID of the set this card defines: it cannot be one of its own terms.
        if (loadSetId == card.id)
            return CardProblem{"card.bad-value", "the card cannot refer to its own set ID", "loadSetId1"};
        // each Li is different from the others.
        const auto first = card.terms.begin();
        const auto before = first + static_cast<std::ptrdiff_t>(index);
        if (std::any_of(first, before,
                        [loadSetId](const DynamicLoadCombinationTerm& other) { return other.loadSetId == loadSetId; }))
            return CardProblem{"card.bad-value",
                               "the load set IDs " + std::string(CardProblem::FIELD) + " of the card must be different",
                               "loadSetId1"};
    }
    return std::nullopt;
}

void DynamicLoadCombinationFormat::write(const DynamicLoadCombinationCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.scaleFactor);
    int field = FIRST_TERM_FIELD;
    for (const DynamicLoadCombinationTerm& term : card.terms)
    {
        writer.real(field, term.scaleFactor);
        writer.integer(field + 1, term.loadSetId);
        field += 2;
    }
}

std::uint32_t DynamicLoadCombinationTable::append(const DynamicLoadCombinationCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_scaleFactor.push(card.scaleFactor);
    std::vector<double> scales;
    std::vector<base::EntityId> loads;
    for (const DynamicLoadCombinationTerm& term : card.terms)
    {
        scales.push_back(term.scaleFactor);
        loads.push_back(term.loadSetId);
    }
    m_termScaleFactors.push(scales);
    m_termLoadSetIds.push(loads);
    return row;
}

DynamicLoadCombinationCard DynamicLoadCombinationTable::card(std::uint32_t row) const
{
    DynamicLoadCombinationCard card;
    card.id = m_id.get(row);
    card.scaleFactor = m_scaleFactor.get(row);
    const auto scales = m_termScaleFactors.get(row);
    const auto loads = m_termLoadSetIds.get(row);
    for (std::size_t index = 0; index < scales.size(); ++index)
        card.terms.push_back(DynamicLoadCombinationTerm{scales[index], loads[index]});
    return card;
}

void DynamicLoadCombinationTable::store(std::uint32_t row, const DynamicLoadCombinationCard& card)
{
    m_id.set(row, card.id);
    m_scaleFactor.set(row, card.scaleFactor);
    std::vector<double> scales;
    std::vector<base::EntityId> loads;
    for (const DynamicLoadCombinationTerm& term : card.terms)
    {
        scales.push_back(term.scaleFactor);
        loads.push_back(term.loadSetId);
    }
    m_termScaleFactors.set(row, scales);
    m_termLoadSetIds.set(row, loads);
}

void DynamicLoadCombinationTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_scaleFactor.keepRows(rows);
    m_termScaleFactors.keepRows(rows);
    m_termLoadSetIds.keepRows(rows);
}

void DynamicLoadCombinationTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    std::vector<base::EntityId> loads(m_termLoadSetIds.get(row).begin(), m_termLoadSetIds.get(row).end());
    bool changed = false;
    for (base::EntityId& load : loads)
    {
        const base::EntityId updated = visitor.visit("loadSetId1", ReferenceTarget::DynamicLoadSet, load);
        changed = changed || updated != load;
        load = updated;
    }
    if (changed)
        m_termLoadSetIds.set(row, loads);
}

} // namespace oor::model
