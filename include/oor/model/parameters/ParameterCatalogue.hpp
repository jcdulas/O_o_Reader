#pragma once

#include "oor/model/cards/parameters/parameter/ParameterCard.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace oor::model
{

// The kind of value the format reference gives a parameter.
enum class ParamType : std::uint8_t
{
    Integer, // PARAM,POST,-1 (PostprocessingFile)
    Real,    // PARAM,WTMASS,0.00259 (WeightToMass; an integer is accepted where a real is expected, as the card reader
             // does)
    Number,  // an integer or a real: the format reference does not say which (a percentage whose default is written as
             // an integer)
    Text,    // PARAM,AUTOSPC,YES: a word
    Complex, // PARAM,CK1,1.0,0.0: a real and an imaginary part (a lone real is its real part)
    Any // the kind depends on the use, or the format reference says nothing (a parameter replaced by a Case Control
        // command)
};

[[nodiscard]] std::string_view paramTypeName(ParamType type) noexcept;

// The value a parameter has when no PARAM card gives it. `kind` Blank means the format reference gives no default (the
// program computes it, or it is another parameter's value).
struct ParamDefault
{
    ParameterKind kind = ParameterKind::Blank;
    std::int32_t integerValue = 0;
    double realValue = 0.0;
    double imaginaryValue = 0.0;
    std::string_view textValue;

    [[nodiscard]] bool exists() const noexcept { return kind != ParameterKind::Blank; }
};

// One name of the format reference chapter "Parameters". Hand-typed from the chapter (ParameterCatalogueData.cpp): the
// kind, the default and, when the entry lists its options as the complete set, the allowed values.
struct ParamInfo
{
    std::string_view name; // the exact neutral name, such as WeightToMass
    ParamType type = ParamType::Any;
    ParamDefault defaultValue;
    // The values the format reference lists as the options of the parameter, separated by '|' ("Yes|No", "0|1|2");
    // empty when any value of the type is allowed.
    std::string_view allowed;
    // Numbered definitions match their exact neutral base name followed by decimal digits; for example,
    // DofSetPrintSets1 matches the DofSetPrintSets definition.
    bool numbered = false;

    constexpr ParamInfo(std::string_view parameterName, ParamType parameterType, ParamDefault parameterDefault,
                        std::string_view allowedValues = {}, bool numberedName = false) noexcept
        : name(parameterName),
          type(parameterType),
          defaultValue(parameterDefault),
          allowed(allowedValues),
          numbered(numberedName)
    {
    }

    // The allowed values one by one (empty when any value is allowed).
    [[nodiscard]] std::vector<std::string_view> allowedValues() const;
};

// How a PARAM card's value stands against the catalogue.
enum class ParameterFit : std::uint8_t
{
    Fits,
    WrongKind, // a word where a number is expected, a real where an integer is, ...
    NotAllowed // the kind is right but the value is not one of the options the format reference lists
};

// What is wrong with the value of a PARAM card, in a sentence.
struct ParamProblem
{
    ParameterFit fit = ParameterFit::Fits;
    std::string message;
};

// The names of the format reference parameters and what is known of each. A name the catalogue does not know is a valid
// PARAM all the same (the format reference lists the parameters of the format, and a deck may carry those of a solver
// or a pre-processor).
class ParameterCatalogue
{
public:
    [[nodiscard]] static const ParameterCatalogue& instance();

    // The entries in the order of the format reference (a heading that lists several names gives one entry for each).
    [[nodiscard]] std::span<const ParamInfo> entries() const noexcept { return m_entries; }

    // The entry of a neutral PARAM name (exact: "WeightToMass"; DofSetPrintSets3 finds the numbered DofSetPrintSets),
    // nullptr when the catalogue has no definition with this exact neutral name.
    [[nodiscard]] const ParamInfo* find(std::string_view name) const;

    // Does the value of `card` suit the entry? A blank card (no value) always fits: the reader does not know what the
    // solver does with it.
    [[nodiscard]] static ParameterFit fit(const ParamInfo& info, const ParameterCard& card);

    // A problem naming the neutral parameter and the expected value; nullopt when it fits or is not catalogued.
    [[nodiscard]] std::optional<ParamProblem> problemWith(const ParameterCard& card) const;

    // The default of the entry as a card (kind Blank when there is none), named `info.name`.
    [[nodiscard]] static ParameterCard defaultCard(const ParamInfo& info);

private:
    ParameterCatalogue();

    std::vector<ParamInfo> m_entries;
    std::vector<std::size_t> m_byName;   // indexes of the entries, sorted by name
    std::vector<std::size_t> m_numbered; // indexes of the entries whose name is followed by digits
};

} // namespace oor::model
