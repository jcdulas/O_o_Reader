#include "oor/model/cards/superelements/axisymConeSupport/AxisymConeSupportTable.hpp"

#include "oor/base/text/DigitSets.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIELDS_PER_LINE = 8;
constexpr int DOFS_PER_LINE = 2;
constexpr int FIELDS_PER_DOF = 3;

// The first field of degree of freedom `index`: two to a line, from the first field of each line.
int firstFieldOf(std::size_t index) noexcept
{
    const int position = static_cast<int>(index);
    return 1 + FIELDS_PER_LINE * (position / DOFS_PER_LINE) + FIELDS_PER_DOF * (position % DOFS_PER_LINE);
}

} // namespace

std::optional<AxisymConeSupportCard> AxisymConeSupportFormat::read(FieldReader& reader)
{
    AxisymConeSupportCard card;
    const int last = reader.lastUsedField();
    for (std::size_t index = 0; firstFieldOf(index) <= last && !reader.failed(); ++index)
    {
        const int field = firstFieldOf(index);
        if (reader.isBlank(field) && reader.isBlank(field + 1) && reader.isBlank(field + 2))
            continue; // padding
        AxisymConeSupportDof dof;
        dof.ring = reader.requiredInteger(field, "ringId");
        dof.harmonic = reader.requiredInteger(field + 1, "harmonicId");
        dof.components = base::sortedDigits(reader.requiredInteger(field + 2, "components"));
        card.dofs.push_back(dof);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AxisymConeSupportFormat::check(const AxisymConeSupportCard& card)
{
    if (card.dofs.empty())
        return CardProblem{
            "card.missing-field",
            "the card needs at least one degree of freedom " + std::string(CardProblem::FIELD) + " HID1 C1", "ringId1"};
    if (card.dofs.size() > AxisymConeSupportCard::MAX_DOFS)
        return CardProblem{"card.bad-value", "the card has at most 12 degrees of freedom", "ringId"};
    for (const AxisymConeSupportDof& dof : card.dofs)
    {
        if (const auto problem = checkRequiredId(dof.ring, "ringId"))
            return problem;
        if (dof.harmonic < 0)
            return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must not be negative", "harmonicId"};
        if (dof.components <= 0 || !base::isComponentSet(dof.components))
            return CardProblem{
                "card.bad-value",
                std::string(CardProblem::FIELD) + " must be a list of the digits 1 to 6, each at most once",
                "components"};
    }
    return std::nullopt;
}

void AxisymConeSupportFormat::write(const AxisymConeSupportCard& card, FieldWriter& writer)
{
    std::size_t index = 0;
    for (const AxisymConeSupportDof& dof : card.dofs)
    {
        const int field = firstFieldOf(index++);
        writer.integer(field, dof.ring);
        writer.integer(field + 1, dof.harmonic);
        writer.integer(field + 2, dof.components);
    }
}

std::uint32_t AxisymConeSupportTable::append(const AxisymConeSupportCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_count.push(static_cast<std::int32_t>(card.dofs.size()));
    m_cards.push(card);
    return row;
}

void AxisymConeSupportTable::store(std::uint32_t row, const AxisymConeSupportCard& card)
{
    m_count.set(row, static_cast<std::int32_t>(card.dofs.size()));
    m_cards.set(row, card);
}

void AxisymConeSupportTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_count.keepRows(rows);
    m_cards.keepRows(rows);
}

} // namespace oor::model
