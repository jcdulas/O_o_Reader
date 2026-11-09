#include "oor/model/control/case_control/CaseValue.hpp"

#include "oor/base/text/NumberFormatter.hpp"
#include "oor/base/text/TextUtilities.hpp"

namespace oor::model
{

CaseValue CaseValue::ofInteger(std::int64_t value)
{
    CaseValue result;
    result.kind = CaseValueKind::Integer;
    result.integer = value;
    return result;
}

CaseValue CaseValue::ofReal(double value)
{
    CaseValue result;
    result.kind = CaseValueKind::Real;
    result.real = value;
    return result;
}

CaseValue CaseValue::ofKeyword(std::string word)
{
    CaseValue result;
    result.kind = CaseValueKind::Keyword;
    result.text = std::move(word);
    return result;
}

CaseValue CaseValue::ofText(std::string text)
{
    CaseValue result;
    result.kind = CaseValueKind::Text;
    result.text = std::move(text);
    return result;
}

CaseValue CaseValue::ofIntegers(std::vector<std::int64_t> values)
{
    CaseValue result;
    result.kind = CaseValueKind::IntegerList;
    result.integers = std::move(values);
    return result;
}

CaseValue CaseValue::ofReals(std::vector<double> values)
{
    CaseValue result;
    result.kind = CaseValueKind::RealList;
    result.reals = std::move(values);
    return result;
}

std::vector<std::string> CaseValue::items() const
{
    std::vector<std::string> result;
    switch (kind)
    {
    case CaseValueKind::None:
        break;
    case CaseValueKind::Integer:
        result.push_back(std::to_string(integer));
        for (const std::int64_t value : integers) // the load sequence of SUPER = i, l
            result.push_back(std::to_string(value));
        break;
    case CaseValueKind::Real:
        result.push_back(base::NumberFormatter::formatRealFree(real));
        break;
    case CaseValueKind::Keyword:
    case CaseValueKind::Text:
        result.push_back(text);
        break;
    case CaseValueKind::IntegerList:
        for (const std::int64_t value : integers)
            result.push_back(std::to_string(value));
        break;
    case CaseValueKind::RealList:
        for (const double value : reals)
            result.push_back(base::NumberFormatter::formatRealFree(value));
        break;
    }
    return result;
}

std::string CaseValue::format() const
{
    if (kind == CaseValueKind::Keyword || kind == CaseValueKind::Text)
        return text;
    std::string joined;
    for (const std::string& item : items())
    {
        if (!joined.empty())
            joined.push_back(',');
        joined += item;
    }
    return joined;
}

} // namespace oor::model
