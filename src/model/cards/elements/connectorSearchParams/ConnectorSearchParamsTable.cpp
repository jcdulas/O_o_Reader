#include "oor/model/cards/elements/connectorSearchParams/ConnectorSearchParamsTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/elements/connectorSearchParams/ConnectorSearchParamsCatalogue.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/core/Model.hpp"

#include <cctype>
#include <cmath>
#include <string>
#include <utility>

namespace oor::model
{

namespace
{

constexpr int FIELDS_PER_PAIR = 2;

// The problem with the parameters of a group (the global ones, or the ones of a section), if there is one.
std::optional<CardProblem> checkParameters(const std::vector<ConnectorSearchParamsParameter>& parameters,
                                           bool afterKeyword)
{
    for (const ConnectorSearchParamsParameter& parameter : parameters)
    {
        // A name is checked as the deck writes it: the keyword of a neutral name, or the word as it was read.
        const std::string label = std::string(parameter.name);
        if (const auto problem = checkNameWord(label, "parameterName"))
            return problem;
        // A name that starts with anything but a letter is a value that lost its name (a field out of place).
        if (std::isalpha(static_cast<unsigned char>(label.front())) == 0)
            return CardProblem{"card.bad-value", "the parameter name " + label + " must start with a letter",
                               "parameterName"};
        if (!parameter.value.isInteger() && !std::isfinite(std::get<double>(parameter.value.number)))
            return CardProblem{"card.bad-value", "the value of " + label + " must be a finite number",
                               "parameterValue"};
        const ConnectorSearchParamsInfo* info = findConnectorSearchParam(parameter.name);
        if (info == nullptr)
            continue; // a name the table does not list is kept as it is
        if (info->onlyGlobal && afterKeyword)
            return CardProblem{"card.bad-value",
                               label + " must be global: it comes before any " +
                                   CardProblem::cardNamed("Fastener") + ", " + CardProblem::cardNamed("Weld") +
                                   " or " + CardProblem::cardNamed("Seam") + " keyword",
                               "parameterName"};
        if (const std::string problem = connectorSearchParamsValueProblem(*info, parameter.value); !problem.empty())
            return CardProblem{"card.bad-value", problem, "parameterValue"};
    }
    return std::nullopt;
}

} // namespace

std::optional<ConnectorSearchParamsCard> ConnectorSearchParamsFormat::read(FieldReader& reader)
{
    ConnectorSearchParamsCard card;
    int pairNumber = 0;
    const int last = reader.lastUsedField();
    // The fields are a run of name and value pairs; a keyword (CFAST, CWELD, CSEAM) takes one field and starts a
    // section, and blank fields between the pairs are for readability.
    for (int field = 1; field <= last;)
    {
        if (reader.isBlank(field))
        {
            ++field;
            continue;
        }
        if (const auto element =
                parseKeyword<ConnectorSearchParamsElement>(CONNECTOR_SEARCH_ELEMENT_WORDS, reader.text(field)))
        {
            card.sections.push_back(ConnectorSearchParamsSection{*element, {}});
            ++field;
            continue;
        }
        ++pairNumber;
        const std::string valueName = "parameterValue" + std::to_string(pairNumber);
        ConnectorSearchParamsParameter parameter;
        const std::string word = std::string(reader.text(field));
        parameter.name = std::string(word);
        if (reader.isBlank(field + 1))
        {
            // The value is in the field right after the name, even across a continuation line.
            reader.error("card.missing-field", "the parameter " + word + " needs its value in the next field",
                         valueName.c_str());
            break;
        }
        if (const auto number = reader.integerOrReal(field + 1, valueName.c_str()))
        {
            parameter.value.number = *number;
            std::vector<ConnectorSearchParamsParameter>& group =
                card.sections.empty() ? card.global : card.sections.back().parameters;
            group.push_back(std::move(parameter));
        }
        field += FIELDS_PER_PAIR;
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ConnectorSearchParamsFormat::check(const ConnectorSearchParamsCard& card)
{
    if (const auto problem = checkParameters(card.global, false))
        return problem;
    for (const ConnectorSearchParamsSection& section : card.sections)
    {
        if (const auto problem = checkParameters(section.parameters, true))
            return problem;
    }
    return std::nullopt;
}

void ConnectorSearchParamsFormat::write(const ConnectorSearchParamsCard& card, FieldWriter& writer)
{
    int field = 1;
    const auto writeParameters = [&](const std::vector<ConnectorSearchParamsParameter>& parameters)
    {
        for (const ConnectorSearchParamsParameter& parameter : parameters)
        {
            writer.text(field, std::string(parameter.name));
            if (parameter.value.isInteger())
                writer.integer(field + 1, std::get<std::int32_t>(parameter.value.number));
            else
                writer.real(field + 1, std::get<double>(parameter.value.number));
            field += FIELDS_PER_PAIR;
        }
    };
    writeParameters(card.global);
    for (const ConnectorSearchParamsSection& section : card.sections)
    {
        writer.text(field, keywordText(CONNECTOR_SEARCH_ELEMENT_WORDS, section.element));
        ++field;
        writeParameters(section.parameters);
    }
}

std::uint32_t ConnectorSearchParamsTable::append(const ConnectorSearchParamsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_cards.push(card);
    return row;
}

void ConnectorSearchParamsTable::finishRead(Model&, const ReadContext& context)
{
    if (m_rows.aliveCount() > 1)
        context.sink.error("deck.duplicate-connector-search-params",
                           "only one " + std::string(NAME) + " may appear in the Bulk Data; the first one is used");
}

} // namespace oor::model
