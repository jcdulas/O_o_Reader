#include "oor/model/cards/materials/nonlinearMatOptions/NonlinearMatOptionsTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <cmath>
#include <initializer_list>
#include <string>

namespace oor::model
{

namespace
{

constexpr int LINE = 8; // fields to a line

bool is(std::string_view text, std::string_view word)
{
    return (text == word);
}

// True when the line that starts at `first` has any field after the keyword that is not blank.
bool lineHasValues(const FieldReader& reader, int first)
{
    for (int field = first; field < first + LINE; ++field)
    {
        if (!reader.isBlank(field))
            return true;
    }
    return false;
}

// True when `value` is one of `words` (the empty word stands for a blank field).
bool isOneOf(const std::string& value, std::initializer_list<const char*> words)
{
    for (const char* word : words)
    {
        if (value == word)
            return true;
    }
    return false;
}

// Reports an error when a field of `first` to `last` of the line of the option `option` is not blank.
bool requireBlank(FieldReader& reader, int first, int last, const char* option)
{
    for (int field = first; field <= last; ++field)
    {
        if (!reader.isBlank(field))
        {
            reader.error("card.bad-value",
                         "field " + std::to_string(field) + " of the " + std::string(option) + " option of a " +
                             std::string(reader.cardName()) + " is not used and must be blank",
                         option);
            return false;
        }
    }
    return true;
}

// The word of the field as written; the value of a blank field is empty.
std::string wordAt(const FieldReader& reader, int field)
{
    return std::string(reader.text(field));
}

void writeWord(FieldWriter& writer, int field, const std::string& word)
{
    if (!word.empty())
        writer.text(field, word);
}

} // namespace

std::optional<NonlinearMatOptionsCard> NonlinearMatOptionsFormat::read(FieldReader& reader)
{
    NonlinearMatOptionsCard card;
    const int last = reader.lastUsedField();
    for (int line = 1; line <= last && !reader.failed(); line += LINE)
    {
        const std::string_view keyword = reader.text(line);
        if (keyword.empty())
        {
            if (lineHasValues(reader, line))
            {
                reader.error("card.bad-value",
                             "a line of a " + std::string(reader.cardName()) + " starts with the keyword of an option",
                             "");
                return card;
            }
            continue;
        }
        const int v = line + 1; // the first value
        const auto taken = [&](bool present, const char* name)
        {
            if (!present)
                return true;
            reader.error(
                "card.bad-value",
                "the " + std::string(name) + " option of a " + std::string(reader.cardName()) + " is there twice",
                name);
            return false;
        };
        // Starts an option: it is there once, and the fields `firstBlank` to `lastBlank` of its line are blank.
        const auto opens = [&](bool present, const char* option, int firstBlank, int lastBlank)
        { return taken(present, option) && requireBlank(reader, firstBlank, lastBlank, option); };
        if (is(keyword, "Creep"))
        {
            if (!opens(card.creep.has_value(), "creepOption", v + 4, v + 6))
                return card;
            card.creep = NonlinearMatOptionsCreep{
                reader.integer(v, "explicitCreepModel", 0), reader.integer(v + 1, "explicitCreepOption", 0),
                reader.integer(v + 2, "implicitCreep", 0), reader.integer(v + 3, "implicitCreepTangent", 0)};
        }
        else if (is(keyword, "AssumedStrainOption"))
        {
            if (!opens(card.assumedStrain.has_value(), "assumedStrain", v + 1, v + 6))
                return card;
            card.assumedStrain = wordAt(reader, v);
        }
        else if (is(keyword, "TransverseShear"))
        {
            if (!opens(card.transverseShear.has_value(), "transverseShear", v + 1, v + 6))
                return card;
            card.transverseShear = wordAt(reader, v);
        }
        else if (is(keyword, "LargeStrain"))
        {
            if (!opens(card.largeStrain.has_value(), "largeStrain", v + 1, v + 6))
                return card;
            card.largeStrain = reader.integer(v, "largeStrain", 0);
        }
        else if (is(keyword, "Hemicube"))
        {
            if (!opens(card.hemicube.has_value(), "hemicubeOption", v + 2, v + 2))
                return card;
            NonlinearMatOptionsHemicube hemicube;
            hemicube.method = reader.integer(v, "viewFactorMethod", 0);
            hemicube.pixelCount = reader.integer(v + 1, "pixelCount", NonlinearMatOptionsHemicube::DEFAULT_PIXEL_COUNT);
            hemicube.viewFactorCutoff = reader.real(v + 3, "viewFactorCutoff", 0.0);
            hemicube.implicitFraction =
                reader.real(v + 4, "implicitFraction", NonlinearMatOptionsHemicube::DEFAULT_IMPLICIT_FRACTION);
            hemicube.explicitReflection = reader.integer(v + 5, "explicitReflection", 0);
            hemicube.reflectionTolerance = reader.optionalReal(v + 6, "reflectionTolerance");
            card.hemicube = hemicube;
        }
        else if (is(keyword, "TemperatureDistribution"))
        {
            if (!opens(card.temperatureDistribution.has_value(), "temperatureDistribution", v + 1, v + 6))
                return card;
            card.temperatureDistribution = wordAt(reader, v);
        }
        else if (is(keyword, "GridMapping"))
        {
            if (!opens(card.gridMapping.has_value(), "gridMapping", v + 1, v + 6))
                return card;
            card.gridMapping = wordAt(reader, v);
        }
        else if (is(keyword, "PropertyMap"))
        {
            if (!opens(card.propertyMap.has_value(), "propertyMapOption", v + 5, v + 6))
                return card;
            NonlinearMatOptionsPropertyMap map;
            map.mapping = reader.integer(v, "propertyMapping", 0);
            map.behaviour = wordAt(reader, v + 1);
            map.layerDirection = reader.integer(v + 2, "layerDirection", 1);
            map.outOfPlaneThickness = reader.real(v + 3, "outOfPlaneThickness", 1.0);
            map.printFlag = reader.integer(v + 4, "printFlag", 0);
            card.propertyMap = map;
        }
        else if (is(keyword, "ConstraintRamp"))
        {
            if (!opens(card.spcRamp.has_value(), "spcRampOption", v + 1, v + 6))
                return card;
            card.spcRamp = reader.integer(v, "spcRamp", 0);
        }
        else if (is(keyword, "Deactivation"))
        {
            if (!opens(card.deactivation.has_value(), "deactivationOption", v + 2, v + 6))
                return card;
            card.deactivation = NonlinearMatOptionsDeactivation{reader.optionalInteger(v, "deactivationRule"),
                                                                reader.integer(v + 1, "deactivationOutput", 0)};
        }
        else if (is(keyword, "Enthalpy"))
        {
            if (!opens(card.enthalpy.has_value(), "enthalpyOption", v + 2, v + 6))
                return card;
            card.enthalpy = NonlinearMatOptionsEnthalpy{reader.integer(v, "capacitanceScheme", 0),
                                                        reader.integer(v + 1, "enthalpyScheme", 0)};
        }
        else if (is(keyword, "MapTolerance"))
        {
            if (!opens(card.mapTolerance.has_value(), "mapToleranceOption", v + 1, v + 6))
                return card;
            card.mapTolerance = reader.real(v, "mapTolerance", 0.2);
        }
        else if (is(keyword, "Interlaminar"))
        {
            if (!opens(card.interlaminar.has_value(), "interlaminarOption", v + 3, v + 6))
                return card;
            NonlinearMatOptionsInterlaminar interlaminar;
            interlaminar.frame = wordAt(reader, v);
            if (const std::string word = wordAt(reader, v + 1); !word.empty())
                interlaminar.axisProjection = word;
            interlaminar.strainOutput = reader.integer(v + 2, "strainOutput", 0);
            card.interlaminar = interlaminar;
        }
        else
        {
            reader.error(
                "card.bad-value",
                "\"" + std::string(keyword) + "\" is not an option of a " + std::string(reader.cardName()) + ": " +
                    fieldNameList({"creepOption", "assumedStrain", "transverseShear", "largeStrain", "hemicubeOption",
                                   "temperatureDistribution", "gridMapping", "propertyMapOption", "spcRampOption",
                                   "deactivationOption", "enthalpyOption", "mapToleranceOption", "interlaminarOption"}),
                "");
            return card;
        }
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> NonlinearMatOptionsFormat::check(const NonlinearMatOptionsCard& card)
{
    // The message follows the keyword of the field.
    const auto bad = [](const char* message, const char* field)
    { return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + message, field}; };
    if (card.creep)
    {
        const NonlinearMatOptionsCreep& creep = *card.creep;
        if (creep.explicitModel < 0 || creep.explicitModel > 1)
            return bad(" must be 0 or 1", "explicitCreepModel");
        if (creep.explicitOption < 0 || creep.explicitOption > 1)
            return bad(" must be 0 or 1", "explicitCreepOption");
        if (creep.implicitCreep < 0 || creep.implicitCreep > 1)
            return bad(" must be 0 or 1", "implicitCreep");
        if (creep.implicitTangent < 0 || creep.implicitTangent > 2)
            return bad(" must be 0, 1 or 2", "implicitCreepTangent");
    }
    if (card.assumedStrain && !isOneOf(*card.assumedStrain, {"", "AssumedStrain", "Off"}))
        return bad(" must be ASSUMED, OFF or blank", "assumedStrain");
    if (card.transverseShear && !isOneOf(*card.transverseShear, {"", "TransverseShear"}))
        return bad(" must be TSHEAR or blank", "transverseShear");
    if (card.largeStrain && (*card.largeStrain < -1 || *card.largeStrain > 2))
        return bad(" must be -1, 0, 1 or 2", "largeStrain");
    if (card.hemicube)
    {
        const NonlinearMatOptionsHemicube& hemicube = *card.hemicube;
        if (hemicube.method < 0 || hemicube.method > 1)
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " of " + CardProblem::fieldNamed("hemicubeOption") +
                                   " must be 0 or 1",
                               "viewFactorMethod"};
        if (hemicube.pixelCount <= 0)
            return bad(" must be greater than zero", "pixelCount");
        if (!(hemicube.viewFactorCutoff >= 0.0) || !std::isfinite(hemicube.viewFactorCutoff))
            return bad(" must not be negative", "viewFactorCutoff");
        if (!(hemicube.implicitFraction >= 0.0) || !std::isfinite(hemicube.implicitFraction))
            return bad(" must not be negative", "implicitFraction");
        if (hemicube.explicitReflection < 0 || hemicube.explicitReflection > 1)
            return bad(" must be 0 or 1", "explicitReflection");
        if (hemicube.reflectionTolerance &&
            !(*hemicube.reflectionTolerance > 0.0 && std::isfinite(*hemicube.reflectionTolerance)))
            return bad(" must be greater than zero", "reflectionTolerance");
    }
    if (card.temperatureDistribution &&
        !isOneOf(*card.temperatureDistribution, {"", "Constant", "Linear", "Quadratic"}))
        return bad(" must be CONS, LINE, QUAD or blank", "temperatureDistribution");
    if (card.gridMapping && !isOneOf(*card.gridMapping, {"", "No", "Yes"}))
        return bad(" must be NO, YES or blank", "gridMapping");
    if (card.propertyMap)
    {
        const NonlinearMatOptionsPropertyMap& map = *card.propertyMap;
        if (map.mapping < -1 || map.mapping > 3)
            return bad(" must be from -1 to 3", "propertyMapping");
        if (!isOneOf(map.behaviour,
                     {"", "OneDimensional", "ThreeDimensional", "PlaneStrain", "PlaneStress", "Axisymmetric"}))
            return bad(" must be 1D, 3D, PLST, PSTR, AXI or blank", "propertyBehaviour");
        if (map.layerDirection < 1 || map.layerDirection > 3)
            return bad(" must be 1, 2 or 3", "layerDirection");
        if (!(map.outOfPlaneThickness > 0.0) || !std::isfinite(map.outOfPlaneThickness))
            return bad(" must be greater than zero", "outOfPlaneThickness");
        if (map.printFlag < 0 || map.printFlag > 3)
            return bad(" must be from 0 to 3", "printFlag");
    }
    if (card.spcRamp && (*card.spcRamp < 0 || *card.spcRamp > 1))
        return bad(" must be 0 or 1", "spcRamp");
    if (card.deactivation)
    {
        if (card.deactivation->rule && (*card.deactivation->rule < -1 || *card.deactivation->rule > 2))
            return bad(" must be from -1 to 2", "deactivationRule");
        if (card.deactivation->output < 0 || card.deactivation->output > 3)
            return bad(" must be from 0 to 3", "deactivationOutput");
    }
    if (card.enthalpy)
    {
        if (card.enthalpy->capacitanceScheme < 0 || card.enthalpy->capacitanceScheme > 3)
            return bad(" must be from 0 to 3", "capacitanceScheme");
        if (card.enthalpy->enthalpyScheme < 0 || card.enthalpy->enthalpyScheme > 2)
            return bad(" must be 0, 1 or 2", "enthalpyScheme");
    }
    if (card.mapTolerance && !(*card.mapTolerance > 0.0 && std::isfinite(*card.mapTolerance)))
        return bad(" must be greater than zero", "mapTolerance");
    if (card.interlaminar)
    {
        if (!isOneOf(card.interlaminar->frame, {"", "BottomSurface", "Top"}))
            return bad(" must be BOTT, TOP or blank", "interlaminarFrame");
        if (!isOneOf(card.interlaminar->axisProjection, {"Off", "On"}))
            return bad(" must be OFF or ON", "axisProjection");
        if (card.interlaminar->strainOutput < 0 || card.interlaminar->strainOutput > 1)
            return bad(" must be 0 or 1", "strainOutput");
    }
    return std::nullopt;
}

void NonlinearMatOptionsFormat::write(const NonlinearMatOptionsCard& card, FieldWriter& writer)
{
    int line = 1;
    // Starts the line of an option and gives the field of its first value.
    const auto start = [&](const char* keyword)
    {
        writer.text(line, keyword);
        const int first = line + 1;
        line += LINE;
        return first;
    };
    if (card.creep)
    {
        const int v = start("Creep");
        writer.integerUnlessDefault(v, card.creep->explicitModel, 0);
        writer.integerUnlessDefault(v + 1, card.creep->explicitOption, 0);
        writer.integerUnlessDefault(v + 2, card.creep->implicitCreep, 0);
        writer.integerUnlessDefault(v + 3, card.creep->implicitTangent, 0);
    }
    if (card.assumedStrain)
        writeWord(writer, start("AssumedStrainOption"), *card.assumedStrain);
    if (card.transverseShear)
        writeWord(writer, start("TransverseShear"), *card.transverseShear);
    if (card.largeStrain)
        writer.integer(start("LargeStrain"), *card.largeStrain);
    if (card.hemicube)
    {
        const int v = start("Hemicube");
        writer.integerUnlessDefault(v, card.hemicube->method, 0);
        writer.integerUnlessDefault(v + 1, card.hemicube->pixelCount, NonlinearMatOptionsHemicube::DEFAULT_PIXEL_COUNT);
        writer.realUnlessDefault(v + 3, card.hemicube->viewFactorCutoff, 0.0);
        writer.realUnlessDefault(v + 4, card.hemicube->implicitFraction,
                                 NonlinearMatOptionsHemicube::DEFAULT_IMPLICIT_FRACTION);
        writer.integerUnlessDefault(v + 5, card.hemicube->explicitReflection, 0);
        if (card.hemicube->reflectionTolerance)
            writer.real(v + 6, *card.hemicube->reflectionTolerance);
    }
    if (card.temperatureDistribution)
        writeWord(writer, start("TemperatureDistribution"), *card.temperatureDistribution);
    if (card.gridMapping)
        writeWord(writer, start("GridMapping"), *card.gridMapping);
    if (card.propertyMap)
    {
        const int v = start("PropertyMap");
        writer.integerUnlessDefault(v, card.propertyMap->mapping, 0);
        writeWord(writer, v + 1, card.propertyMap->behaviour);
        writer.integerUnlessDefault(v + 2, card.propertyMap->layerDirection, 1);
        writer.realUnlessDefault(v + 3, card.propertyMap->outOfPlaneThickness, 1.0);
        writer.integerUnlessDefault(v + 4, card.propertyMap->printFlag, 0);
    }
    if (card.spcRamp)
        writer.integer(start("ConstraintRamp"), *card.spcRamp);
    if (card.deactivation)
    {
        const int v = start("Deactivation");
        if (card.deactivation->rule)
            writer.integer(v, *card.deactivation->rule);
        writer.integerUnlessDefault(v + 1, card.deactivation->output, 0);
    }
    if (card.enthalpy)
    {
        const int v = start("Enthalpy");
        writer.integerUnlessDefault(v, card.enthalpy->capacitanceScheme, 0);
        writer.integerUnlessDefault(v + 1, card.enthalpy->enthalpyScheme, 0);
    }
    if (card.mapTolerance)
        writer.real(start("MapTolerance"), *card.mapTolerance);
    if (card.interlaminar)
    {
        const int v = start("Interlaminar");
        writeWord(writer, v, card.interlaminar->frame);
        writer.textUnlessDefault(v + 1, card.interlaminar->axisProjection, "Off");
        writer.integerUnlessDefault(v + 2, card.interlaminar->strainOutput, 0);
    }
}

std::uint32_t NonlinearMatOptionsTable::append(const NonlinearMatOptionsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_cards.push(card);
    return row;
}

} // namespace oor::model
