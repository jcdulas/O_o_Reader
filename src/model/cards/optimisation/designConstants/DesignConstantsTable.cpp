#include "oor/model/cards/optimisation/designConstants/DesignConstantsTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/optimisation/DesignValue.hpp"

#include <variant>

namespace oor::model
{

namespace
{

constexpr int FIELDS_PER_ENTRY = 2;

} // namespace

std::optional<DesignConstantsCard> DesignConstantsFormat::read(FieldReader& reader)
{
    DesignConstantsCard card;
    for (int field = 1; field <= reader.lastUsedField(); field += FIELDS_PER_ENTRY)
    {
        const bool noLabel = reader.isBlank(field);
        const bool noValue = reader.isBlank(field + 1);
        if (noLabel && noValue)
            continue;
        if (noLabel || noValue)
        {
            reader.error("card.missing-field", "a label and its value go together: one of them is blank",
                         noLabel ? "label" : "value");
            break;
        }
        DesignConstantsEntry entry;
        entry.label = std::string(reader.text(field));
        const auto value = reader.integerOrReal(field + 1, "value");
        if (!value)
            break; // an error was reported
        if (std::holds_alternative<std::int32_t>(*value))
            entry.table = std::get<std::int32_t>(*value);
        else
            entry.value = std::get<double>(*value);
        card.entries.push_back(std::move(entry));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> DesignConstantsFormat::check(const DesignConstantsCard& card)
{
    if (card.entries.empty())
        return CardProblem{"card.missing-field", "at least one constant is required", "label"};
    for (const DesignConstantsEntry& entry : card.entries)
    {
        if (const auto problem = checkDesignLabel(entry.label, "label"))
            return problem;
        if (const auto problem = checkOptionalId(entry.table, "value"))
            return problem;
        if (entry.table == 0)
        {
            if (const auto problem = checkReal(entry.value, "value", RealRule::Finite))
                return problem;
        }
    }
    return std::nullopt;
}

void DesignConstantsFormat::write(const DesignConstantsCard& card, FieldWriter& writer)
{
    int field = 1;
    for (const DesignConstantsEntry& entry : card.entries)
    {
        writer.text(field, entry.label);
        if (entry.table > 0)
            writer.integer(field + 1, entry.table);
        else
            writer.real(field + 1, entry.value);
        field += FIELDS_PER_ENTRY;
    }
}

std::uint32_t DesignConstantsTable::append(const DesignConstantsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_cards.push(card);
    return row;
}

void DesignConstantsTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    for (DesignConstantsEntry& entry : m_cards.edit(row).entries)
        visitId(visitor, "value", ReferenceTarget::DynamicTable, entry.table);
}

} // namespace oor::model
