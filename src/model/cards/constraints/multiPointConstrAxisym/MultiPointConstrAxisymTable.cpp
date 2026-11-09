#include "oor/model/cards/constraints/multiPointConstrAxisym/MultiPointConstrAxisymTable.hpp"

#include "oor/model/cards/sets/DofLists.hpp"

#include <string>

namespace oor::model
{

namespace
{

constexpr int FIRST_TERM_FIELD = 5;
constexpr int FIELDS_PER_TERM = 4;

// The field of the RID of term `index`: four fields a term, from field 5 on, over the continuation lines.
int firstFieldOf(std::size_t index)
{
    return FIRST_TERM_FIELD + FIELDS_PER_TERM * static_cast<int>(index);
}

} // namespace

std::optional<MultiPointConstrAxisymCard> MultiPointConstrAxisymFormat::read(FieldReader& reader)
{
    MultiPointConstrAxisymCard card;
    card.id = reader.requiredInteger(1, "id");
    const int last = reader.lastUsedField();
    for (std::size_t index = 0; firstFieldOf(index) <= last && !reader.failed(); ++index)
    {
        const int field = firstFieldOf(index);
        if (reader.isBlank(field) && reader.isBlank(field + 1) && reader.isBlank(field + 2) &&
            reader.isBlank(field + 3))
            continue; // padding
        MultiPointConstrAxisymTerm term;
        term.ringId = reader.requiredInteger(field, "ringId");
        term.harmonic = reader.requiredInteger(field + 1, "harmonicId");
        term.component = reader.requiredInteger(field + 2, "components");
        // format reference: Aj defaults to 0.0, except A1, which must be given and not be zero.
        term.coefficient = card.terms.empty() ? reader.requiredReal(field + 3, "coefficient")
                                              : reader.real(field + 3, "coefficient", 0.0);
        card.terms.push_back(term);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> MultiPointConstrAxisymFormat::check(const MultiPointConstrAxisymCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the constraint set ID must be a positive integer", "id"};
    if (card.terms.empty())
        return CardProblem{"card.missing-field", "the card needs at least one term", "ringId1"};
    for (const MultiPointConstrAxisymTerm& term : card.terms)
    {
        if (term.ringId <= 0)
            return CardProblem{"card.bad-id", "a ring ID must be a positive integer", "ringId1"};
        if (term.harmonic < 0)
            return CardProblem{"card.bad-value", "a harmonic ID must not be negative", "harmonicId1"};
        if (term.component < 1 || term.component > 6)
            return CardProblem{"card.bad-value", "C must be one of the integers 1 to 6", "components1"};
    }
    if (card.terms.front().coefficient == 0.0)
        return CardProblem{
            "card.bad-value",
            "the coefficient of the dependent term (" + std::string(CardProblem::FIELD) + ") must not be zero",
            "coefficient1"};
    return std::nullopt;
}

void MultiPointConstrAxisymFormat::write(const MultiPointConstrAxisymCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    std::size_t index = 0;
    for (const MultiPointConstrAxisymTerm& term : card.terms)
    {
        const int field = firstFieldOf(index++);
        writer.integer(field, term.ringId);
        writer.integer(field + 1, term.harmonic);
        writer.integer(field + 2, term.component);
        writer.real(field + 3, term.coefficient);
    }
}

std::uint32_t MultiPointConstrAxisymTable::append(const MultiPointConstrAxisymCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    std::vector<base::EntityId> rings;
    std::vector<std::int32_t> harmonics;
    std::vector<std::int32_t> components;
    std::vector<double> coefficients;
    for (const MultiPointConstrAxisymTerm& term : card.terms)
    {
        rings.push_back(term.ringId);
        harmonics.push_back(term.harmonic);
        components.push_back(term.component);
        coefficients.push_back(term.coefficient);
    }
    m_ringId.push(rings);
    m_harmonic.push(harmonics);
    m_component.push(components);
    m_coefficient.push(coefficients);
    return row;
}

MultiPointConstrAxisymCard MultiPointConstrAxisymTable::card(std::uint32_t row) const
{
    MultiPointConstrAxisymCard card;
    card.id = m_id.get(row);
    const auto rings = m_ringId.get(row);
    const auto harmonics = m_harmonic.get(row);
    const auto components = m_component.get(row);
    const auto coefficients = m_coefficient.get(row);
    for (std::size_t index = 0; index < rings.size(); ++index)
        card.terms.push_back(
            MultiPointConstrAxisymTerm{rings[index], harmonics[index], components[index], coefficients[index]});
    return card;
}

void MultiPointConstrAxisymTable::store(std::uint32_t row, const MultiPointConstrAxisymCard& card)
{
    m_id.set(row, card.id);
    std::vector<base::EntityId> rings;
    std::vector<std::int32_t> harmonics;
    std::vector<std::int32_t> components;
    std::vector<double> coefficients;
    for (const MultiPointConstrAxisymTerm& term : card.terms)
    {
        rings.push_back(term.ringId);
        harmonics.push_back(term.harmonic);
        components.push_back(term.component);
        coefficients.push_back(term.coefficient);
    }
    m_ringId.set(row, rings);
    m_harmonic.set(row, harmonics);
    m_component.set(row, components);
    m_coefficient.set(row, coefficients);
}

void MultiPointConstrAxisymTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_ringId.keepRows(rows);
    m_harmonic.keepRows(rows);
    m_component.keepRows(rows);
    m_coefficient.keepRows(rows);
}

void MultiPointConstrAxisymTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The rings are RINGAX points, which are in the node ID space.
    visitIdList(m_ringId, row, visitor, "ringId", ReferenceTarget::Node);
}

} // namespace oor::model
