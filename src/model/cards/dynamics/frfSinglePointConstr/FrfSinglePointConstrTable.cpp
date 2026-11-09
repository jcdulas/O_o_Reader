#include "oor/model/cards/dynamics/frfSinglePointConstr/FrfSinglePointConstrTable.hpp"

#include "oor/base/text/DigitSets.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <array>

namespace oor::model
{

namespace
{

constexpr std::array<FrfPointNames, 3> GROUP_NAMES{
    {{"frfComponent1", "point1"}, {"frfComponent2", "point2"}, {"frfComponent3", "point3"}}};

constexpr int FIRST_GROUP_FIELD = 3; // the groups are fields 3-4, 5-6 and 7-8
constexpr int LAST_FIELD = 8;

} // namespace

std::optional<FrfSinglePointConstrCard> FrfSinglePointConstrFormat::read(FieldReader& reader)
{
    FrfSinglePointConstrCard card;
    card.id = reader.requiredInteger(1, "id");
    card.components = base::sortedDigits(reader.integer(2, "components", 0));
    for (std::size_t group = 0; group < card.points.size(); ++group)
        card.points[group] =
            readFrfComponentPoint(reader, FIRST_GROUP_FIELD + 2 * static_cast<int>(group), GROUP_NAMES[group]);
    if (reader.lastUsedField() > LAST_FIELD)
        reader.error("card.bad-value", "the card has no field beyond " + std::string("point3"), "point3");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FrfSinglePointConstrFormat::check(const FrfSinglePointConstrCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (!base::isComponentSet(card.components))
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must be 0 or a list of the digits 1 to 6, each at most once",
            "components"};
    for (std::size_t group = 0; group < card.points.size(); ++group)
    {
        if (const auto problem = checkFrfComponentPoint(card.points[group], GROUP_NAMES[group], group == 0))
            return problem;
    }
    return std::nullopt;
}

void FrfSinglePointConstrFormat::write(const FrfSinglePointConstrCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.components, 0);
    for (std::size_t group = 0; group < card.points.size(); ++group)
    {
        if (!card.points[group].isBlank())
            writeFrfComponentPoint(writer, FIRST_GROUP_FIELD + 2 * static_cast<int>(group), card.points[group]);
    }
}

std::uint32_t FrfSinglePointConstrTable::append(const FrfSinglePointConstrCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_components.push(card.components);
    m_cards.push(card);
    return row;
}

void FrfSinglePointConstrTable::store(std::uint32_t row, const FrfSinglePointConstrCard& card)
{
    m_id.set(row, card.id);
    m_components.set(row, card.components);
    m_cards.set(row, card);
}

void FrfSinglePointConstrTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_components.keepRows(rows);
    m_cards.keepRows(rows);
}

void FrfSinglePointConstrTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void FrfSinglePointConstrTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    FrfSinglePointConstrCard& card = m_cards.edit(row);
    visitFrfComponent(visitor, "frfComponent1", card.points[0].component);
    visitFrfComponent(visitor, "frfComponent2", card.points[1].component);
    visitFrfComponent(visitor, "frfComponent3", card.points[2].component);
}

} // namespace oor::model
