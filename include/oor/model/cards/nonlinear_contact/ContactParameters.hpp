#pragma once

#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/FieldWriter.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace oor::model
{

// What the format reference says a parameter holds.
enum class ContactParameterKind : std::uint8_t
{
    Integer, // an integer (a flag, a count, a table ID)
    Real,    // a real (an integer is read as the real of the same value)
    Number,  // a real or an integer: a value, or the ID of a table of values
    Text,    // a word
    Any      // an integer, a real or a word (BCAUTOP FTYPE: BLCOUL or 6)
};

struct ContactParameterRule
{
    std::string_view name; // the neutral name
    ContactParameterKind kind;
};

// The value of a parameter as the card has it: blank (the default), an integer, a real or a word.
using ContactValue = std::variant<std::monostate, std::int64_t, double, std::string>;

struct ContactParameter
{
    std::string name;
    ContactValue value;

    [[nodiscard]] bool operator==(const ContactParameter&) const = default;
};

// A field that holds a real or, as the format reference often has it, a table ID when it is an integer: blank, an
// integer or a real as written. Anything else is an error on `name`.
[[nodiscard]] ContactValue readContactValue(FieldReader& reader, int field, const char* name);
void writeContactValue(FieldWriter& writer, int field, const ContactValue& value);
// The card types whose keywords the parameters of a card take, in the order they are looked up: the card type itself,
// then the card types whose parameters it also accepts (a BCAUTOP takes those of BCPARA, BCONPRP, BCONPRG and BCBDPRP).
using ParameterOwners = std::span<const std::string_view>;

// A field read as the kind `rule` says (what readContactParameters does for each pair): blank, an integer, a real or a
// word, with an error on the field when the text does not fit the kind. `owner` is the card type (for the message).
[[nodiscard]] ContactValue readContactParameterValue(FieldReader& reader, int field, const ContactParameterRule& rule,
                                                     std::string_view owner);
[[nodiscard]] ContactValue readContactParameterValue(FieldReader& reader, int field, const ContactParameterRule& rule,
                                                     ParameterOwners owners);
// Whether the value is blank or a finite real or an integer (what readContactValue gives).
[[nodiscard]] bool isContactNumber(const ContactValue& value) noexcept;

// Reads the pairs of a card whose first pair is at field `firstPairField` (3 for BCONPRG, 2 for BCPARA). The first line
// has the pairs that fit before field 8; the pairs after it start at field 9 and take two fields each. A name that is
// not in `rules` is an error, and so is a value of the wrong kind. The names are keywords of the card types `owners`.
[[nodiscard]] std::vector<ContactParameter> readContactParameters(FieldReader& reader, int firstPairField,
                                                                  std::span<const ContactParameterRule> rules,
                                                                  ParameterOwners owners);
[[nodiscard]] std::vector<ContactParameter> readContactParameters(FieldReader& reader, int firstPairField,
                                                                  std::span<const ContactParameterRule> rules,
                                                                  std::string_view owner);

// Writes the pairs as readContactParameters finds them, each name as its keyword.
void writeContactParameters(FieldWriter& writer, int firstPairField, const std::vector<ContactParameter>& parameters,
                            ParameterOwners owners);
void writeContactParameters(FieldWriter& writer, int firstPairField, const std::vector<ContactParameter>& parameters,
                            std::string_view owner);

// The first problem of a list of parameters: a name that is not in `rules`, a value of the wrong kind. A parameter
// given twice is kept as written (the format reference says nothing against it). The messages show the keywords.
[[nodiscard]] std::optional<CardProblem> checkContactParameters(const std::vector<ContactParameter>& parameters,
                                                                std::span<const ContactParameterRule> rules,
                                                                ParameterOwners owners);
[[nodiscard]] std::optional<CardProblem> checkContactParameters(const std::vector<ContactParameter>& parameters,
                                                                std::span<const ContactParameterRule> rules,
                                                                std::string_view owner);

// The rule of a neutral parameter name, or nullptr.
[[nodiscard]] const ContactParameterRule* findContactParameterRule(std::span<const ContactParameterRule> rules,
                                                                   std::string_view name);

} // namespace oor::model
