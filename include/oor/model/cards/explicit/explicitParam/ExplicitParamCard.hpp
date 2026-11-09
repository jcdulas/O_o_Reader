#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace oor::model
{

// What one field of a DYPARAM card holds. The kind is decided by the text, as for a PARAM: a whole number is an
// integer, a number with a decimal point or an exponent is a real, anything else is a word.
enum class ExplicitParamValueKind : std::uint8_t
{
    Blank,
    Integer,
    Real,
    Text
};

// One field after the name of a DYPARAM card.
struct ExplicitParamValue
{
    ExplicitParamValueKind kind = ExplicitParamValueKind::Blank;
    std::int32_t integerValue = 0;
    double realValue = 0.0;
    std::string textValue; // upper case: the words of the format do not depend on the case

    [[nodiscard]] static ExplicitParamValue blank() { return {}; }
    [[nodiscard]] static ExplicitParamValue integer(std::int32_t value)
    {
        ExplicitParamValue result;
        result.kind = ExplicitParamValueKind::Integer;
        result.integerValue = value;
        return result;
    }
    [[nodiscard]] static ExplicitParamValue real(double value)
    {
        ExplicitParamValue result;
        result.kind = ExplicitParamValueKind::Real;
        result.realValue = value;
        return result;
    }
    [[nodiscard]] static ExplicitParamValue text(std::string value)
    {
        ExplicitParamValue result;
        result.kind = ExplicitParamValueKind::Text;
        result.textValue = std::move(value);
        return result;
    }

    [[nodiscard]] bool isBlank() const noexcept { return kind == ExplicitParamValueKind::Blank; }
    [[nodiscard]] bool isNumber() const noexcept
    {
        return kind == ExplicitParamValueKind::Integer || kind == ExplicitParamValueKind::Real;
    }
    // The number of an integer or a real (0 for a word or a blank).
    [[nodiscard]] double number() const noexcept
    {
        if (kind == ExplicitParamValueKind::Integer)
            return static_cast<double>(integerValue);
        return kind == ExplicitParamValueKind::Real ? realValue : 0.0;
    }

    [[nodiscard]] bool operator==(const ExplicitParamValue&) const = default;
};

// One DYPARAM card (SOL 700): a named parameter of the explicit solver. It is the entry of the Bulk Data that sets what
// the format reference documents as 80 separate entries (DYPARAM,ATBAOUT; DYPARAM,CONTACT; DYPARAM,SMP,BATCHSIZ ...),
// each with its own values. The card keeps the fields as they were given and ExplicitParamCatalogue knows what each
// name takes.
//
// Card layout: DYPARAM  NAME  VALUE1  VALUE2  ...      field 1 is the name, the values follow in fields 2, 3, ...
// A name the catalogue lists must have the values of its entry (kinds, words, ranges, the number of values); a name it
// does not list is kept as written, whatever it holds. A blank field between two values is kept as a blank value (the
// value there is the default of the entry); blank fields after the last value are not kept.
struct ExplicitParamCard
{
    std::string entryName;
    std::vector<ExplicitParamValue> values; // fields 2 to the last used one

    [[nodiscard]] bool operator==(const ExplicitParamCard&) const = default;
};

} // namespace oor::model
