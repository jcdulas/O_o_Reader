#include "oor/model/cards/nodes/adaptiveRefineCriterion/AdaptiveRefineCriterionTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <array>

namespace oor::model
{

namespace
{

// The parameters are fields 3 to 8, after the CRITID and the TYPE.
constexpr int FIRST_PARAMETER_FIELD = 3;
constexpr int LAST_FIELD = 8;
constexpr std::int32_t FIRST_TYPE = 1;
constexpr std::int32_t LAST_TYPE = 4;
constexpr std::int32_t ERROR_INDICATOR_TYPE = 1;

constexpr std::array<const char*, ADAPTIVE_CRITERION_PARAMETER_COUNT> PARAMETER_NAMES{
    "parameter1", "parameter2", "parameter3", "parameter4", "parameter5", "parameter6"};

} // namespace

std::optional<AdaptiveRefineCriterionCard> AdaptiveRefineCriterionFormat::read(FieldReader& reader)
{
    AdaptiveRefineCriterionCard card;
    card.id = reader.requiredInteger(1, "id");
    card.kind = reader.integer(2, "kind", FIRST_TYPE);
    for (std::size_t index = 0; index < ADAPTIVE_CRITERION_PARAMETER_COUNT; ++index)
        card.parameters[index] =
            reader.optionalReal(FIRST_PARAMETER_FIELD + static_cast<int>(index), PARAMETER_NAMES[index]);
    if (reader.lastUsedField() > LAST_FIELD)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 8 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AdaptiveRefineCriterionFormat::check(const AdaptiveRefineCriterionCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.kind < FIRST_TYPE || card.kind > LAST_TYPE)
        return CardProblem{"card.bad-value", "{field} must be 1, 2, 3 or 4", "kind"};
    for (std::size_t index = 0; index < ADAPTIVE_CRITERION_PARAMETER_COUNT; ++index)
    {
        if (const auto problem = checkOptionalReal(card.parameters[index], PARAMETER_NAMES[index], RealRule::Finite))
            return problem;
    }
    // The weight of the error indicator criterion; for the other types F1 is a coordinate.
    if (card.kind == ERROR_INDICATOR_TYPE && card.parameters[0])
    {
        if (const auto problem = checkRealRange(*card.parameters[0], "parameter1", 0.0, 1.0))
            return problem;
    }
    return std::nullopt;
}

void AdaptiveRefineCriterionFormat::write(const AdaptiveRefineCriterionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.kind, FIRST_TYPE);
    for (std::size_t index = 0; index < ADAPTIVE_CRITERION_PARAMETER_COUNT; ++index)
    {
        if (card.parameters[index])
            writer.real(FIRST_PARAMETER_FIELD + static_cast<int>(index), *card.parameters[index]);
    }
}

std::uint32_t AdaptiveRefineCriterionTable::append(const AdaptiveRefineCriterionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void AdaptiveRefineCriterionTable::store(std::uint32_t row, const AdaptiveRefineCriterionCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void AdaptiveRefineCriterionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void AdaptiveRefineCriterionTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
