#include "oor/model/cards/rigid_elements/rigidBodyStar/RigidBodyStarTable.hpp"

#include "oor/base/text/DigitSets.hpp"

#include <variant>

namespace oor::model
{

namespace
{

constexpr int FIRST_DEPENDENT_FIELD = 4;

// True when the field holds a real (a number that is not an integer). Reports an error for text that is neither.
bool isReal(FieldReader& reader, int field)
{
    const auto value = reader.integerOrReal(field, "dependentNode");
    return value && std::holds_alternative<double>(*value);
}

} // namespace

std::optional<RigidBodyStarCard> RigidBodyStarFormat::read(FieldReader& reader)
{
    RigidBodyStarCard card;
    card.id = reader.requiredInteger(1, "id");
    card.independent = reader.requiredInteger(2, "independentNode");
    card.components = base::sortedDigits(reader.requiredInteger(3, "dependentComponents"));

    // After the last grid point there may be ALPHA, or ALPHA and TREF: the reals at the end of the card. A blank
    // before a final real is a blank ALPHA.
    int lastDependentField = reader.lastUsedField();
    if (lastDependentField >= FIRST_DEPENDENT_FIELD && isReal(reader, lastDependentField))
    {
        const int before = lastDependentField - 1;
        if (before >= FIRST_DEPENDENT_FIELD && (reader.isBlank(before) || isReal(reader, before)))
        {
            card.alphaCoefficient = reader.real(before, "alphaCoefficient", 0.0);
            card.referenceTemperature = reader.real(lastDependentField, "referenceTemperature", 0.0);
            lastDependentField -= 2;
        }
        else
        {
            card.alphaCoefficient = reader.real(lastDependentField, "alphaCoefficient", 0.0);
            lastDependentField -= 1;
        }
    }

    for (int field = FIRST_DEPENDENT_FIELD; field <= lastDependentField && !reader.failed(); ++field)
    {
        if (!reader.isBlank(field)) // blank fields are the padding of the continuation lines
            card.dependents.push_back(reader.requiredInteger(field, "dependentNode"));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RigidBodyStarFormat::check(const RigidBodyStarCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (card.independent <= 0)
        return CardProblem{
            "card.bad-id",
            "the independent grid point " + std::string(CardProblem::FIELD) + " must be a positive integer",
            "independentNode"};
    if (card.components <= 0 || !base::isComponentSet(card.components))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be a list of the digits 1 to 6, each at most once",
                           "dependentComponents"};
    if (card.dependents.empty())
        return CardProblem{"card.missing-field", "the element needs at least one dependent grid point",
                           "dependentNode1"};
    for (const base::EntityId dependent : card.dependents)
    {
        if (dependent <= 0)
            return CardProblem{"card.bad-id", "a dependent grid point ID must be a positive integer", "dependentNode1"};
    }
    return std::nullopt;
}

void RigidBodyStarFormat::write(const RigidBodyStarCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.independent);
    writer.integer(3, card.components);
    int field = FIRST_DEPENDENT_FIELD;
    for (const base::EntityId dependent : card.dependents)
        writer.integer(field++, dependent);
    // ALPHA and TREF in the next two fields. When only TREF is not zero, ALPHA is written as 0.0 so that the reals at
    // the end of the card read back as the same two values.
    if (card.alphaCoefficient != 0.0 || card.referenceTemperature != 0.0)
    {
        writer.real(field++, card.alphaCoefficient);
        writer.realUnlessDefault(field, card.referenceTemperature, 0.0);
    }
}

std::uint32_t RigidBodyStarTable::append(const RigidBodyStarCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_independent.push(card.independent);
    m_components.push(card.components);
    m_dependents.push(card.dependents);
    m_alphaCoefficient.push(card.alphaCoefficient);
    m_referenceTemperature.push(card.referenceTemperature);
    return row;
}

RigidBodyStarCard RigidBodyStarTable::card(std::uint32_t row) const
{
    RigidBodyStarCard card;
    card.id = m_id.get(row);
    card.independent = m_independent.get(row);
    card.components = m_components.get(row);
    const auto dependents = m_dependents.get(row);
    card.dependents.assign(dependents.begin(), dependents.end());
    card.alphaCoefficient = m_alphaCoefficient.get(row);
    card.referenceTemperature = m_referenceTemperature.get(row);
    return card;
}

void RigidBodyStarTable::store(std::uint32_t row, const RigidBodyStarCard& card)
{
    m_id.set(row, card.id);
    m_independent.set(row, card.independent);
    m_components.set(row, card.components);
    m_dependents.set(row, card.dependents);
    m_alphaCoefficient.set(row, card.alphaCoefficient);
    m_referenceTemperature.set(row, card.referenceTemperature);
}

void RigidBodyStarTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_independent.keepRows(rows);
    m_components.keepRows(rows);
    m_dependents.keepRows(rows);
    m_alphaCoefficient.keepRows(rows);
    m_referenceTemperature.keepRows(rows);
}

void RigidBodyStarTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    m_independent.set(row, visitor.visit("independentNode", ReferenceTarget::Node, m_independent.get(row)));
    std::vector<base::EntityId> dependents(m_dependents.get(row).begin(), m_dependents.get(row).end());
    bool changed = false;
    for (base::EntityId& dependent : dependents)
    {
        const base::EntityId updated = visitor.visit("dependentNode", ReferenceTarget::Node, dependent);
        changed = changed || updated != dependent;
        dependent = updated;
    }
    if (changed)
        m_dependents.set(row, dependents);
}

} // namespace oor::model
