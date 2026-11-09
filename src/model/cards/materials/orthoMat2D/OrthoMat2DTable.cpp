#include "oor/model/cards/materials/orthoMat2D/OrthoMat2DTable.hpp"

#include "oor/base/text/TextUtilities.hpp"

#include <cmath>
#include <limits>

namespace oor::model
{

namespace
{

constexpr int HFAIL_FIELD = 25; // the lines of the three optional groups are at these fields
constexpr int HTAPE_FIELD = 33;
constexpr int HFABR_FIELD = 49;
constexpr int LAST_FIELD = 59;
constexpr std::uint8_t HFAIL_BIT = 1;
constexpr std::uint8_t HTAPE_BIT = 2;
constexpr std::uint8_t HFABR_BIT = 4;

// One value of a group of Hashin criteria: where it sits behind the keyword, and what it is when blank.
struct GroupValue
{
    int offset;
    const char* name;
    int blank; // the index of the earlier value a blank one repeats, or REQUIRED, or ZERO
};
constexpr int REQUIRED = -1;
constexpr int ZERO = -2;

constexpr GroupValue HFAIL_VALUES[6] = {{1, "hashinUnidirectional1", REQUIRED},  {2, "hashinUnidirectional2", 0},
                                        {3, "hashinUnidirectional3", REQUIRED},  {4, "hashinUnidirectional4", 2},
                                        {5, "hashinUnidirectional10", REQUIRED}, {6, "hashinUnidirectional11", 4}};
constexpr GroupValue HTAPE_VALUES[9] = {
    {1, "hashinTape1", REQUIRED},  {2, "hashinTape2", 0},    {3, "hashinTape3", REQUIRED},
    {4, "hashinTape4", 2},         {5, "hashinTape5", ZERO}, {6, "hashinTape6", ZERO},
    {7, "hashinTape10", REQUIRED}, {9, "hashinTape11", 6},   {10, "hashinTape12", 7}};
constexpr GroupValue HFABR_VALUES[9] = {
    {1, "hashinFabric1", REQUIRED},  {2, "hashinFabric2", 0},        {3, "hashinFabric3", REQUIRED},
    {4, "hashinFabric4", 2},         {5, "hashinFabric5", REQUIRED}, {6, "hashinFabric6", 4},
    {7, "hashinFabric10", REQUIRED}, {9, "hashinFabric11", 6},       {10, "hashinFabric12", 7}};

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

template <std::size_t N>
std::array<double, N> readGroup(FieldReader& reader, int keywordField, const GroupValue (&layout)[N],
                                std::array<bool, LAST_FIELD + 1>& used)
{
    std::array<double, N> values = {};
    used[static_cast<std::size_t>(keywordField)] = true;
    for (std::size_t index = 0; index < N; ++index)
    {
        const int field = keywordField + layout[index].offset;
        used[static_cast<std::size_t>(field)] = true;
        if (layout[index].blank == REQUIRED)
            values[index] = reader.requiredReal(field, layout[index].name);
        else if (layout[index].blank == ZERO)
            values[index] = reader.real(field, layout[index].name, 0.0);
        else
            values[index] =
                reader.real(field, layout[index].name, values[static_cast<std::size_t>(layout[index].blank)]);
    }
    return values;
}

template <std::size_t N>
void writeGroup(FieldWriter& writer, int keywordField, const char* keyword, const GroupValue (&layout)[N],
                const std::array<double, N>& values)
{
    writer.text(keywordField, keyword);
    for (std::size_t index = 0; index < N; ++index)
    {
        // A value that repeats an earlier one when blank is always written, so that it does not follow that one.
        if (layout[index].blank == ZERO)
            writer.realUnlessDefault(keywordField + layout[index].offset, values[index], 0.0);
        else
            writer.real(keywordField + layout[index].offset, values[index]);
    }
}

} // namespace

std::optional<OrthoMat2DCard> OrthoMat2DFormat::read(FieldReader& reader)
{
    OrthoMat2DCard card;
    card.id = reader.requiredInteger(1, "id");
    card.youngsModulus1 = reader.requiredReal(2, "youngsModulus1");
    card.youngsModulus2 = reader.requiredReal(3, "youngsModulus2");
    card.poissonRatio12 = reader.real(4, "poissonRatio12", 0.0);
    card.shearModulus12 = reader.real(5, "shearModulus12", 0.0);
    card.shearModulus1Z = reader.optionalReal(6, "shearModulus1Z");
    card.shearModulus2Z = reader.optionalReal(7, "shearModulus2Z");
    card.density = reader.real(8, "density", 0.0);
    card.thermalExpansion1 = reader.real(9, "thermalExpansion1", 0.0);
    card.thermalExpansion2 = reader.real(10, "thermalExpansion2", 0.0);
    card.referenceTemperature = reader.real(11, "referenceTemperature", 0.0);
    card.tensionStrength1 = reader.real(12, "tensionStrength1", 0.0);
    card.compressionStrength1 =
        reader.real(13, "compressionStrength1", card.tensionStrength1); // format reference: Xc defaults to Xt, Yc to Yt
    card.tensionStrength2 = reader.real(14, "tensionStrength2", 0.0);
    card.compressionStrength2 = reader.real(15, "compressionStrength2", card.tensionStrength2);
    card.shearStrength = reader.real(16, "shearStrength", 0.0);
    card.structuralDamping = reader.real(17, "structuralDamping", 0.0);
    card.interactionTerm12 = reader.real(18, "interactionTerm12", 0.0);
    card.strainAllowables = reader.real(19, "strainAllowables", 0.0);

    // The optional groups, each behind its keyword at its own line; nothing else may follow field 19.
    std::array<bool, LAST_FIELD + 1> used = {};
    if ((reader.text(HFAIL_FIELD) == "HashinCriterion"))
        card.hashinUnidirectional = readGroup(reader, HFAIL_FIELD, HFAIL_VALUES, used);
    if ((reader.text(HTAPE_FIELD) == "HashinTape"))
        card.hashinTape = readGroup(reader, HTAPE_FIELD, HTAPE_VALUES, used);
    if ((reader.text(HFABR_FIELD) == "HashinFabricFailure"))
        card.hashinFabric = readGroup(reader, HFABR_FIELD, HFABR_VALUES, used);
    bool stray = reader.lastUsedField() > LAST_FIELD;
    for (int field = 20; field <= LAST_FIELD && !stray; ++field)
        stray = !used[static_cast<std::size_t>(field)] && !reader.isBlank(field);
    if (stray)
        reader.error("card.bad-value", "fields after field 19 of a " + std::string(reader.cardName()) +
                                           " are the groups HFAIL (field 25), HTAPE (field 33) and HFABR "
                                           "(field 49) only");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> OrthoMat2DFormat::check(const OrthoMat2DCard& card)
{
    if (card.id <= 0 || card.id >= 100000000)
        return CardProblem{"card.bad-id", "the material ID must be an integer from 1 to 99999999", "id"};
    if (!(card.youngsModulus1 > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "youngsModulus1"};
    if (!(card.youngsModulus2 > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "youngsModulus2"};
    if (card.shearModulus12 < 0.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "shearModulus12"};
    if (card.shearModulus1Z && !(*card.shearModulus1Z >= 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "shearModulus1Z"};
    if (card.shearModulus2Z && !(*card.shearModulus2Z >= 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "shearModulus2Z"};
    return std::nullopt;
}

void OrthoMat2DFormat::write(const OrthoMat2DCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.youngsModulus1);
    writer.real(3, card.youngsModulus2);
    writer.real(4, card.poissonRatio12);
    writer.realUnlessDefault(5, card.shearModulus12, 0.0);
    if (card.shearModulus1Z)
        writer.real(6, *card.shearModulus1Z);
    if (card.shearModulus2Z)
        writer.real(7, *card.shearModulus2Z);
    writer.realUnlessDefault(8, card.density, 0.0);
    writer.realUnlessDefault(9, card.thermalExpansion1, 0.0);
    writer.realUnlessDefault(10, card.thermalExpansion2, 0.0);
    writer.realUnlessDefault(11, card.referenceTemperature, 0.0);
    writer.realUnlessDefault(12, card.tensionStrength1, 0.0);
    writer.real(13, card.compressionStrength1); // depends on Xt: always written
    writer.realUnlessDefault(14, card.tensionStrength2, 0.0);
    writer.real(15, card.compressionStrength2); // depends on Yt: always written
    writer.realUnlessDefault(16, card.shearStrength, 0.0);
    writer.realUnlessDefault(17, card.structuralDamping, 0.0);
    writer.realUnlessDefault(18, card.interactionTerm12, 0.0);
    writer.realUnlessDefault(19, card.strainAllowables, 0.0);
    if (card.hashinUnidirectional)
        writeGroup(writer, HFAIL_FIELD, "HashinCriterion", HFAIL_VALUES, *card.hashinUnidirectional);
    if (card.hashinTape)
        writeGroup(writer, HTAPE_FIELD, "HashinTape", HTAPE_VALUES, *card.hashinTape);
    if (card.hashinFabric)
        writeGroup(writer, HFABR_FIELD, "HashinFabricFailure", HFABR_VALUES, *card.hashinFabric);
}

std::uint32_t OrthoMat2DTable::append(const OrthoMat2DCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_youngsModulus1.push(card.youngsModulus1);
    m_youngsModulus2.push(card.youngsModulus2);
    m_poissonRatio12.push(card.poissonRatio12);
    m_shearModulus12.push(card.shearModulus12);
    m_shearModulus1Z.push(blankIfNone(card.shearModulus1Z));
    m_shearModulus2Z.push(blankIfNone(card.shearModulus2Z));
    m_density.push(card.density);
    m_thermalExpansion1.push(card.thermalExpansion1);
    m_thermalExpansion2.push(card.thermalExpansion2);
    m_referenceTemperature.push(card.referenceTemperature);
    m_tensionStrength1.push(card.tensionStrength1);
    m_compressionStrength1.push(card.compressionStrength1);
    m_tensionStrength2.push(card.tensionStrength2);
    m_compressionStrength2.push(card.compressionStrength2);
    m_shearStrength.push(card.shearStrength);
    m_structuralDamping.push(card.structuralDamping);
    m_interactionTerm12.push(card.interactionTerm12);
    m_strainAllowables.push(card.strainAllowables);
    m_hashin.push(static_cast<std::uint8_t>((card.hashinUnidirectional ? HFAIL_BIT : 0) |
                                            (card.hashinTape ? HTAPE_BIT : 0) | (card.hashinFabric ? HFABR_BIT : 0)));
    m_hashinUnidirectional.push(card.hashinUnidirectional.value_or(std::array<double, 6>{}));
    m_hashinTape.push(card.hashinTape.value_or(std::array<double, 9>{}));
    m_hashinFabric.push(card.hashinFabric.value_or(std::array<double, 9>{}));
    return row;
}

OrthoMat2DCard OrthoMat2DTable::card(std::uint32_t row) const
{
    OrthoMat2DCard card;
    card.id = m_id.get(row);
    card.youngsModulus1 = m_youngsModulus1.get(row);
    card.youngsModulus2 = m_youngsModulus2.get(row);
    card.poissonRatio12 = m_poissonRatio12.get(row);
    card.shearModulus12 = m_shearModulus12.get(row);
    card.shearModulus1Z = noneIfBlank(m_shearModulus1Z.get(row));
    card.shearModulus2Z = noneIfBlank(m_shearModulus2Z.get(row));
    card.density = m_density.get(row);
    card.thermalExpansion1 = m_thermalExpansion1.get(row);
    card.thermalExpansion2 = m_thermalExpansion2.get(row);
    card.referenceTemperature = m_referenceTemperature.get(row);
    card.tensionStrength1 = m_tensionStrength1.get(row);
    card.compressionStrength1 = m_compressionStrength1.get(row);
    card.tensionStrength2 = m_tensionStrength2.get(row);
    card.compressionStrength2 = m_compressionStrength2.get(row);
    card.shearStrength = m_shearStrength.get(row);
    card.structuralDamping = m_structuralDamping.get(row);
    card.interactionTerm12 = m_interactionTerm12.get(row);
    card.strainAllowables = m_strainAllowables.get(row);
    const std::uint8_t groups = m_hashin.get(row);
    if ((groups & HFAIL_BIT) != 0)
        card.hashinUnidirectional = m_hashinUnidirectional.get(row);
    if ((groups & HTAPE_BIT) != 0)
        card.hashinTape = m_hashinTape.get(row);
    if ((groups & HFABR_BIT) != 0)
        card.hashinFabric = m_hashinFabric.get(row);
    return card;
}

void OrthoMat2DTable::store(std::uint32_t row, const OrthoMat2DCard& card)
{
    m_id.set(row, card.id);
    m_youngsModulus1.set(row, card.youngsModulus1);
    m_youngsModulus2.set(row, card.youngsModulus2);
    m_poissonRatio12.set(row, card.poissonRatio12);
    m_shearModulus12.set(row, card.shearModulus12);
    m_shearModulus1Z.set(row, blankIfNone(card.shearModulus1Z));
    m_shearModulus2Z.set(row, blankIfNone(card.shearModulus2Z));
    m_density.set(row, card.density);
    m_thermalExpansion1.set(row, card.thermalExpansion1);
    m_thermalExpansion2.set(row, card.thermalExpansion2);
    m_referenceTemperature.set(row, card.referenceTemperature);
    m_tensionStrength1.set(row, card.tensionStrength1);
    m_compressionStrength1.set(row, card.compressionStrength1);
    m_tensionStrength2.set(row, card.tensionStrength2);
    m_compressionStrength2.set(row, card.compressionStrength2);
    m_shearStrength.set(row, card.shearStrength);
    m_structuralDamping.set(row, card.structuralDamping);
    m_interactionTerm12.set(row, card.interactionTerm12);
    m_strainAllowables.set(row, card.strainAllowables);
    m_hashin.set(
        row, static_cast<std::uint8_t>((card.hashinUnidirectional ? HFAIL_BIT : 0) | (card.hashinTape ? HTAPE_BIT : 0) |
                                       (card.hashinFabric ? HFABR_BIT : 0)));
    m_hashinUnidirectional.set(row, card.hashinUnidirectional.value_or(std::array<double, 6>{}));
    m_hashinTape.set(row, card.hashinTape.value_or(std::array<double, 9>{}));
    m_hashinFabric.set(row, card.hashinFabric.value_or(std::array<double, 9>{}));
}

void OrthoMat2DTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_youngsModulus1.keepRows(rows);
    m_youngsModulus2.keepRows(rows);
    m_poissonRatio12.keepRows(rows);
    m_shearModulus12.keepRows(rows);
    m_shearModulus1Z.keepRows(rows);
    m_shearModulus2Z.keepRows(rows);
    m_density.keepRows(rows);
    m_thermalExpansion1.keepRows(rows);
    m_thermalExpansion2.keepRows(rows);
    m_referenceTemperature.keepRows(rows);
    m_tensionStrength1.keepRows(rows);
    m_compressionStrength1.keepRows(rows);
    m_tensionStrength2.keepRows(rows);
    m_compressionStrength2.keepRows(rows);
    m_shearStrength.keepRows(rows);
    m_structuralDamping.keepRows(rows);
    m_interactionTerm12.keepRows(rows);
    m_strainAllowables.keepRows(rows);
    m_hashin.keepRows(rows);
    m_hashinUnidirectional.keepRows(rows);
    m_hashinTape.keepRows(rows);
    m_hashinFabric.keepRows(rows);
}

void OrthoMat2DTable::visitReferences(std::uint32_t, ReferenceVisitor&)
{
    // A MAT8 refers to no other card (a MATT8 refers to it, not the other way round).
}

} // namespace oor::model
