#include "oor/model/cards/aero/aeroTrim2/AeroTrim2Table.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/aero/AeroSupport.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <cmath>
#include <string>

namespace oor::model
{

namespace
{

constexpr int EQUIVALENT_ASPECT_RATIO_FIELD = 8;
constexpr int FIRST_VARIABLE_FIELD = 9;

} // namespace

std::optional<AeroTrim2Card> AeroTrim2Format::read(FieldReader& reader)
{
    AeroTrim2Card card;
    card.id = reader.requiredInteger(1, "id");
    card.machNumber = reader.requiredReal(2, "machNumber");
    card.dynamicPressure = reader.requiredReal(3, "dynamicPressure");
    card.equivalentAspectRatio = reader.real(EQUIVALENT_ASPECT_RATIO_FIELD, "equivalentAspectRatio", 1.0);
    // Fields 4 to 7 are blank cells of the layout; the list of variables goes on to the end of the card.
    requireBlankFields(reader, {4, 5, 6, 7}, reader.slotCount());
    const int last = reader.lastUsedField();
    for (int field = FIRST_VARIABLE_FIELD; field <= last; field += 2)
    {
        const bool noLabel = reader.isBlank(field);
        const bool noValue = reader.isBlank(field + 1);
        if (noLabel && noValue)
            continue; // the padding of a line
        if (noLabel)
        {
            reader.report(CardProblem{"card.missing-field", "a value is given without its {field}", "label"});
            continue;
        }
        AeroTrim2Variable variable;
        variable.label = std::string(reader.text(field));
        if (const auto state = parseKeyword<TrimState>(TRIM_STATE_WORDS, reader.text(field + 1)))
            variable.value = *state;
        else
            variable.value =
                reader.requiredReal(field + 1, "value"); // a word that is not FREE, LINKED or SCHED fails here
        card.variables.push_back(std::move(variable));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroTrim2Format::check(const AeroTrim2Card& card)
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
        const AeroTrim2Variable& variable = card.variables[index];
        if (const auto problem = checkTrimLabel(variable.label, "label"))
            return problem;
        if (const double* number = std::get_if<double>(&variable.value); number != nullptr && !std::isfinite(*number))
            return CardProblem{"card.bad-value", "the value of " + variable.label + " must be a finite number",
                               "value"};
        // A variable is given once: two values for one label are a conflict.
        for (std::size_t earlier = 0; earlier < index; ++earlier)
        {
            if (card.variables[earlier].label == variable.label)
                return CardProblem{"card.bad-value", "the label " + variable.label + " is given twice", "label"};
        }
    }
    return std::nullopt;
}

void AeroTrim2Format::write(const AeroTrim2Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.machNumber);
    writer.real(3, card.dynamicPressure);
    writer.realUnlessDefault(EQUIVALENT_ASPECT_RATIO_FIELD, card.equivalentAspectRatio, 1.0);
    int field = FIRST_VARIABLE_FIELD;
    for (const AeroTrim2Variable& variable : card.variables)
    {
        writer.text(field, variable.label);
        if (const double* number = std::get_if<double>(&variable.value))
            writer.real(field + 1, *number);
        else
            writeKeyword(writer, field + 1, TRIM_STATE_WORDS, std::get<TrimState>(variable.value));
        field += 2;
    }
}

std::uint32_t AeroTrim2Table::append(const AeroTrim2Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void AeroTrim2Table::store(std::uint32_t row, const AeroTrim2Card& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void AeroTrim2Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void AeroTrim2Table::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
