#include "oor/model/cards/dynamics/lanczosEigenMethod/LanczosEigenMethodTable.hpp"

#include "oor/base/text/NumberParser.hpp"
#include "oor/base/text/TextUtilities.hpp"

#include <array>
#include <cmath>
#include <string>

namespace oor::model
{

namespace
{

constexpr int FIRST_OPTION_FIELD = 9;
constexpr std::string_view SEGMENT_FREQUENCY_PREFIX = "SegmentFrequency";

double blankIfNone(const std::optional<double>& value)
{
    return value ? *value : std::numeric_limits<double>::quiet_NaN();
}

std::optional<double> noneIfBlank(double value)
{
    if (std::isnan(value))
        return std::nullopt;
    return value;
}

const char* normWord(EigenvectorNorm norm)
{
    switch (norm)
    {
    case EigenvectorNorm::Mass:
        return "Mass";
    case EigenvectorNorm::Max:
        return "Maximum";
    case EigenvectorNorm::Default:
        break;
    }
    return "";
}

// NORM, in the field of the first line or as an option: blank is the default, a word is MASS or MAX.
std::optional<EigenvectorNorm> parseNorm(std::string_view text)
{
    if (text.empty())
        return EigenvectorNorm::Default;
    if ((text == "Mass"))
        return EigenvectorNorm::Mass;
    if ((text == "Maximum"))
        return EigenvectorNorm::Max;
    return std::nullopt;
}

// The options of the lines after the first, NAME=value, one per field. Spaces around the "=" are allowed.
class OptionReader
{
public:
    OptionReader(FieldReader& reader, LanczosEigenMethodCard& card) : m_reader(reader), m_card(card) {}

    void readAll()
    {
        const int last = m_reader.lastUsedField();
        for (int field = FIRST_OPTION_FIELD; field <= last; ++field)
        {
            if (!m_reader.isBlank(field))
                readOne(m_reader.text(field));
        }
        // F1, F2, ... form a list: a frequency cannot be given without the ones before it.
        for (std::size_t index = 0; index < m_frequencies.size(); ++index)
        {
            if (!m_frequencies[index])
            {
                if (hasLaterFrequency(index))
                    m_reader.error("card.bad-value", "F" + std::to_string(index + 1) + " is missing before a later Fi",
                                   "segmentFrequencies");
                break;
            }
            m_card.segmentFrequencies.push_back(*m_frequencies[index]);
        }
    }

private:
    [[nodiscard]] bool hasLaterFrequency(std::size_t index) const
    {
        for (std::size_t later = index + 1; later < m_frequencies.size(); ++later)
        {
            if (m_frequencies[later])
                return true;
        }
        return false;
    }

    void readOne(std::string_view text)
    {
        std::string compact;
        for (const char character : text)
        {
            if (!base::isAsciiSpace(character))
                compact.push_back(character);
        }
        const std::size_t equals = compact.find('=');
        if (equals == std::string::npos || equals == 0 || equals + 1 == compact.size())
        {
            m_reader.error("card.bad-value", "an option is written NAME=value, found \"" + std::string(text) + "\"",
                           "option");
            return;
        }
        const std::string name = compact.substr(0, equals);
        const std::string value = compact.substr(equals + 1);
        if (name == "Alpha")
            setReal(name, value, m_seenAlph, m_card.alpha);
        else if (name == "FrequencySegmentCount")
            setInteger(name, value, m_seenNums, m_card.frequencySegmentCount);
        else if (name.starts_with(SEGMENT_FREQUENCY_PREFIX) &&
                 isNumber(std::string_view(name).substr(SEGMENT_FREQUENCY_PREFIX.size())))
            setFrequency(name, value);
        else if (name == "LowerFrequency")
            setMainReal(name, value, 2, m_card.lowerFrequency);
        else if (name == "UpperFrequency")
            setMainReal(name, value, 3, m_card.upperFrequency);
        else if (name == "ShiftScale")
            setMainReal(name, value, 7, m_card.shiftScale);
        else if (name == "DesiredModeCount")
            setMainInteger(name, value, 4, m_card.desiredModeCount);
        else if (name == "MessageLevel")
            setMainInteger(name, value, 5, m_card.messageLevel);
        else if (name == "MaximumSet")
            setMainInteger(name, value, 6, m_card.maximumSet);
        else if (name == "Normalization")
            setNorm(value);
        else
            m_reader.error("card.bad-value",
                           "unknown option " + name +
                               " (Alpha, FrequencySegmentCount, SegmentFrequency1 to SegmentFrequency15, "
                               "LowerFrequency, UpperFrequency, DesiredModeCount, MessageLevel, MaximumSet, "
                               "ShiftScale, Normalization)",
                           "option");
    }

    static bool isNumber(std::string_view text)
    {
        for (const char character : text)
        {
            if (character < '0' || character > '9')
                return false;
        }
        return !text.empty();
    }

    void setReal(const std::string& name, const std::string& value, bool& seen, double& target)
    {
        const auto parsed = base::NumberParser::parseReal(value);
        if (!parsed)
        {
            m_reader.error("card.bad-real", "option " + name + ": expected a real number, found \"" + value + "\"",
                           "option");
            return;
        }
        if (!claim(name, seen))
            return;
        target = *parsed;
    }

    void setInteger(const std::string& name, const std::string& value, bool& seen, std::int32_t& target)
    {
        const auto parsed = base::NumberParser::parseInteger(value);
        if (!parsed || *parsed > std::numeric_limits<std::int32_t>::max() ||
            *parsed < std::numeric_limits<std::int32_t>::min())
        {
            m_reader.error("card.bad-integer", "option " + name + ": expected an integer, found \"" + value + "\"",
                           "option");
            return;
        }
        if (!claim(name, seen))
            return;
        target = static_cast<std::int32_t>(*parsed);
    }

    void setFrequency(const std::string& name, const std::string& value)
    {
        const std::string digits = name.substr(SEGMENT_FREQUENCY_PREFIX.size());
        const int index = digits.size() > 3 ? 0 : std::stoi(digits);
        if (index < 1 || index > LanczosEigenMethodCard::MAX_SEGMENTS)
        {
            m_reader.error("card.bad-value",
                           "the frequencies are SegmentFrequency1 to SegmentFrequency15, found " + name,
                           "segmentFrequencies");
            return;
        }
        std::optional<double>& target = m_frequencies[static_cast<std::size_t>(index - 1)];
        const auto parsed = base::NumberParser::parseReal(value);
        if (!parsed)
        {
            m_reader.error("card.bad-real", "option " + name + ": expected a real number, found \"" + value + "\"",
                           "segmentFrequencies");
            return;
        }
        if (target)
            m_reader.error("card.bad-value", "option " + name + " is given twice", "segmentFrequencies");
        target = *parsed;
    }

    // An option that stands for a field of the first line: that field must be blank.
    void setMainReal(const std::string& name, const std::string& value, int mainField, std::optional<double>& target)
    {
        const auto parsed = base::NumberParser::parseReal(value);
        if (!parsed)
        {
            m_reader.error("card.bad-real", "option " + name + ": expected a real number, found \"" + value + "\"",
                           "option");
            return;
        }
        if (!mainFieldIsFree(name, mainField))
            return;
        target = *parsed;
    }

    void setMainInteger(const std::string& name, const std::string& value, int mainField, std::int32_t& target)
    {
        const auto parsed = base::NumberParser::parseInteger(value);
        if (!parsed || *parsed > std::numeric_limits<std::int32_t>::max() ||
            *parsed < std::numeric_limits<std::int32_t>::min())
        {
            m_reader.error("card.bad-integer", "option " + name + ": expected an integer, found \"" + value + "\"",
                           "option");
            return;
        }
        if (!mainFieldIsFree(name, mainField))
            return;
        target = static_cast<std::int32_t>(*parsed);
    }

    void setNorm(const std::string& value)
    {
        const auto norm = parseNorm(value);
        if (!norm)
        {
            m_reader.error("card.bad-value", std::string("norm") + " must be Mass or Maximum, found \"" + value + "\"",
                           "norm");
            return;
        }
        if (!mainFieldIsFree("Normalization", 8))
            return;
        m_card.norm = *norm;
    }

    bool mainFieldIsFree(const std::string& name, int mainField)
    {
        if (m_reader.isBlank(mainField) && !m_given[static_cast<std::size_t>(mainField)])
        {
            m_given[static_cast<std::size_t>(mainField)] = true;
            return true;
        }
        m_reader.error("card.bad-value", "option " + name + " repeats a field that is already given", "option");
        return false;
    }

    bool claim(const std::string& name, bool& seen)
    {
        if (seen)
        {
            m_reader.error("card.bad-value", "option " + name + " is given twice", "option");
            return false;
        }
        seen = true;
        return true;
    }

    FieldReader& m_reader;
    LanczosEigenMethodCard& m_card;
    bool m_seenAlph = false;
    bool m_seenNums = false;
    std::array<bool, FIRST_OPTION_FIELD> m_given{};
    std::array<std::optional<double>, LanczosEigenMethodCard::MAX_SEGMENTS> m_frequencies{};
};

} // namespace

std::optional<LanczosEigenMethodCard> LanczosEigenMethodFormat::read(FieldReader& reader)
{
    LanczosEigenMethodCard card;
    card.id = reader.requiredInteger(1, "id");
    card.lowerFrequency = reader.optionalReal(2, "lowerFrequency");
    card.upperFrequency = reader.optionalReal(3, "upperFrequency");
    card.desiredModeCount = reader.integer(4, "desiredModeCount", 0);
    card.messageLevel = reader.integer(5, "messageLevel", 0);
    card.maximumSet = reader.integer(6, "maximumSet", 0);
    card.shiftScale = reader.optionalReal(7, "shiftScale");
    if (const auto norm = parseNorm(reader.text(8)))
        card.norm = *norm;
    else
        reader.error("card.bad-value",
                     std::string("norm") + " must be Mass or Maximum, found \"" + std::string(reader.text(8)) + "\"",
                     "norm");
    OptionReader(reader, card).readAll();
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> LanczosEigenMethodFormat::check(const LanczosEigenMethodCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the method ID must be a positive integer", "id"};
    if (card.lowerFrequency && card.upperFrequency && !(*card.lowerFrequency < *card.upperFrequency))
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must be below " + CardProblem::fieldNamed("upperFrequency"),
            "lowerFrequency"};
    if (card.desiredModeCount < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer or blank",
                           "desiredModeCount"};
    if (card.messageLevel < 0 || card.messageLevel > 4)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 0 to 4", "messageLevel"};
    if (card.maximumSet < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer or blank",
                           "maximumSet"};
    if (!(card.alpha > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero", "alpha"};
    if (card.frequencySegmentCount < 1)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer",
                           "frequencySegmentCount"};
    if (card.segmentFrequencies.size() > static_cast<std::size_t>(LanczosEigenMethodCard::MAX_SEGMENTS))
        return CardProblem{"card.bad-value",
                           "the card has at most 15 segment frequencies (" + std::string(CardProblem::FIELD) + ")",
                           "segmentFrequencies"};
    return std::nullopt;
}

void LanczosEigenMethodFormat::write(const LanczosEigenMethodCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    if (card.lowerFrequency)
        writer.real(2, *card.lowerFrequency);
    if (card.upperFrequency)
        writer.real(3, *card.upperFrequency);
    if (card.desiredModeCount != 0)
        writer.integer(4, card.desiredModeCount);
    writer.integerUnlessDefault(5, card.messageLevel, 0);
    if (card.maximumSet != 0)
        writer.integer(6, card.maximumSet);
    if (card.shiftScale)
        writer.real(7, *card.shiftScale);
    if (card.norm != EigenvectorNorm::Default)
        writer.text(8, normWord(card.norm));
    // The options follow each other from field 9: only the ones that are not defaults are written.
    int field = FIRST_OPTION_FIELD;
    if (!writer.omitsDefaults() || card.alpha != 1.0)
        writer.namedReal(field++, "Alpha", card.alpha);
    if (!writer.omitsDefaults() || card.frequencySegmentCount != 1)
        writer.text(field++, "FrequencySegmentCount=" + std::to_string(card.frequencySegmentCount));
    for (std::size_t index = 0; index < card.segmentFrequencies.size(); ++index)
        writer.namedReal(field++, std::string(SEGMENT_FREQUENCY_PREFIX) + std::to_string(index + 1),
                         card.segmentFrequencies[index]);
}

std::uint32_t LanczosEigenMethodTable::append(const LanczosEigenMethodCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_lowerFrequency.push(blankIfNone(card.lowerFrequency));
    m_upperFrequency.push(blankIfNone(card.upperFrequency));
    m_desiredModeCount.push(card.desiredModeCount);
    m_messageLevel.push(card.messageLevel);
    m_maximumSet.push(card.maximumSet);
    m_shiftScale.push(blankIfNone(card.shiftScale));
    m_norm.push(static_cast<std::uint8_t>(card.norm));
    m_alpha.push(card.alpha);
    m_frequencySegmentCount.push(card.frequencySegmentCount);
    m_segmentFrequencies.push(card.segmentFrequencies);
    return row;
}

LanczosEigenMethodCard LanczosEigenMethodTable::card(std::uint32_t row) const
{
    LanczosEigenMethodCard card;
    card.id = m_id.get(row);
    card.lowerFrequency = noneIfBlank(m_lowerFrequency.get(row));
    card.upperFrequency = noneIfBlank(m_upperFrequency.get(row));
    card.desiredModeCount = m_desiredModeCount.get(row);
    card.messageLevel = m_messageLevel.get(row);
    card.maximumSet = m_maximumSet.get(row);
    card.shiftScale = noneIfBlank(m_shiftScale.get(row));
    card.norm = static_cast<EigenvectorNorm>(m_norm.get(row));
    card.alpha = m_alpha.get(row);
    card.frequencySegmentCount = m_frequencySegmentCount.get(row);
    const auto frequencies = m_segmentFrequencies.get(row);
    card.segmentFrequencies.assign(frequencies.begin(), frequencies.end());
    return card;
}

void LanczosEigenMethodTable::store(std::uint32_t row, const LanczosEigenMethodCard& card)
{
    m_id.set(row, card.id);
    m_lowerFrequency.set(row, blankIfNone(card.lowerFrequency));
    m_upperFrequency.set(row, blankIfNone(card.upperFrequency));
    m_desiredModeCount.set(row, card.desiredModeCount);
    m_messageLevel.set(row, card.messageLevel);
    m_maximumSet.set(row, card.maximumSet);
    m_shiftScale.set(row, blankIfNone(card.shiftScale));
    m_norm.set(row, static_cast<std::uint8_t>(card.norm));
    m_alpha.set(row, card.alpha);
    m_frequencySegmentCount.set(row, card.frequencySegmentCount);
    m_segmentFrequencies.set(row, card.segmentFrequencies);
}

void LanczosEigenMethodTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_lowerFrequency.keepRows(rows);
    m_upperFrequency.keepRows(rows);
    m_desiredModeCount.keepRows(rows);
    m_messageLevel.keepRows(rows);
    m_maximumSet.keepRows(rows);
    m_shiftScale.keepRows(rows);
    m_norm.keepRows(rows);
    m_alpha.keepRows(rows);
    m_frequencySegmentCount.keepRows(rows);
    m_segmentFrequencies.keepRows(rows);
}

} // namespace oor::model
