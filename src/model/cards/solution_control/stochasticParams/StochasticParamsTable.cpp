#include "oor/model/cards/solution_control/stochasticParams/StochasticParamsTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIELDS_PER_LINE = 8;
// The offsets of a group in its line: the field before the flag is the SID on the first line and blank on the others.
constexpr int FLAG_OFFSET = 2;
constexpr int DISTRIBUTION_OFFSET = 3;
constexpr int VARIATION_COEFFICIENT_OFFSET = 4;
constexpr int STANDARD_DEVIATIONS_OFFSET = 5;

std::optional<CardProblem> checkDistribution(const std::string& name)
{
    return name.empty() ? std::nullopt : checkNameWord(name, "distribution");
}

// The flag in a message: the one that is a card by a placeholder, which the reader turns into its keyword.
std::string flagText(StochasticParamsFlag flag)
{
    const std::string_view word = keywordText(STOCHASTIC_PARAMS_FLAG_WORDS, flag);
    return flag == StochasticParamsFlag::Spcd ? CardProblem::cardNamed(word) : std::string(word);
}

} // namespace

std::optional<StochasticParamsCard> StochasticParamsFormat::read(FieldReader& reader)
{
    StochasticParamsCard card;
    card.id = reader.requiredInteger(1, "id");
    const int last = reader.lastUsedField();
    for (int line = 0; line * FIELDS_PER_LINE < last && !reader.failed(); ++line)
    {
        const int lineBase = line * FIELDS_PER_LINE;
        // Every field of the line that is not part of the group is blank (the SID is the first field of the first
        // line).
        for (int offset = 1; offset <= FIELDS_PER_LINE; ++offset)
        {
            const bool inGroup = offset >= FLAG_OFFSET && offset <= STANDARD_DEVIATIONS_OFFSET;
            if (inGroup || (line == 0 && offset == 1) || reader.isBlank(lineBase + offset))
                continue;
            reader.error("card.bad-value",
                         "field " + std::to_string(lineBase + offset + 1) + " of a " + std::string(reader.cardName()) +
                             " must be blank",
                         "flag");
            break;
        }
        if (reader.failed())
            break;
        if (reader.isBlank(lineBase + FLAG_OFFSET))
        {
            if (!reader.isBlank(lineBase + DISTRIBUTION_OFFSET) ||
                !reader.isBlank(lineBase + VARIATION_COEFFICIENT_OFFSET) ||
                !reader.isBlank(lineBase + STANDARD_DEVIATIONS_OFFSET))
                reader.error("card.missing-field",
                             std::string("distribution") + ", " + std::string("variationCoefficient") + " and " +
                                 std::string("standardDeviations") + " need the flag of their line",
                             "flag");
            continue; // a line without a group is padding
        }
        StochasticParamsGroup group;
        group.flag = readRequiredKeyword<StochasticParamsFlag>(reader, lineBase + FLAG_OFFSET, "flag",
                                                               STOCHASTIC_PARAMS_FLAG_WORDS);
        group.distribution = std::string(reader.text(lineBase + DISTRIBUTION_OFFSET));
        group.variationCoefficient = reader.real(lineBase + VARIATION_COEFFICIENT_OFFSET, "variationCoefficient",
                                                 STOCHASTIC_PARAMS_DEFAULT_VARIATION_COEFFICIENT);
        group.standardDeviations = reader.real(lineBase + STANDARD_DEVIATIONS_OFFSET, "standardDeviations",
                                               STOCHASTIC_PARAMS_DEFAULT_STANDARD_DEVIATIONS);
        card.groups.push_back(std::move(group));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> StochasticParamsFormat::check(const StochasticParamsCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.groups.empty())
        return CardProblem{"card.missing-field",
                           "at least one flag is required (PENTRY, MENTRY, CENTRY, LOADS or " +
                               CardProblem::cardNamed("EnforcedMotionConstr") + ")",
                           "flag"};
    for (std::size_t index = 0; index < card.groups.size(); ++index)
    {
        const StochasticParamsGroup& group = card.groups[index];
        for (std::size_t earlier = 0; earlier < index; ++earlier)
        {
            if (card.groups[earlier].flag == group.flag)
                return CardProblem{"card.bad-value", flagText(group.flag) + " appears twice", "flag"};
        }
        if (const auto problem = checkDistribution(group.distribution))
            return problem;
        if (const auto problem = checkReal(group.variationCoefficient, "variationCoefficient", RealRule::Positive))
            return problem;
        if (const auto problem = checkReal(group.standardDeviations, "standardDeviations", RealRule::Positive))
            return problem;
        // The range mean +- m standard deviations must not reach zero.
        if (!(group.standardDeviations * group.variationCoefficient < 1.0))
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " times " +
                                   CardProblem::fieldNamed("variationCoefficient") + " must be less than 1.0",
                               "standardDeviations"};
    }
    return std::nullopt;
}

void StochasticParamsFormat::write(const StochasticParamsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    int line = 0;
    for (const StochasticParamsGroup& group : card.groups)
    {
        const int lineBase = line * FIELDS_PER_LINE;
        writer.text(lineBase + FLAG_OFFSET, keywordText(STOCHASTIC_PARAMS_FLAG_WORDS, group.flag));
        writer.text(lineBase + DISTRIBUTION_OFFSET, group.distribution);
        writer.realUnlessDefault(lineBase + VARIATION_COEFFICIENT_OFFSET, group.variationCoefficient,
                                 STOCHASTIC_PARAMS_DEFAULT_VARIATION_COEFFICIENT);
        writer.realUnlessDefault(lineBase + STANDARD_DEVIATIONS_OFFSET, group.standardDeviations,
                                 STOCHASTIC_PARAMS_DEFAULT_STANDARD_DEVIATIONS);
        ++line;
    }
}

std::uint32_t StochasticParamsTable::append(const StochasticParamsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void StochasticParamsTable::store(std::uint32_t row, const StochasticParamsCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void StochasticParamsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void StochasticParamsTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
