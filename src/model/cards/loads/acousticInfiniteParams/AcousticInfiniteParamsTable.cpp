#include "oor/model/cards/loads/acousticInfiniteParams/AcousticInfiniteParamsTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/loads/acousticInfiniteParams/AcousticInfiniteParamsCatalogue.hpp"
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

} // namespace

std::optional<AcousticInfiniteParamsCard> AcousticInfiniteParamsFormat::read(FieldReader& reader)
{
    AcousticInfiniteParamsCard card;
    const int last = reader.lastUsedField();
    // The pairs sit side by side from field 1, four to a line. A pair whose two fields are blank is padding.
    for (int field = 1; field <= last && !reader.failed(); field += FIELDS_PER_PAIR)
    {
        if (reader.isBlank(field) && reader.isBlank(field + 1))
            continue;
        AcousticInfiniteParamsParameter parameter;
        const std::string word = std::string(reader.text(field));
        parameter.name = std::string(word);
        if (reader.isBlank(field))
        {
            reader.error("card.missing-field", "a value needs the name of its parameter in the field before it",
                         "parameterName");
            break;
        }
        if (reader.isBlank(field + 1))
        {
            reader.error("card.missing-field", "the parameter " + word + " needs its value in the next field",
                         "parameterValue");
            break;
        }
        if (const auto number = reader.integerOrReal(field + 1, "parameterValue"))
        {
            parameter.value.number = *number;
            card.parameters.push_back(std::move(parameter));
        }
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AcousticInfiniteParamsFormat::check(const AcousticInfiniteParamsCard& card)
{
    for (const AcousticInfiniteParamsParameter& parameter : card.parameters)
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
        const AcousticInfiniteParamsInfo* info = findAcousticInfiniteParam(parameter.name);
        if (info == nullptr)
            continue; // a name the table does not list is kept as it is
        if (const std::string problem = acousticInfiniteParamsValueProblem(*info, parameter.value); !problem.empty())
            return CardProblem{"card.bad-value", problem, "parameterValue"};
    }
    return std::nullopt;
}

void AcousticInfiniteParamsFormat::write(const AcousticInfiniteParamsCard& card, FieldWriter& writer)
{
    int field = 1;
    for (const AcousticInfiniteParamsParameter& parameter : card.parameters)
    {
        writer.text(field, std::string(parameter.name));
        if (parameter.value.isInteger())
            writer.integer(field + 1, std::get<std::int32_t>(parameter.value.number));
        else
            writer.real(field + 1, std::get<double>(parameter.value.number));
        field += FIELDS_PER_PAIR;
    }
}

std::uint32_t AcousticInfiniteParamsTable::append(const AcousticInfiniteParamsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_cards.push(card);
    return row;
}

void AcousticInfiniteParamsTable::finishRead(Model&, const ReadContext& context)
{
    if (m_rows.aliveCount() > 1)
        context.sink.error("deck.duplicate-acoustic-infinite-params",
                           "only one " + std::string(NAME) + " may appear in the Bulk Data; the first one is used");
}

} // namespace oor::model
