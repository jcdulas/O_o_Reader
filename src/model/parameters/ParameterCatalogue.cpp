#include "oor/model/parameters/ParameterCatalogue.hpp"

#include "ParameterCatalogueData.hpp"
#include "oor/base/text/NumberParser.hpp"
#include "oor/base/text/TextUtilities.hpp"

#include <algorithm>
#include <cctype>

namespace oor::model
{

namespace
{

// The name without the digits that end it (DofSetPrintSets3 gives DofSetPrintSets).
std::string_view withoutTrailingDigits(std::string_view name) noexcept
{
    while (!name.empty() && std::isdigit(static_cast<unsigned char>(name.back())) != 0)
        name.remove_suffix(1);
    return name;
}

std::string describeValue(const ParameterCard& card)
{
    switch (card.kind)
    {
    case ParameterKind::Blank:
        return "no value";
    case ParameterKind::Integer:
        return "the integer " + std::to_string(card.integerValue);
    case ParameterKind::Real:
        return "a real number";
    case ParameterKind::Text:
        return "the word " + card.textValue;
    case ParameterKind::Complex:
        return "a pair of real numbers";
    }
    return "a value";
}

std::string_view expectedKind(ParamType type) noexcept
{
    switch (type)
    {
    case ParamType::Integer:
        return "an integer";
    case ParamType::Real:
        return "a real number";
    case ParamType::Number:
        return "a number";
    case ParamType::Text:
        return "a word";
    case ParamType::Complex:
        return "a pair of real numbers";
    case ParamType::Any:
        return "any value";
    }
    return "a value";
}

bool kindFits(ParamType type, ParameterKind kind) noexcept
{
    if (kind == ParameterKind::Blank || type == ParamType::Any)
        return true;
    switch (type)
    {
    case ParamType::Integer:
        return kind == ParameterKind::Integer;
    case ParamType::Real:
    case ParamType::Number:
        return kind == ParameterKind::Integer || kind == ParameterKind::Real;
    case ParamType::Text:
        return kind == ParameterKind::Text;
    case ParamType::Complex:
        return kind == ParameterKind::Complex || kind == ParameterKind::Integer || kind == ParameterKind::Real;
    case ParamType::Any:
        return true;
    }
    return false;
}

// True when `value` is one of the options of `info`. An option is compared as an integer for an integer card and as a
// word for a text card; a real value has no options to compare with.
bool allowedValue(const ParamInfo& info, const ParameterCard& card)
{
    const std::vector<std::string_view> options = info.allowedValues();
    if (options.empty())
        return true;
    for (const std::string_view option : options)
    {
        if (card.kind == ParameterKind::Integer)
        {
            const auto parsed = base::NumberParser::parseInteger(option);
            if (parsed && *parsed == card.integerValue)
                return true;
        }
        else if (card.kind == ParameterKind::Text)
        {
            if (option == card.textValue)
                return true;
        }
        else
        {
            return true;
        }
    }
    return false;
}

} // namespace

std::string_view paramTypeName(ParamType type) noexcept
{
    switch (type)
    {
    case ParamType::Integer:
        return "integer";
    case ParamType::Real:
        return "real";
    case ParamType::Number:
        return "number";
    case ParamType::Text:
        return "text";
    case ParamType::Complex:
        return "complex";
    case ParamType::Any:
        return "any";
    }
    return "any";
}

std::vector<std::string_view> ParamInfo::allowedValues() const
{
    std::vector<std::string_view> values;
    std::string_view rest = allowed;
    while (!rest.empty())
    {
        const std::size_t bar = rest.find('|');
        values.push_back(rest.substr(0, bar));
        rest = bar == std::string_view::npos ? std::string_view{} : rest.substr(bar + 1);
    }
    return values;
}

ParameterCatalogue::ParameterCatalogue() : m_entries(parameterCatalogueEntries())
{
    m_byName.resize(m_entries.size());
    for (std::size_t index = 0; index < m_entries.size(); ++index)
    {
        m_byName[index] = index;
        if (m_entries[index].numbered)
            m_numbered.push_back(index);
    }
    std::ranges::sort(
        m_byName, [this](std::size_t left, std::size_t right) { return m_entries[left].name < m_entries[right].name; });
}

const ParameterCatalogue& ParameterCatalogue::instance()
{
    static const ParameterCatalogue CATALOGUE;
    return CATALOGUE;
}

const ParamInfo* ParameterCatalogue::find(std::string_view name) const
{
    const auto found =
        std::ranges::lower_bound(m_byName, name, {}, [this](std::size_t index) { return m_entries[index].name; });
    if (found != m_byName.end() && m_entries[*found].name == name)
        return &m_entries[*found];
    const std::string_view stem = withoutTrailingDigits(name);
    if (stem.size() == name.size() || stem.empty())
        return nullptr;
    for (const std::size_t index : m_numbered)
    {
        if (m_entries[index].name == stem)
            return &m_entries[index];
    }
    return nullptr;
}

ParameterFit ParameterCatalogue::fit(const ParamInfo& info, const ParameterCard& card)
{
    if (!kindFits(info.type, card.kind))
        return ParameterFit::WrongKind;
    return allowedValue(info, card) ? ParameterFit::Fits : ParameterFit::NotAllowed;
}

std::optional<ParamProblem> ParameterCatalogue::problemWith(const ParameterCard& card) const
{
    const ParamInfo* info = find(card.name);
    if (info == nullptr)
        return std::nullopt;
    const std::string label = std::string(card.name);
    const ParameterFit verdict = fit(*info, card);
    switch (verdict)
    {
    case ParameterFit::Fits:
        return std::nullopt;
    case ParameterFit::WrongKind:
        return ParamProblem{
            verdict, label + " takes " + std::string(expectedKind(info->type)) + ", found " + describeValue(card)};
    case ParameterFit::NotAllowed:
    {
        std::string options;
        for (const std::string_view option : info->allowedValues())
            options += (options.empty() ? "" : ", ") + std::string(option);
        return ParamProblem{verdict, label + " takes one of " + options + ", found " + describeValue(card)};
    }
    }
    return std::nullopt;
}

ParameterCard ParameterCatalogue::defaultCard(const ParamInfo& info)
{
    ParameterCard card;
    card.name = std::string(info.name);
    card.kind = info.defaultValue.kind;
    card.integerValue = info.defaultValue.integerValue;
    card.realValue = info.defaultValue.realValue;
    card.imaginaryValue = info.defaultValue.imaginaryValue;
    card.textValue = std::string(info.defaultValue.textValue);
    return card;
}

} // namespace oor::model
