#include "oor/model/cards/materials/elastoPlasticMat/ElastoPlasticMatTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <cmath>
#include <limits>
#include <string>

namespace oor::model
{

namespace
{

constexpr std::uint8_t RATE_DEPENDENCE_BIT = 1;
constexpr std::uint8_t ANISOTROPY_BIT = 2;
constexpr std::uint8_t PRESSURE_DEPENDENCE_BIT = 4;
constexpr std::uint8_t CHABOCHE_HARDENING_BIT = 8;
constexpr std::uint8_t JOHNSON_COOK_HARDENING_BIT = 16;
constexpr std::uint8_t IMPLICIT_CREEP_BIT = 32;

constexpr int FIRST_GROUP_FIELD = 9;
constexpr int LINE = 8; // fields to a line

constexpr const char* ANISOTROPY_NAMES[6] = {"anisotropyRatio11", "anisotropyRatio22", "anisotropyRatio33",
                                             "anisotropyRatio12", "anisotropyRatio23", "anisotropyRatio31"};
constexpr const char* CHABOCHE_NAMES[10] = {"chabocheYieldStress",        "saturatedIsotropicHardening",
                                            "hardeningCoefficient",       "hardeningOrRateCoefficient",
                                            "kinematicRecallRate",        "viscousCoefficient",
                                            "hardeningOrViscousExponent", "memoryHardeningLimit",
                                            "memoryHardeningCoefficient", "memoryEvolutionFactor"};
constexpr const char* JOHNSON_COOK_NAMES[8] = {"johnsonCookYieldStress",
                                               "hardeningCoefficient",
                                               "hardeningOrViscousExponent",
                                               "hardeningOrRateCoefficient",
                                               "thermalSofteningExponent",
                                               "meltTemperature",
                                               "roomTemperature",
                                               "referenceStrainRate"};

double notGiven()
{
    return std::numeric_limits<double>::quiet_NaN();
}

std::optional<double> optionalOf(double value)
{
    return std::isnan(value) ? std::nullopt : std::optional<double>(value);
}

bool is(std::string_view text, std::string_view word)
{
    return (text == word);
}

// True when the line that starts at `first` has any field that is not blank.
bool lineHasValues(const FieldReader& reader, int first)
{
    for (int field = first; field < first + LINE; ++field)
    {
        if (!reader.isBlank(field))
            return true;
    }
    return false;
}

// Reports an error when a field of `first` to `last` is not blank (the cells the format reference leaves empty in a
// group).
void requireBlankRange(FieldReader& reader, int first, int last, const char* group)
{
    for (int field = first; field <= last; ++field)
    {
        if (!reader.isBlank(field))
        {
            reader.error("card.bad-value",
                         "field " + std::to_string(field) + " of the " + std::string(group) + " group of a " +
                             std::string(reader.cardName()) + " is not used and must be blank",
                         group);
            return;
        }
    }
}

// The message of a group that a card gives twice.
std::string groupTwice(const FieldReader& reader, const char* group)
{
    return "the " + std::string(group) + " group of a " + std::string(reader.cardName()) + " is there twice";
}

// The keywords that start the group lines, as a message shows them.
std::string groupKeywords()
{
    return fieldNameList({"rateDependence", "anisotropy", "pressureDependence", "chabocheHardening",
                          "johnsonCookHardening", "implicitCreep"});
}

bool isNumber(double value)
{
    return std::isfinite(value);
}

} // namespace

const char* ElastoPlasticMatFormat::formWord(ElastoPlasticMatForm form) noexcept
{
    switch (form)
    {
    case ElastoPlasticMatForm::Slope:
        return "Slope";
    case ElastoPlasticMatForm::Table:
        return "Table";
    case ElastoPlasticMatForm::Perfect:
        return "PerfectPlasticity";
    }
    return "Slope";
}

std::optional<ElastoPlasticMatForm> ElastoPlasticMatFormat::parseForm(std::string_view text) noexcept
{
    for (const ElastoPlasticMatForm form :
         {ElastoPlasticMatForm::Slope, ElastoPlasticMatForm::Table, ElastoPlasticMatForm::Perfect})
    {
        if (is(text, formWord(form)))
            return form;
    }
    return std::nullopt;
}

const char* ElastoPlasticMatFormat::yieldWord(ElastoPlasticMatYield yield) noexcept
{
    switch (yield)
    {
    case ElastoPlasticMatYield::VonMises:
        return "VonMises";
    case ElastoPlasticMatYield::Hill:
        return "Hill";
    case ElastoPlasticMatYield::Barlat:
        return "BarlatYield";
    case ElastoPlasticMatYield::LinMohr:
        return "LinearMohrCoulomb";
    case ElastoPlasticMatYield::PblMohr:
        return "ParabolicMohrCoulomb";
    case ElastoPlasticMatYield::ImpCreep:
        return "ImplicitCreep";
    }
    return "VonMises";
}

std::optional<ElastoPlasticMatYield> ElastoPlasticMatFormat::parseYield(std::string_view text) noexcept
{
    for (const ElastoPlasticMatYield yield :
         {ElastoPlasticMatYield::VonMises, ElastoPlasticMatYield::Hill, ElastoPlasticMatYield::Barlat,
          ElastoPlasticMatYield::LinMohr, ElastoPlasticMatYield::PblMohr, ElastoPlasticMatYield::ImpCreep})
    {
        if (is(text, yieldWord(yield)))
            return yield;
    }
    return std::nullopt;
}

const char* ElastoPlasticMatFormat::hardeningWord(ElastoPlasticMatHardening hardening) noexcept
{
    switch (hardening)
    {
    case ElastoPlasticMatHardening::Isotropic:
        return "IsotropicHardening";
    case ElastoPlasticMatHardening::Kinematic:
        return "KinematicHardening";
    case ElastoPlasticMatHardening::Combined:
        return "CombinedHardening";
    case ElastoPlasticMatHardening::Chaboche:
        return "ChabocheHardening";
    case ElastoPlasticMatHardening::JohnsonCook:
        return "JohnsonCookHardening";
    }
    return "IsotropicHardening";
}

std::optional<ElastoPlasticMatHardening> ElastoPlasticMatFormat::parseHardening(std::string_view text) noexcept
{
    for (const ElastoPlasticMatHardening hardening :
         {ElastoPlasticMatHardening::Isotropic, ElastoPlasticMatHardening::Kinematic,
          ElastoPlasticMatHardening::Combined, ElastoPlasticMatHardening::Chaboche,
          ElastoPlasticMatHardening::JohnsonCook})
    {
        if (is(text, hardeningWord(hardening)))
            return hardening;
    }
    return std::nullopt;
}

const char* ElastoPlasticMatFormat::rateWord(ElastoPlasticMatRateOption option) noexcept
{
    return option == ElastoPlasticMatRateOption::Cowper ? "CowperSymonds" : "Table";
}

std::optional<ElastoPlasticMatRateOption> ElastoPlasticMatFormat::parseRate(std::string_view text) noexcept
{
    if (is(text, "Table"))
        return ElastoPlasticMatRateOption::Table;
    if (is(text, "CowperSymonds"))
        return ElastoPlasticMatRateOption::Cowper;
    return std::nullopt;
}

const char* ElastoPlasticMatFormat::pressureWord(ElastoPlasticMatPressureOption option) noexcept
{
    return option == ElastoPlasticMatPressureOption::Parabolic ? "Parabolic" : "Linear";
}

std::optional<ElastoPlasticMatPressureOption> ElastoPlasticMatFormat::parsePressure(std::string_view text) noexcept
{
    if (is(text, "Linear"))
        return ElastoPlasticMatPressureOption::Linear;
    if (is(text, "Parabolic"))
        return ElastoPlasticMatPressureOption::Parabolic;
    return std::nullopt;
}

ElastoPlasticMatTable::ElastoPlasticMatTable()
    : m_yieldStress(notGiven()),
      m_cowperCoefficient(1.0),
      m_cowperExponent(1.0),
      m_anisotropy(std::array<double, 6>{1.0, 1.0, 1.0, 1.0, 1.0, 1.0}),
      m_mohrCoulombAlpha(notGiven()),
      m_mohrCoulombBeta(notGiven()),
      m_creepEquivalentStress(notGiven())
{
}

std::optional<ElastoPlasticMatCard> ElastoPlasticMatFormat::read(FieldReader& reader)
{
    ElastoPlasticMatCard card;
    card.id = reader.requiredInteger(1, "id");
    if (const std::string_view word = reader.text(2); !word.empty())
    {
        if (const auto form = parseForm(word))
            card.form = *form;
        else
            reader.error("card.bad-value",
                         std::string("form") + " must be SLOPE, TABLE or PERFECT, found \"" + std::string(word) + "\"",
                         "form");
    }
    card.yieldStress = reader.optionalReal(3, "yieldStress");
    card.stressStrainTableId = reader.integer(4, "stressStrainTableId", 0);
    if (const std::string_view word = reader.text(5); !word.empty())
    {
        if (const auto yield = parseYield(word))
            card.yieldCriterion = *yield;
        else
            reader.error("card.bad-value",
                         std::string("yieldCriterion") + " is not a yield criterion of the format reference: \"" +
                             std::string(word) + "\"",
                         "yieldCriterion");
    }
    if (const std::string_view word = reader.text(6); !word.empty())
    {
        if (const auto hardening = parseHardening(word))
            card.workHardening = *hardening;
        else
            reader.error("card.bad-value",
                         std::string("workHardening") + " is not a hardening rule of the format reference: \"" +
                             std::string(word) + "\"",
                         "workHardening");
    }
    if (!reader.isBlank(7))
        reader.error("card.bad-value",
                     "field 7 of a " + std::string(reader.cardName()) + " is not used and must be blank",
                     "workHardening");
    card.hardeningSlope = reader.real(8, "hardeningSlope", 0.0);

    // The groups: each starts a line with its keyword (a blank line is skipped).
    const int last = reader.lastUsedField();
    int position = FIRST_GROUP_FIELD;
    while (position <= last && !reader.failed())
    {
        const std::string_view keyword = reader.text(position);
        if (keyword.empty())
        {
            if (lineHasValues(reader, position))
                reader.error("card.bad-value",
                             "a line of a " + std::string(reader.cardName()) +
                                 " after the first starts with a keyword: " + groupKeywords(),
                             "");
            position += LINE;
            continue;
        }
        if (is(keyword, "RateEffect"))
        {
            if (card.rateDependence)
                reader.error("card.bad-value", groupTwice(reader, "rateDependence"), "rateDependence");
            ElastoPlasticMatRateGroup group;
            if (const std::string_view word = reader.text(position + 1); !word.empty())
            {
                if (const auto option = parseRate(word))
                    group.option = *option;
                else
                    reader.error("card.bad-value",
                                 "the " + std::string("groupOption") + " of " + std::string("rateDependence") +
                                     " must be TABLE or COWPER",
                                 "groupOption");
            }
            group.strainRateTableId = reader.integer(position + 2, "strainRateTableId", 0);
            group.cowperCoefficient = reader.real(position + 3, "hardeningOrRateCoefficient", 1.0);
            group.cowperExponent = reader.real(position + 4, "cowperExponent", 1.0);
            requireBlankRange(reader, position + 5, position + 7, "rateDependence");
            card.rateDependence = group;
            position += LINE;
        }
        else if (is(keyword, "AnisotropicYield"))
        {
            if (card.anisotropy)
                reader.error("card.bad-value", groupTwice(reader, "anisotropy"), "anisotropy");
            requireBlankRange(reader, position + 1, position + 1, "anisotropy");
            std::array<double, 6> values{};
            for (std::size_t index = 0; index < 6; ++index)
                values[index] = reader.real(position + 2 + static_cast<int>(index), ANISOTROPY_NAMES[index], 1.0);
            card.anisotropy = values;
            position += LINE;
        }
        else if (is(keyword, "PressureDependence"))
        {
            if (card.pressureDependence)
                reader.error("card.bad-value", groupTwice(reader, "pressureDependence"), "pressureDependence");
            ElastoPlasticMatPressureGroup group;
            if (const std::string_view word = reader.text(position + 1); !word.empty())
            {
                if (const auto option = parsePressure(word))
                    group.option = *option;
                else
                    reader.error("card.bad-value",
                                 "the " + std::string("groupOption") + " of " + std::string("pressureDependence") +
                                     " must be LIN or PBL",
                                 "groupOption");
            }
            group.mohrCoulombAlpha = reader.optionalReal(position + 2, "mohrCoulombAlpha");
            group.mohrCoulombBeta = reader.optionalReal(position + 3, "mohrCoulombBeta");
            requireBlankRange(reader, position + 4, position + 7, "pressureDependence");
            card.pressureDependence = group;
            position += LINE;
        }
        else if (is(keyword, "ChabocheHardening"))
        {
            if (card.chabocheHardening)
                reader.error("card.bad-value", groupTwice(reader, "chabocheHardening"), "chabocheHardening");
            std::array<double, 10> values{};
            for (std::size_t index = 0; index < 7; ++index)
                values[index] = reader.real(position + 1 + static_cast<int>(index), CHABOCHE_NAMES[index], 0.0);
            position += LINE;
            // The second line (QM, MU, ETA) has a blank first field, a keyword never has.
            if (position <= last && reader.isBlank(position) && lineHasValues(reader, position))
            {
                for (std::size_t index = 7; index < 10; ++index)
                    values[index] = reader.real(position + 1 + static_cast<int>(index - 7), CHABOCHE_NAMES[index], 0.0);
                requireBlankRange(reader, position + 4, position + 7, "chabocheHardening");
                position += LINE;
            }
            card.chabocheHardening = values;
        }
        else if (is(keyword, "JohnsonCookHardening"))
        {
            if (card.johnsonCookHardening)
                reader.error("card.bad-value", groupTwice(reader, "johnsonCookHardening"), "johnsonCookHardening");
            std::array<double, 8> values{};
            for (std::size_t index = 0; index < 7; ++index)
                values[index] = reader.real(position + 1 + static_cast<int>(index), JOHNSON_COOK_NAMES[index], 0.0);
            position += LINE;
            if (position <= last && reader.isBlank(position) && lineHasValues(reader, position))
            {
                values[7] = reader.real(position + 1, JOHNSON_COOK_NAMES[7], 0.0);
                requireBlankRange(reader, position + 2, position + 7, "johnsonCookHardening");
                position += LINE;
            }
            card.johnsonCookHardening = values;
        }
        else if (is(keyword, "ImplicitCreep"))
        {
            if (card.implicitCreep)
                reader.error("card.bad-value", groupTwice(reader, "implicitCreep"), "implicitCreep");
            card.implicitCreep = true;
            card.creepEquivalentStress = reader.optionalReal(position + 1, "creepEquivalentStress");
            requireBlankRange(reader, position + 2, position + 7, "implicitCreep");
            position += LINE;
        }
        else
        {
            reader.error("card.bad-value",
                         "\"" + std::string(keyword) + "\" is not a group of a " + std::string(reader.cardName()) +
                             ": " + groupKeywords(),
                         "");
        }
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ElastoPlasticMatFormat::check(const ElastoPlasticMatCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (card.yieldStress && !(*card.yieldStress > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "yieldStress"};
    if (card.hardeningSlope < 0.0 || !isNumber(card.hardeningSlope))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "hardeningSlope"};
    if (card.form == ElastoPlasticMatForm::Table)
    {
        if (card.stressStrainTableId <= 0)
            return CardProblem{"card.missing-field",
                               std::string(CardProblem::FIELD) +
                                   ", the table of the stress-plastic strain function, is required for " +
                                   CardProblem::fieldNamed("form") + " = TABLE",
                               "stressStrainTableId"};
    }
    else
    {
        if (card.stressStrainTableId != 0)
            return CardProblem{
                "card.bad-value",
                std::string(CardProblem::FIELD) + " belongs to " + CardProblem::fieldNamed("form") + " = TABLE",
                "stressStrainTableId"};
        if (card.form == ElastoPlasticMatForm::Perfect && card.hardeningSlope != 0.0)
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " must be blank for a perfectly plastic material",
                               "hardeningSlope"};
    }
    // A group goes with the model it is for (format reference).
    if (card.anisotropy && card.yieldCriterion != ElastoPlasticMatYield::Hill &&
        card.yieldCriterion != ElastoPlasticMatYield::Barlat)
        return CardProblem{"card.bad-value",
                           "the " + std::string(CardProblem::FIELD) + " group needs " +
                               CardProblem::fieldNamed("yieldCriterion") + " = HILL or BARLAT",
                           "anisotropy"};
    if (card.pressureDependence && card.yieldCriterion != ElastoPlasticMatYield::LinMohr &&
        card.yieldCriterion != ElastoPlasticMatYield::PblMohr)
        return CardProblem{"card.bad-value",
                           "the " + std::string(CardProblem::FIELD) + " group needs " +
                               CardProblem::fieldNamed("yieldCriterion") + " = LINMOHR or PBLMOHR",
                           "pressureDependence"};
    if (card.chabocheHardening && card.workHardening != ElastoPlasticMatHardening::Chaboche)
        return CardProblem{"card.bad-value",
                           "the " + std::string(CardProblem::FIELD) + " group needs " +
                               CardProblem::fieldNamed("workHardening") + " = CHABOCHE",
                           "chabocheHardening"};
    if (card.johnsonCookHardening && card.workHardening != ElastoPlasticMatHardening::JohnsonCook)
        return CardProblem{"card.bad-value",
                           "the " + std::string(CardProblem::FIELD) + " group needs " +
                               CardProblem::fieldNamed("workHardening") + " = JHCOOK",
                           "johnsonCookHardening"};
    if (card.implicitCreep && card.yieldCriterion != ElastoPlasticMatYield::ImpCreep)
        return CardProblem{"card.bad-value",
                           "the " + std::string(CardProblem::FIELD) + " group needs " +
                               CardProblem::fieldNamed("yieldCriterion") + " = IMPCREEP",
                           "implicitCreep"};
    if (!card.implicitCreep && card.creepEquivalentStress)
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " belongs to the " + CardProblem::fieldNamed("implicitCreep") + " group",
            "creepEquivalentStress"};
    if (card.rateDependence)
    {
        const ElastoPlasticMatRateGroup& group = *card.rateDependence;
        if (const auto problem = checkTableReference(group.strainRateTableId, "strainRateTableId"))
            return problem;
        if (group.option == ElastoPlasticMatRateOption::Table && group.strainRateTableId <= 0)
            return CardProblem{"card.missing-field",
                               std::string(CardProblem::FIELD) + " is required for the TABLE option of " +
                                   CardProblem::fieldNamed("rateDependence"),
                               "strainRateTableId"};
        if (group.option == ElastoPlasticMatRateOption::Cowper && group.strainRateTableId != 0)
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " belongs to the TABLE option of " +
                                   CardProblem::fieldNamed("rateDependence"),
                               "strainRateTableId"};
        if (group.option == ElastoPlasticMatRateOption::Table &&
            (group.cowperCoefficient != 1.0 || group.cowperExponent != 1.0))
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " and " + CardProblem::fieldNamed("cowperExponent") +
                                   " belong to the COWPER option of " + CardProblem::fieldNamed("rateDependence"),
                               "hardeningOrRateCoefficient"};
    }
    if (card.pressureDependence)
    {
        const ElastoPlasticMatPressureGroup& group = *card.pressureDependence;
        if (group.mohrCoulombAlpha && !(*group.mohrCoulombAlpha > 0.0))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                               "mohrCoulombAlpha"};
        if (group.mohrCoulombBeta && !(*group.mohrCoulombBeta > 0.0))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                               "mohrCoulombBeta"};
    }
    if (card.anisotropy)
    {
        for (std::size_t index = 0; index < 6; ++index)
        {
            if (!isNumber((*card.anisotropy)[index]))
                return CardProblem{"card.bad-value",
                                   "the terms of " + CardProblem::fieldNamed("anisotropy") + " must be finite numbers",
                                   ANISOTROPY_NAMES[index]};
        }
    }
    return std::nullopt;
}

void ElastoPlasticMatFormat::write(const ElastoPlasticMatCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.textUnlessDefault(2, formWord(card.form), "Slope");
    if (card.yieldStress)
        writer.real(3, *card.yieldStress);
    writer.integerUnlessDefault(4, card.stressStrainTableId, 0);
    writer.textUnlessDefault(5, yieldWord(card.yieldCriterion), "VonMises");
    writer.textUnlessDefault(6, hardeningWord(card.workHardening), "IsotropicHardening");
    writer.realUnlessDefault(8, card.hardeningSlope, 0.0);
    int position = FIRST_GROUP_FIELD;
    if (card.rateDependence)
    {
        const ElastoPlasticMatRateGroup& group = *card.rateDependence;
        writer.text(position, "RateEffect");
        writer.textUnlessDefault(position + 1, rateWord(group.option), "Table");
        writer.integerUnlessDefault(position + 2, group.strainRateTableId, 0);
        writer.realUnlessDefault(position + 3, group.cowperCoefficient, 1.0);
        writer.realUnlessDefault(position + 4, group.cowperExponent, 1.0);
        position += LINE;
    }
    if (card.anisotropy)
    {
        writer.text(position, "AnisotropicYield");
        for (std::size_t index = 0; index < 6; ++index)
            writer.realUnlessDefault(position + 2 + static_cast<int>(index), (*card.anisotropy)[index], 1.0);
        position += LINE;
    }
    if (card.pressureDependence)
    {
        const ElastoPlasticMatPressureGroup& group = *card.pressureDependence;
        writer.text(position, "PressureDependence");
        writer.textUnlessDefault(position + 1, pressureWord(group.option), "Linear");
        if (group.mohrCoulombAlpha)
            writer.real(position + 2, *group.mohrCoulombAlpha);
        if (group.mohrCoulombBeta)
            writer.real(position + 3, *group.mohrCoulombBeta);
        position += LINE;
    }
    if (card.chabocheHardening)
    {
        const std::array<double, 10>& values = *card.chabocheHardening;
        writer.text(position, "ChabocheHardening");
        for (std::size_t index = 0; index < 7; ++index)
            writer.realUnlessDefault(position + 1 + static_cast<int>(index), values[index], 0.0);
        position += LINE;
        if (values[7] != 0.0 || values[8] != 0.0 || values[9] != 0.0)
        {
            for (std::size_t index = 7; index < 10; ++index)
                writer.realUnlessDefault(position + 1 + static_cast<int>(index - 7), values[index], 0.0);
            // The line must exist even when the first of its values is the default.
            position += LINE;
        }
    }
    if (card.johnsonCookHardening)
    {
        const std::array<double, 8>& values = *card.johnsonCookHardening;
        writer.text(position, "JohnsonCookHardening");
        for (std::size_t index = 0; index < 7; ++index)
            writer.realUnlessDefault(position + 1 + static_cast<int>(index), values[index], 0.0);
        position += LINE;
        if (values[7] != 0.0)
        {
            writer.real(position + 1, values[7]);
            position += LINE;
        }
    }
    if (card.implicitCreep)
    {
        writer.text(position, "ImplicitCreep");
        if (card.creepEquivalentStress)
            writer.real(position + 1, *card.creepEquivalentStress);
        position += LINE;
    }
}

std::uint32_t ElastoPlasticMatTable::append(const ElastoPlasticMatCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    const ElastoPlasticMatRateGroup rate = card.rateDependence.value_or(ElastoPlasticMatRateGroup{});
    const ElastoPlasticMatPressureGroup pressure = card.pressureDependence.value_or(ElastoPlasticMatPressureGroup{});
    m_id.push(card.id);
    m_form.push(static_cast<std::uint8_t>(card.form));
    m_yieldStress.push(card.yieldStress.value_or(notGiven()));
    m_stressStrainTableId.push(card.stressStrainTableId);
    m_yieldCriterion.push(static_cast<std::uint8_t>(card.yieldCriterion));
    m_workHardening.push(static_cast<std::uint8_t>(card.workHardening));
    m_hardeningSlope.push(card.hardeningSlope);
    m_groups.push(static_cast<std::uint8_t>(
        (card.rateDependence ? RATE_DEPENDENCE_BIT : 0) | (card.anisotropy ? ANISOTROPY_BIT : 0) |
        (card.pressureDependence ? PRESSURE_DEPENDENCE_BIT : 0) |
        (card.chabocheHardening ? CHABOCHE_HARDENING_BIT : 0) |
        (card.johnsonCookHardening ? JOHNSON_COOK_HARDENING_BIT : 0) | (card.implicitCreep ? IMPLICIT_CREEP_BIT : 0)));
    m_rateOption.push(static_cast<std::uint8_t>(rate.option));
    m_strainRateTableId.push(rate.strainRateTableId);
    m_cowperCoefficient.push(rate.cowperCoefficient);
    m_cowperExponent.push(rate.cowperExponent);
    m_anisotropy.push(card.anisotropy.value_or(std::array<double, 6>{1.0, 1.0, 1.0, 1.0, 1.0, 1.0}));
    m_pressureOption.push(static_cast<std::uint8_t>(pressure.option));
    m_mohrCoulombAlpha.push(pressure.mohrCoulombAlpha.value_or(notGiven()));
    m_mohrCoulombBeta.push(pressure.mohrCoulombBeta.value_or(notGiven()));
    m_chabocheHardening.push(card.chabocheHardening.value_or(std::array<double, 10>{}));
    m_johnsonCookHardening.push(card.johnsonCookHardening.value_or(std::array<double, 8>{}));
    m_creepEquivalentStress.push(card.creepEquivalentStress.value_or(notGiven()));
    return row;
}

ElastoPlasticMatCard ElastoPlasticMatTable::card(std::uint32_t row) const
{
    ElastoPlasticMatCard card;
    card.id = m_id.get(row);
    card.form = static_cast<ElastoPlasticMatForm>(m_form.get(row));
    card.yieldStress = optionalOf(m_yieldStress.get(row));
    card.stressStrainTableId = m_stressStrainTableId.get(row);
    card.yieldCriterion = static_cast<ElastoPlasticMatYield>(m_yieldCriterion.get(row));
    card.workHardening = static_cast<ElastoPlasticMatHardening>(m_workHardening.get(row));
    card.hardeningSlope = m_hardeningSlope.get(row);
    const std::uint8_t groups = m_groups.get(row);
    if ((groups & RATE_DEPENDENCE_BIT) != 0)
    {
        ElastoPlasticMatRateGroup rate;
        rate.option = static_cast<ElastoPlasticMatRateOption>(m_rateOption.get(row));
        rate.strainRateTableId = m_strainRateTableId.get(row);
        rate.cowperCoefficient = m_cowperCoefficient.get(row);
        rate.cowperExponent = m_cowperExponent.get(row);
        card.rateDependence = rate;
    }
    if ((groups & ANISOTROPY_BIT) != 0)
        card.anisotropy = m_anisotropy.get(row);
    if ((groups & PRESSURE_DEPENDENCE_BIT) != 0)
    {
        ElastoPlasticMatPressureGroup pressure;
        pressure.option = static_cast<ElastoPlasticMatPressureOption>(m_pressureOption.get(row));
        pressure.mohrCoulombAlpha = optionalOf(m_mohrCoulombAlpha.get(row));
        pressure.mohrCoulombBeta = optionalOf(m_mohrCoulombBeta.get(row));
        card.pressureDependence = pressure;
    }
    if ((groups & CHABOCHE_HARDENING_BIT) != 0)
        card.chabocheHardening = m_chabocheHardening.get(row);
    if ((groups & JOHNSON_COOK_HARDENING_BIT) != 0)
        card.johnsonCookHardening = m_johnsonCookHardening.get(row);
    card.implicitCreep = (groups & IMPLICIT_CREEP_BIT) != 0;
    card.creepEquivalentStress = optionalOf(m_creepEquivalentStress.get(row));
    return card;
}

void ElastoPlasticMatTable::store(std::uint32_t row, const ElastoPlasticMatCard& card)
{
    const ElastoPlasticMatRateGroup rate = card.rateDependence.value_or(ElastoPlasticMatRateGroup{});
    const ElastoPlasticMatPressureGroup pressure = card.pressureDependence.value_or(ElastoPlasticMatPressureGroup{});
    m_id.set(row, card.id);
    m_form.set(row, static_cast<std::uint8_t>(card.form));
    m_yieldStress.set(row, card.yieldStress.value_or(notGiven()));
    m_stressStrainTableId.set(row, card.stressStrainTableId);
    m_yieldCriterion.set(row, static_cast<std::uint8_t>(card.yieldCriterion));
    m_workHardening.set(row, static_cast<std::uint8_t>(card.workHardening));
    m_hardeningSlope.set(row, card.hardeningSlope);
    m_groups.set(row, static_cast<std::uint8_t>((card.rateDependence ? RATE_DEPENDENCE_BIT : 0) |
                                                (card.anisotropy ? ANISOTROPY_BIT : 0) |
                                                (card.pressureDependence ? PRESSURE_DEPENDENCE_BIT : 0) |
                                                (card.chabocheHardening ? CHABOCHE_HARDENING_BIT : 0) |
                                                (card.johnsonCookHardening ? JOHNSON_COOK_HARDENING_BIT : 0) |
                                                (card.implicitCreep ? IMPLICIT_CREEP_BIT : 0)));
    m_rateOption.set(row, static_cast<std::uint8_t>(rate.option));
    m_strainRateTableId.set(row, rate.strainRateTableId);
    m_cowperCoefficient.set(row, rate.cowperCoefficient);
    m_cowperExponent.set(row, rate.cowperExponent);
    m_anisotropy.set(row, card.anisotropy.value_or(std::array<double, 6>{1.0, 1.0, 1.0, 1.0, 1.0, 1.0}));
    m_pressureOption.set(row, static_cast<std::uint8_t>(pressure.option));
    m_mohrCoulombAlpha.set(row, pressure.mohrCoulombAlpha.value_or(notGiven()));
    m_mohrCoulombBeta.set(row, pressure.mohrCoulombBeta.value_or(notGiven()));
    m_chabocheHardening.set(row, card.chabocheHardening.value_or(std::array<double, 10>{}));
    m_johnsonCookHardening.set(row, card.johnsonCookHardening.value_or(std::array<double, 8>{}));
    m_creepEquivalentStress.set(row, card.creepEquivalentStress.value_or(notGiven()));
}

void ElastoPlasticMatTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_form.keepRows(rows);
    m_yieldStress.keepRows(rows);
    m_stressStrainTableId.keepRows(rows);
    m_yieldCriterion.keepRows(rows);
    m_workHardening.keepRows(rows);
    m_hardeningSlope.keepRows(rows);
    m_groups.keepRows(rows);
    m_rateOption.keepRows(rows);
    m_strainRateTableId.keepRows(rows);
    m_cowperCoefficient.keepRows(rows);
    m_cowperExponent.keepRows(rows);
    m_anisotropy.keepRows(rows);
    m_pressureOption.keepRows(rows);
    m_mohrCoulombAlpha.keepRows(rows);
    m_mohrCoulombBeta.keepRows(rows);
    m_chabocheHardening.keepRows(rows);
    m_johnsonCookHardening.keepRows(rows);
    m_creepEquivalentStress.keepRows(rows);
}

void ElastoPlasticMatTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the material that the card extends; FID and RTID are TABLES1 cards.
    m_id.set(row, visitor.visit("id", ReferenceTarget::Material, m_id.get(row)));
    if (const base::EntityId stressStrainTable = m_stressStrainTableId.get(row); stressStrainTable > 0)
        m_stressStrainTableId.set(
            row,
            visitor.visit("stressStrainTableId", ReferenceTarget::StressOrMultiDimensionalTable, stressStrainTable));
    if ((m_groups.get(row) & RATE_DEPENDENCE_BIT) != 0)
    {
        if (const base::EntityId strainRateTableId = m_strainRateTableId.get(row); strainRateTableId > 0)
            m_strainRateTableId.set(
                row, visitor.visit("strainRateTableId", ReferenceTarget::StressTable, strainRateTableId));
    }
}

} // namespace oor::model
