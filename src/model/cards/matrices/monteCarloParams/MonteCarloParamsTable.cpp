#include "oor/model/cards/matrices/monteCarloParams/MonteCarloParamsTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <cmath>

namespace oor::model
{

namespace
{

constexpr int FIELDS_PER_LINE = 8;
constexpr int VARIABILITY_TYPE_FIELD = 2;
constexpr int FIRST_GROUP_LINE = 1;
constexpr std::string_view STRUCTURAL_WORD = "StructuralVariation";
constexpr std::string_view FLUID_WORD = "FluidVariation";

// The three values of a line after its keyword: mass, stiffness and damping, each 0.0 when blank. `names` are the
// format reference names of the fields for the diagnostics (literals).
MonteCarloParamsVariabilities readVariabilities(FieldReader& reader, int lineBase,
                                                const std::array<const char*, 3>& names)
{
    MonteCarloParamsVariabilities values;
    values.mass = reader.real(lineBase + 2, names[0], 0.0);
    values.stiffness = reader.real(lineBase + 3, names[1], 0.0);
    values.damping = reader.real(lineBase + 4, names[2], 0.0);
    return values;
}

void writeVariabilities(FieldWriter& writer, int lineBase, std::string_view keyword,
                        const MonteCarloParamsVariabilities& values)
{
    writer.text(lineBase + 1, keyword);
    writer.realUnlessDefault(lineBase + 2, values.mass, 0.0);
    writer.realUnlessDefault(lineBase + 3, values.stiffness, 0.0);
    writer.realUnlessDefault(lineBase + 4, values.damping, 0.0);
}

std::optional<CardProblem> checkVariabilities(const MonteCarloParamsVariabilities& values, const char* mass,
                                              const char* stiffness, const char* damping)
{
    if (const auto problem = checkReal(values.mass, mass, RealRule::NonNegative))
        return problem;
    if (const auto problem = checkReal(values.stiffness, stiffness, RealRule::NonNegative))
        return problem;
    return checkReal(values.damping, damping, RealRule::NonNegative);
}

} // namespace

std::optional<MonteCarloParamsCard> MonteCarloParamsFormat::read(FieldReader& reader)
{
    MonteCarloParamsCard card;
    card.id = reader.requiredInteger(1, "id");
    card.type = readKeyword(reader, VARIABILITY_TYPE_FIELD, "variabilityType", MONTE_CARLO_TYPE_WORDS,
                            MonteCarloParamsVariabilityType::Alpha);
    for (int field = VARIABILITY_TYPE_FIELD + 1; field <= FIELDS_PER_LINE; ++field)
    {
        if (!reader.isBlank(field))
        {
            reader.error("card.bad-value",
                         "the first line of a " + std::string(reader.cardName()) + " has no field beyond " +
                             std::string("variabilityType"),
                         "variabilityType");
            break;
        }
    }
    const int last = reader.lastUsedField();
    for (int line = FIRST_GROUP_LINE; line * FIELDS_PER_LINE < last && !reader.failed(); ++line)
    {
        const int lineBase = line * FIELDS_PER_LINE;
        if (reader.isBlank(lineBase + 1))
        {
            // A line of blank fields is padding; anything else needs the keyword that says what the values are.
            bool blankLine = true;
            for (int field = lineBase + 2; field <= lineBase + FIELDS_PER_LINE; ++field)
                blankLine = blankLine && reader.isBlank(field);
            if (!blankLine)
                reader.error("card.missing-field",
                             "a line of a " + std::string(reader.cardName()) + " starts with STRVAR or FLUVAR",
                             "structuralVariability");
            continue;
        }
        const std::string_view word = reader.text(lineBase + 1);
        const bool structural = (word == STRUCTURAL_WORD);
        if (!structural && !(word == FLUID_WORD))
        {
            reader.error("card.bad-value",
                         "a line of a " + std::string(reader.cardName()) + " starts with STRVAR or FLUVAR, not \"" +
                             std::string(word) + "\"",
                         "structuralVariability");
            break;
        }
        std::optional<MonteCarloParamsVariabilities>& group = structural ? card.structural : card.fluid;
        if (group)
        {
            reader.error("card.bad-value", std::string(word) + " appears twice in a " + std::string(reader.cardName()),
                         "structuralVariability");
            break;
        }
        group =
            structural
                ? readVariabilities(
                      reader, lineBase,
                      {"structuralMassVariability", "structuralStiffnessVariability", "structuralDampingVariability"})
                : readVariabilities(reader, lineBase,
                                    {"fluidMassVariability", "fluidStiffnessVariability", "fluidDampingVariability"});
        for (int field = lineBase + 5; field <= lineBase + FIELDS_PER_LINE; ++field)
        {
            if (!reader.isBlank(field))
            {
                reader.error(
                    "card.bad-value",
                    "a line of a " + std::string(reader.cardName()) + " has no field after the three variabilities",
                    structural ? "structuralDampingVariability" : "fluidDampingVariability");
                break;
            }
        }
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> MonteCarloParamsFormat::check(const MonteCarloParamsCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.structural)
    {
        if (const auto problem = checkVariabilities(*card.structural, "structuralMassVariability",
                                                    "structuralStiffnessVariability", "structuralDampingVariability"))
            return problem;
    }
    if (card.fluid)
    {
        if (const auto problem = checkVariabilities(*card.fluid, "fluidMassVariability", "fluidStiffnessVariability",
                                                    "fluidDampingVariability"))
            return problem;
    }
    return std::nullopt;
}

void MonteCarloParamsFormat::write(const MonteCarloParamsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeKeywordUnlessDefault(writer, VARIABILITY_TYPE_FIELD, MONTE_CARLO_TYPE_WORDS, card.type,
                              MonteCarloParamsVariabilityType::Alpha);
    int line = FIRST_GROUP_LINE;
    if (card.structural)
        writeVariabilities(writer, line++ * FIELDS_PER_LINE, STRUCTURAL_WORD, *card.structural);
    if (card.fluid)
        writeVariabilities(writer, line++ * FIELDS_PER_LINE, FLUID_WORD, *card.fluid);
}

std::uint32_t MonteCarloParamsTable::append(const MonteCarloParamsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void MonteCarloParamsTable::store(std::uint32_t row, const MonteCarloParamsCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void MonteCarloParamsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void MonteCarloParamsTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
