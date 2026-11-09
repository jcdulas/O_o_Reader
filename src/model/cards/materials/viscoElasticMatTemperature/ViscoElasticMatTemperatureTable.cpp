#include "oor/model/cards/materials/viscoElasticMatTemperature/ViscoElasticMatTemperatureTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <cmath>
#include <string>

namespace oor::model
{

namespace
{

constexpr int LINE = 8; // fields to a line
constexpr int FIELD_WLF_CONSTANT1 = 9;
constexpr int FIELD_WLF_CONSTANT2 = 10;
constexpr int FIRST_SERIES_FIELD = 17;
constexpr int MAX_TERMS = 100000; // a bound on NP, so that a damaged field does not make the reader loop for long

// The first field of the line after the one that holds `field`.
int nextLine(int field)
{
    return ((field - 1 + LINE - 1) / LINE) * LINE + 1;
}

// Reads `count` reals from `first` on (a series goes on across the lines, eight to a line) and notes the fields it
// used.
std::vector<double> readSeries(FieldReader& reader, int first, int count, const char* name, std::vector<int>& used)
{
    std::vector<double> values;
    values.reserve(static_cast<std::size_t>(count));
    for (int index = 0; index < count; ++index)
    {
        values.push_back(reader.requiredReal(first + index, name));
        used.push_back(first + index);
    }
    return values;
}

void writeSeries(FieldWriter& writer, int first, const std::vector<double>& values)
{
    for (std::size_t index = 0; index < values.size(); ++index)
        writer.real(first + static_cast<int>(index), values[index]);
}

bool isFinite(const std::optional<double>& value)
{
    return !value || std::isfinite(*value);
}

bool allFinite(const std::vector<double>& values)
{
    for (const double value : values)
    {
        if (!std::isfinite(value))
            return false;
    }
    return true;
}

} // namespace

const char* ViscoElasticMatTemperatureFormat::shiftWord(ViscoElasticMatTemperatureShift shift) noexcept
{
    switch (shift)
    {
    case ViscoElasticMatTemperatureShift::Wlf:
        return "WilliamsLandelFerry";
    case ViscoElasticMatTemperatureShift::Power:
        return "PowerLaw";
    case ViscoElasticMatTemperatureShift::Nara:
        return "Narayanaswamy";
    case ViscoElasticMatTemperatureShift::User:
        return "User";
    }
    return "WilliamsLandelFerry";
}

std::optional<ViscoElasticMatTemperatureShift> ViscoElasticMatTemperatureFormat::parseShift(
    std::string_view text) noexcept
{
    for (const ViscoElasticMatTemperatureShift shift :
         {ViscoElasticMatTemperatureShift::Wlf, ViscoElasticMatTemperatureShift::Power,
          ViscoElasticMatTemperatureShift::Nara, ViscoElasticMatTemperatureShift::User})
    {
        if ((text == shiftWord(shift)))
            return shift;
    }
    return std::nullopt;
}

std::optional<ViscoElasticMatTemperatureCard> ViscoElasticMatTemperatureFormat::read(FieldReader& reader)
{
    ViscoElasticMatTemperatureCard card;
    card.id = reader.requiredInteger(1, "id");
    if (const std::string_view word = reader.text(2); !word.empty())
    {
        if (const auto shift = parseShift(word))
            card.shift = *shift;
        else
        {
            reader.error(
                "card.bad-value",
                std::string("shiftFunction") + " must be WLF, POWER, NARA or USER, not \"" + std::string(word) + "\"",
                "shiftFunction");
            return card;
        }
    }
    card.shiftReferenceTemperature = reader.real(3, "shiftReferenceTemperature", 0.0);
    card.activationEnergy = reader.optionalReal(4, "activationEnergy");
    card.structuralFraction = reader.optionalReal(5, "structuralFraction");
    card.temperatureDifference = reader.optionalReal(6, "temperatureDifference");
    card.referenceTemperature = reader.optionalReal(7, "referenceTemperature");
    card.termCount = reader.integer(8, "termCount", 0);

    std::vector<int> used = {1, 2, 3, 4, 5, 6, 7, 8};
    if (card.shift != ViscoElasticMatTemperatureShift::User)
    {
        if (card.shift == ViscoElasticMatTemperatureShift::Wlf)
        {
            card.wlfConstant1 = reader.requiredReal(FIELD_WLF_CONSTANT1, "wlfConstant1");
            card.wlfConstant2 = reader.requiredReal(FIELD_WLF_CONSTANT2, "wlfConstant2");
        }
        else
        {
            card.wlfConstant1 = reader.real(FIELD_WLF_CONSTANT1, "wlfConstant1", 0.0);
            card.wlfConstant2 = reader.real(FIELD_WLF_CONSTANT2, "wlfConstant2", 0.0);
        }
        used.push_back(FIELD_WLF_CONSTANT1);
        used.push_back(FIELD_WLF_CONSTANT2);
    }
    else if (!reader.isBlank(FIELD_WLF_CONSTANT1) || !reader.isBlank(FIELD_WLF_CONSTANT2))
    {
        card.wlfConstant1 = reader.real(FIELD_WLF_CONSTANT1, "wlfConstant1", 0.0);
        card.wlfConstant2 = reader.real(FIELD_WLF_CONSTANT2, "wlfConstant2", 0.0);
        used.push_back(FIELD_WLF_CONSTANT1);
        used.push_back(FIELD_WLF_CONSTANT2);
    }
    if (!reader.failed() && card.termCount >= 0 && card.termCount <= MAX_TERMS)
    {
        if (card.shift == ViscoElasticMatTemperatureShift::Power)
            card.powerCoefficients = readSeries(reader, FIRST_SERIES_FIELD, card.termCount, "powerCoefficient", used);
        else if (card.shift == ViscoElasticMatTemperatureShift::Nara)
        {
            const int timesField = nextLine(FIRST_SERIES_FIELD + card.termCount);
            const int ifictField = nextLine(timesField + card.termCount);
            card.naraWeights = readSeries(reader, FIRST_SERIES_FIELD, card.termCount, "naraWeight", used);
            card.naraRelaxationTimes = readSeries(reader, timesField, card.termCount, "naraRelaxationTime", used);
            card.fictiveTemperatureOption = reader.optionalInteger(ifictField, "fictiveTemperatureOption");
            used.push_back(ifictField);
        }
    }
    requireOnlyFields(reader, used);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ViscoElasticMatTemperatureFormat::check(const ViscoElasticMatTemperatureCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (!std::isfinite(card.shiftReferenceTemperature) || !std::isfinite(card.wlfConstant1) ||
        !std::isfinite(card.wlfConstant2) || !isFinite(card.activationEnergy) || !isFinite(card.structuralFraction) ||
        !isFinite(card.temperatureDifference) || !isFinite(card.referenceTemperature) ||
        !allFinite(card.powerCoefficients) || !allFinite(card.naraWeights) || !allFinite(card.naraRelaxationTimes))
        return CardProblem{"card.bad-value", "a temperature, a constant or a coefficient must be a finite number",
                           "shiftReferenceTemperature"};
    if (card.termCount < 0 || card.termCount > MAX_TERMS)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be from 0 to " + std::to_string(MAX_TERMS),
                           "termCount"};
    const bool nara = card.shift == ViscoElasticMatTemperatureShift::Nara;
    if (!nara &&
        (card.activationEnergy || card.structuralFraction || card.temperatureDifference || card.referenceTemperature))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + ", " + CardProblem::fieldNamed("structuralFraction") +
                               ", " + CardProblem::fieldNamed("temperatureDifference") + " and " +
                               CardProblem::fieldNamed("referenceTemperature") + " are for the NARA function only",
                           "activationEnergy"};
    if (nara &&
        !(card.activationEnergy && card.structuralFraction && card.temperatureDifference && card.referenceTemperature))
        return CardProblem{"card.bad-value",
                           "the NARA function needs " + std::string(CardProblem::FIELD) + ", " +
                               CardProblem::fieldNamed("structuralFraction") + ", " +
                               CardProblem::fieldNamed("temperatureDifference") + " and " +
                               CardProblem::fieldNamed("referenceTemperature"),
                           "activationEnergy"};
    if (card.shift == ViscoElasticMatTemperatureShift::Power)
    {
        if (card.termCount < 1 || card.termCount > ViscoElasticMatTemperatureCard::MAX_POWER_TERMS)
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " must be from 1 to 11 for the POWER function",
                               "termCount"};
        if (static_cast<int>(card.powerCoefficients.size()) != card.termCount)
            return CardProblem{"card.bad-value",
                               "the POWER function needs " + std::string(CardProblem::FIELD) + " coefficients " +
                                   CardProblem::fieldNamed("powerCoefficient"),
                               "termCount"};
    }
    else if (!card.powerCoefficients.empty())
        return CardProblem{"card.bad-value",
                           "the coefficients " + std::string(CardProblem::FIELD) + " are for the POWER function only",
                           "powerCoefficient"};
    if (nara)
    {
        if (card.termCount < 1)
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " must be at least 1 for the NARA function",
                               "termCount"};
        if (static_cast<int>(card.naraWeights.size()) != card.termCount ||
            static_cast<int>(card.naraRelaxationTimes.size()) != card.termCount)
            return CardProblem{"card.bad-value",
                               "the NARA function needs " + std::string(CardProblem::FIELD) + " weights " +
                                   CardProblem::fieldNamed("naraWeight") + " and " + std::string(CardProblem::FIELD) +
                                   " relaxation times " + CardProblem::fieldNamed("naraRelaxationTime"),
                               "termCount"};
        if (card.fictiveTemperatureOption && (*card.fictiveTemperatureOption < 1 || *card.fictiveTemperatureOption > 3))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 1, 2 or 3",
                               "fictiveTemperatureOption"};
    }
    else if (!card.naraWeights.empty() || !card.naraRelaxationTimes.empty() || card.fictiveTemperatureOption)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + ", " + CardProblem::fieldNamed("naraRelaxationTime") +
                               " and " + CardProblem::fieldNamed("fictiveTemperatureOption") +
                               " are for the NARA function only",
                           "naraWeight"};
    return std::nullopt;
}

void ViscoElasticMatTemperatureFormat::write(const ViscoElasticMatTemperatureCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    if (card.shift != ViscoElasticMatTemperatureShift::Wlf)
        writer.text(2, shiftWord(card.shift));
    writer.realUnlessDefault(3, card.shiftReferenceTemperature, 0.0);
    if (card.activationEnergy)
        writer.real(4, *card.activationEnergy);
    if (card.structuralFraction)
        writer.real(5, *card.structuralFraction);
    if (card.temperatureDifference)
        writer.real(6, *card.temperatureDifference);
    if (card.referenceTemperature)
        writer.real(7, *card.referenceTemperature);
    writer.integerUnlessDefault(8, card.termCount, 0);
    if (card.shift != ViscoElasticMatTemperatureShift::User)
    {
        writer.real(FIELD_WLF_CONSTANT1, card.wlfConstant1);
        writer.real(FIELD_WLF_CONSTANT2, card.wlfConstant2);
    }
    if (card.shift == ViscoElasticMatTemperatureShift::Power)
        writeSeries(writer, FIRST_SERIES_FIELD, card.powerCoefficients);
    else if (card.shift == ViscoElasticMatTemperatureShift::Nara)
    {
        const int timesField = nextLine(FIRST_SERIES_FIELD + card.termCount);
        writeSeries(writer, FIRST_SERIES_FIELD, card.naraWeights);
        writeSeries(writer, timesField, card.naraRelaxationTimes);
        if (card.fictiveTemperatureOption)
            writer.integer(nextLine(timesField + card.termCount), *card.fictiveTemperatureOption);
    }
}

std::uint32_t ViscoElasticMatTemperatureTable::append(const ViscoElasticMatTemperatureCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ViscoElasticMatTemperatureTable::store(std::uint32_t row, const ViscoElasticMatTemperatureCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ViscoElasticMatTemperatureTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ViscoElasticMatTemperatureTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ViscoElasticMatTemperatureTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the MATVE that the card extends.
    ViscoElasticMatTemperatureCard& card = m_cards.edit(row);
    card.id = visitor.visit("id", ReferenceTarget::Material, card.id);
    m_id.set(row, card.id);
}

} // namespace oor::model
