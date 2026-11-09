#include "oor/model/cards/optimisation/designOptimParams/DesignOptimParamsTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIELDS_PER_ENTRY = 2;

} // namespace

std::optional<DesignOptimParamsCard> DesignOptimParamsFormat::read(FieldReader& reader)
{
    DesignOptimParamsCard card;
    for (int field = 1; field <= reader.lastUsedField(); field += FIELDS_PER_ENTRY)
    {
        const bool noName = reader.isBlank(field);
        const bool noValue = reader.isBlank(field + 1);
        if (noName && noValue)
            continue;
        if (noName)
        {
            reader.error("card.missing-field", "a value needs the name of its parameter", "parameterName");
            break;
        }
        DesignOptimParamsEntry entry;
        entry.name = std::string(reader.text(field));
        entry.value = readDesignValue(reader, field + 1, "value");
        card.entries.push_back(std::move(entry));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> DesignOptimParamsFormat::check(const DesignOptimParamsCard& card)
{
    if (card.entries.empty())
        return CardProblem{"card.missing-field", "at least one parameter is required", "parameterName"};
    for (const DesignOptimParamsEntry& entry : card.entries)
    {
        // A name is checked as the deck writes it: the keyword of a neutral name, or the word as it was read.
        const std::string label = std::string(entry.name);
        if (const auto problem = checkNameWord(label, "parameterName"))
            return problem;
        if (entry.value.isBlank())
            return CardProblem{"card.missing-field", "the parameter " + label + " has no value", "value"};
        if (const std::string problem = designValueProblem(entry.value); !problem.empty())
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + ": " + problem, "value"};
    }
    return std::nullopt;
}

void DesignOptimParamsFormat::write(const DesignOptimParamsCard& card, FieldWriter& writer)
{
    int field = 1;
    for (const DesignOptimParamsEntry& entry : card.entries)
    {
        writer.text(field, std::string(entry.name));
        writeDesignValue(writer, field + 1, entry.value);
        field += FIELDS_PER_ENTRY;
    }
}

std::uint32_t DesignOptimParamsTable::append(const DesignOptimParamsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_cards.push(card);
    return row;
}

} // namespace oor::model
