#include "oor/model/cards/aero/aeroTrim/AeroTrimTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/aero/AeroSupport.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <cmath>
#include <string>

namespace oor::model
{

namespace
{

constexpr int EQUIVALENT_ASPECT_RATIO_FIELD = 8;

// The field of the label of pair `index` (its value is in the next field): the first two pairs are in fields 4 and 6,
// AEQR has field 8, and the pairs after them start the continuation line at field 9.
int labelField(std::size_t index) noexcept
{
    return index < 2 ? 4 + 2 * static_cast<int>(index) : 9 + 2 * static_cast<int>(index - 2);
}

} // namespace

std::optional<AeroTrimCard> AeroTrimFormat::read(FieldReader& reader)
{
    AeroTrimCard card;
    card.id = reader.requiredInteger(1, "id");
    card.machNumber = reader.requiredReal(2, "machNumber");
    card.dynamicPressure = reader.requiredReal(3, "dynamicPressure");
    card.equivalentAspectRatio = reader.real(EQUIVALENT_ASPECT_RATIO_FIELD, "equivalentAspectRatio", 1.0);
    const int last = reader.lastUsedField();
    for (std::size_t index = 0; labelField(index) <= last; ++index)
    {
        const int field = labelField(index);
        const std::string name = "label" + std::to_string(index + 1);
        const std::string valueName = "value" + std::to_string(index + 1);
        if (reader.isBlank(field))
        {
            // A pair with neither field is the padding of a line.
            if (!reader.isBlank(field + 1))
                reader.error("card.missing-field", std::string(name) + " is required for " + std::string(valueName),
                             name.c_str());
            continue;
        }
        card.variables.push_back(
            AeroTrimVariable{std::string(reader.text(field)), reader.requiredReal(field + 1, valueName.c_str())});
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroTrimFormat::check(const AeroTrimCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.machNumber, "machNumber", RealRule::NonNegative))
        return problem;
    if (card.machNumber == 1.0)
        return CardProblem{"card.bad-value", "{field} cannot be 1.0", "machNumber"};
    if (const auto problem = checkReal(card.dynamicPressure, "dynamicPressure", RealRule::Positive))
        return problem;
    if (const auto problem = checkRealRange(card.equivalentAspectRatio, "equivalentAspectRatio", 0.0, 1.0))
        return problem;
    for (std::size_t index = 0; index < card.variables.size(); ++index)
    {
        const AeroTrimVariable& variable = card.variables[index];
        // The field name of a problem is a plain string that outlives this function, so every pair is "labels" here
        // (the reader, which reports while it reads, says label1, label2, ...).
        if (const auto problem = checkTrimLabel(variable.label, "labels"))
            return problem;
        if (!std::isfinite(variable.value))
            return CardProblem{"card.bad-value", "the value of " + variable.label + " must be a finite number",
                               "values"};
        // A variable is fixed once: two values for one label are a conflict.
        for (std::size_t earlier = 0; earlier < index; ++earlier)
        {
            if (card.variables[earlier].label == variable.label)
                return CardProblem{"card.bad-value", "the label " + variable.label + " is given twice", "labels"};
        }
    }
    return std::nullopt;
}

void AeroTrimFormat::write(const AeroTrimCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.machNumber);
    writer.real(3, card.dynamicPressure);
    writer.realUnlessDefault(EQUIVALENT_ASPECT_RATIO_FIELD, card.equivalentAspectRatio, 1.0);
    for (std::size_t index = 0; index < card.variables.size(); ++index)
    {
        writer.text(labelField(index), card.variables[index].label);
        writer.real(labelField(index) + 1, card.variables[index].value);
    }
}

std::uint32_t AeroTrimTable::append(const AeroTrimCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void AeroTrimTable::store(std::uint32_t row, const AeroTrimCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void AeroTrimTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void AeroTrimTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void AeroTrimTable::visitReferences(std::uint32_t, ReferenceVisitor&)
{
    // The variables are named by label, not by number: a renumbering has nothing to follow.
}

} // namespace oor::model
