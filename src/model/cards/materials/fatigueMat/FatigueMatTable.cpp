#include "oor/model/cards/materials/fatigueMat/FatigueMatTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <cmath>
#include <string>

namespace oor::model
{

namespace
{

constexpr int LINE = 8; // fields to a line
constexpr int FIRST_BLOCK_FIELD = 9;
constexpr const char* CURVE_KEYWORDS[FatigueMatCard::CURVES] = {
    "StressLifeMethod", "StressLifeSegment1", "StressLifeSegment2", "StressLifeBranch1", "StressLifeBranch0"};
// The names of the code of the curve blocks, in the same order: the curve of the material, the curves of the two
// sheets of a spot weld, and the curves of a seam weld under pure bending (bending ratio 1) and under pure membrane
// stress (bending ratio 0), between which the bending ratio of a weld interpolates.
constexpr const char* CURVE_NAMES[FatigueMatCard::CURVES] = {"snCurve", "spotWeldSheetCurve1", "spotWeldSheetCurve2",
                                                             "seamWeldBendingCurve", "seamWeldMembraneCurve"};
constexpr int MAX_TOKENS = 1000000; // a bound on the tokens of the MATID list

bool is(std::string_view text, std::string_view word)
{
    return (text == word);
}

bool lineHasValues(const FieldReader& reader, int first)
{
    for (int field = first; field < first + LINE; ++field)
    {
        if (!reader.isBlank(field))
            return true;
    }
    return false;
}

// True when the line at `line` continues the block above: it is on the card, has a blank first field and a value.
bool continues(const FieldReader& reader, int line, int last)
{
    return line <= last && reader.isBlank(line) && lineHasValues(reader, line);
}

// Reports an error when a field of `first` to `last` is not blank; the cells the format reference leaves empty on a
// line. `block` is the name of the code of the block (its keyword in a message), empty for the first line.
bool requireBlank(FieldReader& reader, int first, int last, const char* block)
{
    for (int field = first; field <= last; ++field)
    {
        if (!reader.isBlank(field))
        {
            const std::string where = *block == '\0' ? std::string("first line") : std::string(block) + " lines";
            reader.error("card.bad-value",
                         "field " + std::to_string(field) + " of the " + where + " of a " +
                             std::string(reader.cardName()) + " is not used and must be blank",
                         block);
            return false;
        }
    }
    return true;
}

template <std::size_t N>
void readValues(FieldReader& reader, int first, const char* name, std::array<std::optional<double>, N>& values)
{
    for (std::size_t index = 0; index < N; ++index)
        values[index] = reader.optionalReal(first + static_cast<int>(index), name);
}

template <std::size_t N>
void writeValues(FieldWriter& writer, int first, const std::array<std::optional<double>, N>& values,
                 std::size_t from = 0, std::size_t count = N)
{
    for (std::size_t index = 0; index < count; ++index)
    {
        if (values[from + index])
            writer.real(first + static_cast<int>(index), *values[from + index]);
    }
}

template <std::size_t N>
bool anyGiven(const std::array<std::optional<double>, N>& values, std::size_t from = 0, std::size_t count = N)
{
    for (std::size_t index = from; index < from + count; ++index)
    {
        if (values[index])
            return true;
    }
    return false;
}

template <std::size_t N>
bool allFinite(const std::array<std::optional<double>, N>& values)
{
    for (const std::optional<double>& value : values)
    {
        if (value && !std::isfinite(*value))
            return false;
    }
    return true;
}

bool inRange(const std::optional<double>& value, double low, double high)
{
    return !value || (*value >= low && *value <= high);
}

bool isType(const std::string& type)
{
    for (const char* word : {"", "MeanStress", "AmplitudeMeanStress", "StressRatio", "AmplitudeStressRatio",
                             "ConstantLife", "AmplitudeConstantLife", "Amplitude", "Range"})
    {
        if (type == word)
            return true;
    }
    return false;
}

} // namespace

std::optional<FatigueMatCard> FatigueMatFormat::read(FieldReader& reader)
{
    FatigueMatCard card;
    card.id = reader.requiredInteger(1, "id");
    card.stressUnitFactor = reader.real(2, "stressUnitFactor", 1.0);
    card.debugOutput = reader.integer(3, "debugOutput", 0);
    if (!requireBlank(reader, 4, FIRST_BLOCK_FIELD - 1, ""))
        return card;
    const int last = reader.lastUsedField();
    bool haveStatic = false;
    int position = FIRST_BLOCK_FIELD;
    const auto twice = [&](const char* block)
    {
        reader.error("card.bad-value",
                     "the " + std::string(block) + " lines of a " + std::string(reader.cardName()) + " are there twice",
                     block);
    };
    while (position <= last && !reader.failed())
    {
        const std::string_view keyword = reader.text(position);
        if (keyword.empty())
        {
            if (lineHasValues(reader, position))
            {
                reader.error("card.bad-value",
                             "a line of a " + std::string(reader.cardName()) +
                                 " after the first starts with a keyword (" +
                                 fieldNameList({"staticProperties", "snCurve", "parameterTables"}) + ", ...)",
                             "");
                return card;
            }
            position += LINE;
            continue;
        }
        const int v = position + 1; // the first value of the line
        if (is(keyword, "Static"))
        {
            if (haveStatic)
            {
                twice("staticProperties");
                return card;
            }
            haveStatic = true;
            FatigueMatStatic& fixed = card.fixed;
            fixed.yieldStrength = reader.optionalReal(v, "yieldStrength");
            fixed.ultimateTensileStrength = reader.optionalReal(v + 1, "ultimateTensileStrength");
            if (const auto code = reader.optionalInteger(v + 2, "materialCode"))
                fixed.materialCode = *code;
            fixed.curveKind = std::string(reader.text(v + 3));
            fixed.testStressRatio = reader.optionalReal(v + 4, "testStressRatio");
            fixed.standardError = reader.optionalReal(v + 5, "standardError");
            fixed.plasticPoissonRatio = reader.optionalReal(v + 6, "plasticPoissonRatio");
            position += LINE;
            if (continues(reader, position, last))
            {
                fixed.areaReduction = reader.optionalReal(position + 1, "areaReduction");
                if (!requireBlank(reader, position + 2, position + LINE - 1, "staticProperties"))
                    return card;
                position += LINE;
            }
            continue;
        }
        std::size_t curveIndex = FatigueMatCard::CURVES;
        for (std::size_t index = 0; index < FatigueMatCard::CURVES; ++index)
        {
            if (is(keyword, CURVE_KEYWORDS[index]))
                curveIndex = index;
        }
        if (curveIndex != FatigueMatCard::CURVES)
        {
            if (card.curves[curveIndex])
            {
                twice(CURVE_NAMES[curveIndex]);
                return card;
            }
            FatigueMatCurve curve;
            readValues(reader, v, "snCurveTerm", curve.curve);
            position += LINE;
            // The three optional lines, each with a blank first field: the mean stress parameters, then the two lines
            // of the spot weld factors. A line of the block can be blank: the lines are positional.
            for (std::size_t extra = 0; extra < 3 && position <= last && reader.isBlank(position); ++extra)
            {
                if (extra == 0)
                    readValues(reader, position + 1, "meanStressTerm", curve.meanStress);
                else
                {
                    for (std::size_t index = 0; index < 6; ++index)
                        curve.spotWeld[(extra - 1) * 6 + index] =
                            reader.optionalReal(position + 1 + static_cast<int>(index), "spotWeldTerm");
                    if (!requireBlank(reader, position + 7, position + 7, CURVE_NAMES[curveIndex]))
                        return card;
                }
                position += LINE;
            }
            card.curves[curveIndex] = curve;
            continue;
        }
        if (is(keyword, "Table"))
        {
            if (card.table)
            {
                twice("parameterTables");
                return card;
            }
            std::vector<std::pair<double, base::EntityId>> pairs;
            int line = position;
            bool first = true;
            while (first || continues(reader, line, last))
            {
                const int w = line + 1;
                for (int pair = 0; pair < 3; ++pair)
                {
                    const int valueField = w + 2 * pair;
                    if (reader.isBlank(valueField) && reader.isBlank(valueField + 1))
                        continue;
                    const double value = reader.requiredReal(valueField, "tableKeyValue");
                    const base::EntityId table = reader.requiredInteger(valueField + 1, "tableId");
                    pairs.emplace_back(value, table);
                }
                if (!requireBlank(reader, w + 6, w + 6, "parameterTables"))
                    return card;
                first = false;
                line += LINE;
            }
            card.table = pairs;
            position = line;
            continue;
        }
        if (is(keyword, "MetallicHandbookData"))
        {
            if (card.handbookCoefficients)
            {
                twice("handbookCoefficients");
                return card;
            }
            std::array<std::optional<double>, 4> values{};
            readValues(reader, v, "handbookCoefficient1", values);
            if (!requireBlank(reader, v + 4, v + 6, "handbookCoefficients"))
                return card;
            card.handbookCoefficients = values;
            position += LINE;
            continue;
        }
        if (is(keyword, "StrainLifeMethod"))
        {
            if (card.strainLife)
            {
                twice("strainLife");
                return card;
            }
            FatigueMatStrainLife strain;
            readValues(reader, v, "strainLifeTerm", strain.curve);
            position += LINE;
            if (continues(reader, position, last))
            {
                for (std::size_t index = 0; index < 4; ++index)
                    strain.errors[index] =
                        reader.optionalReal(position + 1 + static_cast<int>(index), "strainLifeError");
                strain.errors[4] = reader.optionalReal(position + 7, "tolerance");
                if (!requireBlank(reader, position + 5, position + 6, "strainLife"))
                    return card;
                position += LINE;
            }
            card.strainLife = strain;
            continue;
        }
        if (is(keyword, "MatIds"))
        {
            if (card.materials)
            {
                twice("servedMaterials");
                return card;
            }
            FatigueMatMaterials materials;
            materials.youngsModulus = reader.optionalReal(v, "youngsModulus");
            materials.poissonRatio = reader.optionalReal(v + 1, "poissonRatio");
            if (!requireBlank(reader, v + 2, v + 6, "servedMaterials"))
                return card;
            position += LINE;
            // The material IDs: seven to a line on the lines with a blank first field, with THRU between two of them.
            bool afterThru = false;
            int tokens = 0;
            while (position <= last && reader.isBlank(position) && tokens < MAX_TOKENS)
            {
                for (int field = position + 1; field < position + LINE; ++field)
                {
                    const std::string_view text = reader.text(field);
                    if (text.empty())
                        continue;
                    ++tokens;
                    if (is(text, "Through"))
                    {
                        if (materials.matIds.empty() || afterThru ||
                            materials.matIds.back().first != materials.matIds.back().last)
                        {
                            reader.error("card.bad-value",
                                         "THRU in the " + std::string("servedMaterials") + " list of a " +
                                             std::string(reader.cardName()) + " needs an ID before it",
                                         "matIds");
                            return card;
                        }
                        afterThru = true;
                        continue;
                    }
                    const base::EntityId id = reader.requiredInteger(field, "matIds");
                    if (afterThru)
                    {
                        materials.matIds.back().last = id;
                        afterThru = false;
                    }
                    else
                        materials.matIds.push_back(FatigueMatIdRun{id, id});
                }
                position += LINE;
            }
            if (afterThru)
            {
                reader.error("card.bad-value",
                             "THRU in the " + std::string("servedMaterials") + " list of a " +
                                 std::string(reader.cardName()) + " needs an ID after it",
                             "matIds");
                return card;
            }
            card.materials = materials;
            continue;
        }
        reader.error("card.bad-value",
                     "\"" + std::string(keyword) + "\" does not start a block of a " + std::string(reader.cardName()) +
                         ": " +
                         fieldNameList({"staticProperties", "snCurve", "spotWeldSheetCurve1", "spotWeldSheetCurve2",
                                        "seamWeldBendingCurve", "seamWeldMembraneCurve", "parameterTables",
                                        "handbookCoefficients", "strainLife", "servedMaterials"}),
                     "");
        return card;
    }
    if (!haveStatic && !reader.failed())
        reader.error("card.bad-value",
                     "a " + std::string(reader.cardName()) + " needs its " + std::string("staticProperties") + " line",
                     "staticProperties");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FatigueMatFormat::check(const FatigueMatCard& card)
{
    const auto bad = [](const std::string& message, const char* field)
    { return CardProblem{"card.bad-value", message, field}; };
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (!(card.stressUnitFactor > 0.0) || !std::isfinite(card.stressUnitFactor))
        return bad(std::string(CardProblem::FIELD) + " must be greater than zero", "stressUnitFactor");
    if (card.debugOutput < 0 || card.debugOutput > 1)
        return bad(std::string(CardProblem::FIELD) + " must be 0 or 1", "debugOutput");
    const FatigueMatStatic& fixed = card.fixed;
    if (!inRange(fixed.yieldStrength, 0.0, 1.0e300) || !inRange(fixed.ultimateTensileStrength, 0.0, 1.0e300))
        return bad(std::string(CardProblem::FIELD) + " and " + CardProblem::fieldNamed("ultimateTensileStrength") +
                       " must be finite and not negative",
                   "yieldStrength");
    if (fixed.materialCode && *fixed.materialCode < 0)
        return bad(std::string(CardProblem::FIELD) + " must not be negative", "materialCode");
    if (!isType(fixed.curveKind))
        return bad(std::string(CardProblem::FIELD) +
                       " must be MeanStress, AmplitudeMeanStress, StressRatio, AmplitudeStressRatio, ConstantLife, "
                       "AmplitudeConstantLife, Amplitude, Range or blank",
                   "curveKind");
    if (fixed.testStressRatio && !(*fixed.testStressRatio <= 1.0))
        return bad(std::string(CardProblem::FIELD) + " must not be more than 1.0", "testStressRatio");
    if (!inRange(fixed.standardError, 0.0, 10.0))
        return bad(std::string(CardProblem::FIELD) + " must be from 0.0 to 10.0", "standardError");
    if (!inRange(fixed.plasticPoissonRatio, 0.45, 0.55))
        return bad(std::string(CardProblem::FIELD) + " must be from 0.45 to 0.55", "plasticPoissonRatio");
    if (!inRange(fixed.areaReduction, 0.0, 100.0))
        return bad(std::string(CardProblem::FIELD) + " must be from 0.0 to 100.0", "areaReduction");
    for (const std::optional<FatigueMatCurve>& curve : card.curves)
    {
        if (curve && (!allFinite(curve->curve) || !allFinite(curve->meanStress) || !allFinite(curve->spotWeld)))
            return bad("the values of an S-N curve must be finite numbers", "snCurveTerm");
    }
    if (card.table)
    {
        for (const auto& [value, table] : *card.table)
        {
            if (!std::isfinite(value))
                return bad(std::string(CardProblem::FIELD) + " must be a finite number", "tableKeyValue");
            if (table <= 0)
                return CardProblem{"card.bad-id",
                                   std::string(CardProblem::FIELD) + " must be the ID of a table, a positive integer",
                                   "tableId"};
        }
    }
    if (card.handbookCoefficients && !allFinite(*card.handbookCoefficients))
        return bad(std::string(CardProblem::FIELD) + " to " + CardProblem::fieldNamed("handbookCoefficient4") +
                       " must be finite numbers",
                   "handbookCoefficient1");
    if (card.strainLife && (!allFinite(card.strainLife->curve) || !allFinite(card.strainLife->errors)))
        return bad("the values of the " + CardProblem::fieldNamed("strainLife") + " lines must be finite numbers",
                   "strainLifeTerm");
    if (card.materials)
    {
        if (card.materials->matIds.empty())
            return bad("the " + CardProblem::fieldNamed("servedMaterials") + " line needs at least one " +
                           std::string(CardProblem::FIELD),
                       "matIds");
        if (card.materials->youngsModulus &&
            !(*card.materials->youngsModulus > 0.0 && std::isfinite(*card.materials->youngsModulus)))
            return bad(std::string(CardProblem::FIELD) + " must be greater than zero", "youngsModulus");
        if (!inRange(card.materials->poissonRatio, 0.25, 0.35))
            return bad(std::string(CardProblem::FIELD) + " must be from 0.25 to 0.35", "poissonRatio");
        for (const FatigueMatIdRun& run : card.materials->matIds)
        {
            if (run.first <= 0 || run.last < run.first)
                return CardProblem{"card.bad-id",
                                   "a " + std::string(CardProblem::FIELD) +
                                       " must be a positive ID, and a THRU range must not run backwards",
                                   "matIds"};
        }
    }
    return std::nullopt;
}

void FatigueMatFormat::write(const FatigueMatCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.realUnlessDefault(2, card.stressUnitFactor, 1.0);
    writer.integerUnlessDefault(3, card.debugOutput, 0);
    int position = FIRST_BLOCK_FIELD;
    const FatigueMatStatic& fixed = card.fixed;
    writer.text(position, "Static");
    if (fixed.yieldStrength)
        writer.real(position + 1, *fixed.yieldStrength);
    if (fixed.ultimateTensileStrength)
        writer.real(position + 2, *fixed.ultimateTensileStrength);
    if (fixed.materialCode)
        writer.integer(position + 3, *fixed.materialCode);
    if (!fixed.curveKind.empty())
        writer.text(position + 4, fixed.curveKind);
    if (fixed.testStressRatio)
        writer.real(position + 5, *fixed.testStressRatio);
    if (fixed.standardError)
        writer.real(position + 6, *fixed.standardError);
    if (fixed.plasticPoissonRatio)
        writer.real(position + 7, *fixed.plasticPoissonRatio);
    position += LINE;
    if (fixed.areaReduction)
    {
        writer.real(position + 1, *fixed.areaReduction);
        position += LINE;
    }
    for (std::size_t index = 0; index < FatigueMatCard::CURVES; ++index)
    {
        if (!card.curves[index])
            continue;
        const FatigueMatCurve& curve = *card.curves[index];
        writer.text(position, CURVE_KEYWORDS[index]);
        writeValues(writer, position + 1, curve.curve);
        // The optional lines are written as far as the last one that has a value.
        const bool spot2 = anyGiven(curve.spotWeld, 6, 6);
        const bool spot1 = spot2 || anyGiven(curve.spotWeld, 0, 6);
        const bool mean = spot1 || anyGiven(curve.meanStress);
        position += LINE;
        if (mean)
        {
            writeValues(writer, position + 1, curve.meanStress);
            position += LINE;
        }
        if (spot1)
        {
            writeValues(writer, position + 1, curve.spotWeld, 0, 6);
            position += LINE;
        }
        if (spot2)
        {
            writeValues(writer, position + 1, curve.spotWeld, 6, 6);
            position += LINE;
        }
    }
    if (card.table)
    {
        writer.text(position, "Table");
        for (std::size_t index = 0; index < card.table->size(); ++index)
        {
            const int line = position + LINE * static_cast<int>(index / 3);
            const int field = line + 1 + 2 * static_cast<int>(index % 3);
            writer.real(field, (*card.table)[index].first);
            writer.integer(field + 1, (*card.table)[index].second);
        }
        position += LINE * static_cast<int>(card.table->empty() ? 1 : (card.table->size() + 2) / 3);
    }
    if (card.handbookCoefficients)
    {
        writer.text(position, "MetallicHandbookData");
        writeValues(writer, position + 1, *card.handbookCoefficients);
        position += LINE;
    }
    if (card.strainLife)
    {
        writer.text(position, "StrainLifeMethod");
        writeValues(writer, position + 1, card.strainLife->curve);
        position += LINE;
        const auto& errors = card.strainLife->errors;
        if (anyGiven(errors))
        {
            for (std::size_t index = 0; index < 4; ++index)
            {
                if (errors[index])
                    writer.real(position + 1 + static_cast<int>(index), *errors[index]);
            }
            if (errors[4])
                writer.real(position + 7, *errors[4]);
            position += LINE;
        }
    }
    if (card.materials)
    {
        const FatigueMatMaterials& materials = *card.materials;
        writer.text(position, "MatIds");
        if (materials.youngsModulus)
            writer.real(position + 1, *materials.youngsModulus);
        if (materials.poissonRatio)
            writer.real(position + 2, *materials.poissonRatio);
        position += LINE;
        int token = 0;
        const auto put = [&](auto&& write)
        {
            write(position + LINE * (token / 7) + 1 + token % 7);
            ++token;
        };
        for (const FatigueMatIdRun& run : materials.matIds)
        {
            put([&](int field) { writer.integer(field, run.first); });
            if (run.last != run.first)
            {
                put([&](int field) { writer.text(field, "Through"); });
                put([&](int field) { writer.integer(field, run.last); });
            }
        }
    }
}

std::uint32_t FatigueMatTable::append(const FatigueMatCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void FatigueMatTable::store(std::uint32_t row, const FatigueMatCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void FatigueMatTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void FatigueMatTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void FatigueMatTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    FatigueMatCard& card = m_cards.edit(row);
    if (!card.materials)
    {
        // The ID is the one of the MAT1 that the card serves.
        card.id = visitor.visit("id", ReferenceTarget::Material, card.id);
        m_id.set(row, card.id);
        return;
    }
    // With a MATID line the ID is the card's own and the single MIDi are the MAT1 that it serves.
    for (FatigueMatIdRun& run : card.materials->matIds)
    {
        if (run.first == run.last)
        {
            run.first = visitor.visit("matIds", ReferenceTarget::Material, run.first);
            run.last = run.first;
        }
    }
}

} // namespace oor::model
