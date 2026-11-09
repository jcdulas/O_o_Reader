#pragma once

#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/FieldWriter.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace oor::model
{

// A field of a design card that holds an integer, a real or a word depending on the response or the relation it is in:
// the attributes of a DRESP1 (an item code, a mode number, a frequency, the word ALL), the first coefficient of a
// DVPREL1 (a real or PVAL), the PTYPE of a DRESP1 (a property name, or the number of a RANDPS). The deck decides which,
// by what the field looks like.
struct DesignValue
{
    enum class Kind : std::uint8_t
    {
        Blank,
        Integer,
        Real,
        Word,
    };

    Kind kind = Kind::Blank;
    std::int32_t integer = 0;
    double real = 0.0;
    std::string word; // upper case

    [[nodiscard]] static DesignValue ofInteger(std::int32_t value)
    {
        DesignValue result;
        result.kind = Kind::Integer;
        result.integer = value;
        return result;
    }
    [[nodiscard]] static DesignValue ofReal(double value)
    {
        DesignValue result;
        result.kind = Kind::Real;
        result.real = value;
        return result;
    }
    [[nodiscard]] static DesignValue ofWord(std::string value);

    [[nodiscard]] bool isBlank() const noexcept { return kind == Kind::Blank; }
    [[nodiscard]] bool isInteger() const noexcept { return kind == Kind::Integer; }
    [[nodiscard]] bool isReal() const noexcept { return kind == Kind::Real; }
    [[nodiscard]] bool isWord() const noexcept { return kind == Kind::Word; }
    [[nodiscard]] bool isWord(const char* expected) const noexcept { return kind == Kind::Word && word == expected; }

    [[nodiscard]] bool operator==(const DesignValue&) const noexcept = default;
};

// Reads a field that may be blank, an integer, a real or a word. A word is kept in upper case and must read back from
// every format: no blank, comma, dollar sign or quote, and at most 8 characters.
[[nodiscard]] DesignValue readDesignValue(FieldReader& reader, int field, const char* name);

// Writes the value in the field; a blank value leaves the field blank.
void writeDesignValue(FieldWriter& writer, int field, const DesignValue& value);

// The value as text for a message ("blank" for none).
[[nodiscard]] std::string describeDesignValue(const DesignValue& value);

// The problem with the value of a card that was not read from a deck (a word that would not read back), if any.
[[nodiscard]] std::string designValueProblem(const DesignValue& value);

// The problem with a label (of a design variable, a response, a topology region), if there is one: 1 to 8 characters,
// no comma, dollar sign, quote or tab, and no blank at either end (it would not read back).
[[nodiscard]] std::optional<CardProblem> checkDesignLabel(const std::string& label, const char* field);

} // namespace oor::model
