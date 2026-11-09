#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/base/text/NumberParser.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/solution_control/FatigueLines.hpp"
#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/FieldWriter.hpp"

#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

// What the two cards of the HDF5 output parameters (HDF5OUT and H5OUT) share: the list of NAME and value pairs, the
// value as the deck wrote it, and the table of the parameters that the format
// lists for HDF5OUT. Each card still says where its first pair is, and has its own fields apart from the pairs.
namespace oor::model
{

inline constexpr std::string_view HDF5_PARAMETER_OWNER = "ResultFileParams";

// The value of a parameter: an integer (32), a real, or a word (Lz4Compression).
struct Hdf5Value
{
    std::variant<std::int32_t, double, std::string> data = std::int32_t{0};

    [[nodiscard]] bool isInteger() const noexcept { return std::holds_alternative<std::int32_t>(data); }
    [[nodiscard]] bool isReal() const noexcept { return std::holds_alternative<double>(data); }
    [[nodiscard]] bool isWord() const noexcept { return std::holds_alternative<std::string>(data); }

    [[nodiscard]] bool operator==(const Hdf5Value&) const noexcept = default;
};

// A parameter and its value.
struct Hdf5Parameter
{
    std::string name;
    Hdf5Value value;

    [[nodiscard]] bool operator==(const Hdf5Parameter&) const noexcept = default;
};

// What kind of value an entry of the table takes: one of a few integers, an integer in a range, or one of a few words.
enum class Hdf5ParameterKind : std::uint8_t
{
    Integer,      // one of `numbers`
    IntegerRange, // from `minimum` to `maximum`
    Word          // one of `words`
};

// One HDF5 output parameter: its name, the values it takes, the default the solver uses when no card gives it, and a
// few words on what it does. `note` is written for the project.
struct Hdf5ParameterInfo
{
    std::string_view name; // the neutral name
    Hdf5ParameterKind kind;
    std::array<std::string_view, 4> words;
    std::array<int, 4> numbers;
    int count; // how many of `words` or `numbers` are used
    int minimum;
    int maximum;
    std::string_view defaultValue; // empty when the format gives none
    std::string_view values;       // the values as a phrase, for messages
    std::string_view note;
};

// The eleven parameters of the table, in the order of the format (typed by hand from it).
inline constexpr std::array<Hdf5ParameterInfo, 11> HDF5_PARAMETER_CATALOGUE{{
    {"Precision",
     Hdf5ParameterKind::Integer,
     {},
     {32, 64},
     2,
     0,
     0,
     "32",
     "32 or 64",
     "the precision of the file: 32 bit or 64 bit"},
    {"CompressionMethod",
     Hdf5ParameterKind::Word,
     {"None", "BloscCompression", "Lz4Compression", "GzipCompression"},
     {},
     4,
     0,
     0,
     "Lz4Compression",
     "None, BloscCompression, Lz4Compression or GzipCompression",
     "the compression method"},
    {"CompressionLevel",
     Hdf5ParameterKind::IntegerRange,
     {},
     {},
     0,
     1,
     9,
     "",
     "an integer from 1 to 9",
     "the compression level (the GZIP method fixes it at 3)"},
    {"InputData",
     Hdf5ParameterKind::Word,
     {"Yes", "No"},
     {},
     2,
     0,
     0,
     "Yes",
     "Yes or No",
     "whether the input data sets and characters are written"},
    {"RotationalDofs",
     Hdf5ParameterKind::Word,
     {"Yes", "No"},
     {},
     2,
     0,
     0,
     "Yes",
     "Yes or No",
     "whether the rotational components of the vector data sets are written"},
    {"MatrixData",
     Hdf5ParameterKind::Word,
     {"Yes", "No"},
     {},
     2,
     0,
     0,
     "No",
     "Yes or No",
     "whether the matrix data are written"},
    {"SeparateModelFile",
     Hdf5ParameterKind::Integer,
     {},
     {-1, 0, 1, 2},
     4,
     0,
     0,
     "0",
     "-1, 0, 1 or 2",
     "whether the model input data go to a file of their own (2 writes only them; -1 follows the PARAM that writes "
     "the GEOM3 and GEOM4 tables to the binary result file)"},
    {"NodeElementTables",
     Hdf5ParameterKind::Integer,
     {},
     {0, 1},
     2,
     0,
     0,
     "1",
     "0 or 1",
     "whether the GEOM1 and GEOM2 data blocks are written"},
    {"LoadConstraintTables",
     Hdf5ParameterKind::Integer,
     {},
     {-1, 0, 1},
     3,
     0,
     0,
     "-1",
     "-1, 0 or 1",
     "whether the GEOM3 and GEOM4 data blocks are written (-1 follows the PARAM that writes them to the binary result "
     "file)"},
    {"RunInformation",
     Hdf5ParameterKind::Word,
     {"Yes", "No"},
     {},
     2,
     0,
     0,
     "Yes",
     "Yes or No",
     "whether the information of the job run is written"},
    {"ModalMatrixOutput",
     Hdf5ParameterKind::Word,
     {"Yes", "No"},
     {},
     2,
     0,
     0,
     "No",
     "Yes or No",
     "whether the BHH and MHH matrices are written"},
}};

// The entry of a neutral name; null for a name the table does not list (a card may use such a name).
[[nodiscard]] inline const Hdf5ParameterInfo* findHdf5Parameter(std::string_view name) noexcept
{
    for (const Hdf5ParameterInfo& info : HDF5_PARAMETER_CATALOGUE)
    {
        if (info.name == name)
            return &info;
    }
    return nullptr;
}

// Why a value does not suit a parameter: empty when it does. The message names the parameter by its keyword.
[[nodiscard]] inline std::string hdf5ValueProblem(const Hdf5ParameterInfo& info, const Hdf5Value& value)
{
    const std::string what = std::string(info.name) + " must be " + std::string(info.values);
    switch (info.kind)
    {
    case Hdf5ParameterKind::Integer:
    case Hdf5ParameterKind::IntegerRange:
    {
        if (!value.isInteger())
            return what + " (an integer was expected)";
        const int number = std::get<std::int32_t>(value.data);
        if (info.kind == Hdf5ParameterKind::IntegerRange)
            return number >= info.minimum && number <= info.maximum ? std::string() : what;
        for (int index = 0; index < info.count; ++index)
        {
            if (info.numbers[static_cast<std::size_t>(index)] == number)
                return {};
        }
        return what;
    }
    case Hdf5ParameterKind::Word:
    {
        if (!value.isWord())
            return what + " (a word was expected)";
        for (int index = 0; index < info.count; ++index)
        {
            if ((info.words[static_cast<std::size_t>(index)] == std::get<std::string>(value.data)))
                return {};
        }
        return what;
    }
    }
    return {};
}

// Reads the pairs from field `firstField` on, side by side over as many lines as the card has. A pair whose two fields
// are blank is padding; a name with no value and a value with no name are errors. A name takes its neutral name.
[[nodiscard]] inline std::vector<Hdf5Parameter> readHdf5Parameters(FieldReader& reader, int firstField)
{
    std::vector<Hdf5Parameter> parameters;
    const int last = reader.lastUsedField();
    for (int field = firstField; field <= last && !reader.failed(); field += 2)
    {
        if (reader.isBlank(field) && reader.isBlank(field + 1))
            continue;
        if (reader.isBlank(field))
        {
            reader.error("card.missing-field", "a value needs the name of its parameter in the field before it",
                         "parameterName");
            break;
        }
        if (reader.isBlank(field + 1))
        {
            reader.error("card.missing-field",
                         "the parameter " + std::string(reader.text(field)) + " needs its value in the next field",
                         "parameterValue");
            break;
        }
        Hdf5Parameter parameter;
        const std::string word(reader.text(field));
        parameter.name = std::string(word);
        const std::string_view text = reader.text(field + 1);
        if (const auto integer = base::NumberParser::parseInteger(text))
        {
            if (const auto id = base::toEntityId(*integer))
                parameter.value.data = static_cast<std::int32_t>(*id);
            else
                reader.error("card.integer-too-large", "the value of " + word + " does not fit in 32 bits",
                             "parameterValue");
        }
        else if (const auto real = base::NumberParser::parseReal(text))
        {
            parameter.value.data = *real;
        }
        else
        {
            parameter.value.data = std::string(text); // a neutral name (Lz4Compression, Yes)
        }
        parameters.push_back(std::move(parameter));
    }
    return parameters;
}

// Writes the pairs from field `firstField` on, side by side, each name as its neutral name in a model file and as its
// keyword in a deck.
inline void writeHdf5Parameters(FieldWriter& writer, int firstField, const std::vector<Hdf5Parameter>& parameters)
{
    int field = firstField;
    for (const Hdf5Parameter& parameter : parameters)
    {
        writer.text(field, std::string(parameter.name));
        if (parameter.value.isInteger())
            writer.integer(field + 1, std::get<std::int32_t>(parameter.value.data));
        else if (parameter.value.isReal())
            writer.real(field + 1, std::get<double>(parameter.value.data));
        else
            writer.text(field + 1, std::get<std::string>(parameter.value.data));
        field += 2;
    }
}

// The problem with a list of pairs, if there is one: a name that is not a name of up to eight characters that starts
// with a letter, a value that is a real that is not finite or a word that would not read back, and a value that does
// not suit the entry of its name in the table (a name the table does not list is kept with its value as written).
[[nodiscard]] inline std::optional<CardProblem> checkHdf5Parameters(const std::vector<Hdf5Parameter>& parameters)
{
    for (const Hdf5Parameter& parameter : parameters)
    {
        // A name is checked as the deck writes it: the keyword of a neutral name, or the word as it was read.
        const std::string label = std::string(parameter.name);
        if (const auto problem = checkNameWord(label, "parameterName"))
            return problem;
        if (std::isalpha(static_cast<unsigned char>(label.front())) == 0)
            return CardProblem{"card.bad-value", "the parameter name " + label + " must start with a letter",
                               "parameterName"};
        if (parameter.value.isReal() && !std::isfinite(std::get<double>(parameter.value.data)))
            return CardProblem{"card.bad-value", "the value of " + label + " must be a finite number",
                               "parameterValue"};
        if (parameter.value.isWord())
        {
            const std::string& word = std::get<std::string>(parameter.value.data);
            if (const auto problem = checkSpannedText(word, "parameterValue"))
                return problem;
        }
        if (const Hdf5ParameterInfo* info = findHdf5Parameter(parameter.name))
        {
            if (const std::string problem = hdf5ValueProblem(*info, parameter.value); !problem.empty())
                return CardProblem{"card.bad-value", problem, "parameterValue"};
        }
    }
    return std::nullopt;
}

} // namespace oor::model
