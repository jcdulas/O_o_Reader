#include "oor/model/cards/sets/axisymOmittedDofSet/AxisymOmittedDofSetTable.hpp"

#include "oor/base/text/DigitSets.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <string>

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

std::optional<AxisymOmittedDofSetCard> AxisymOmittedDofSetFormat::read(FieldReader& reader)
{
    AxisymOmittedDofSetCard card;
    const int last = reader.lastUsedField();
    for (std::size_t index = 0; firstFieldOf(index) <= last && !reader.failed(); ++index)
    {
        const int field = firstFieldOf(index);
        if (reader.isBlank(field) && reader.isBlank(field + 1) && reader.isBlank(field + 2))
            continue; // padding
        AxisymOmittedDofSetDof dof;
        dof.ring = reader.requiredInteger(field, "ringId");
        dof.harmonic = reader.requiredInteger(field + 1, "harmonicId");
        dof.components = base::sortedDigits(reader.requiredInteger(field + 2, "components"));
        card.dofs.push_back(dof);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AxisymOmittedDofSetFormat::check(const AxisymOmittedDofSetCard& card)
{
    if (card.dofs.empty())
        return CardProblem{"card.missing-field",
                           "the card needs at least one degree of freedom " + CardProblem::fieldNamed("ringId1") + " " +
                               CardProblem::fieldNamed("harmonicId1") + " " + CardProblem::fieldNamed("components1"),
                           "ringId1"};
    if (card.dofs.size() > AxisymOmittedDofSetCard::MAX_DOFS)
        return CardProblem{"card.bad-value", "the card has at most 12 degrees of freedom", "ringId"};
    for (const AxisymOmittedDofSetDof& dof : card.dofs)
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

void AxisymOmittedDofSetFormat::write(const AxisymOmittedDofSetCard& card, FieldWriter& writer)
{
    std::size_t index = 0;
    for (const AxisymOmittedDofSetDof& dof : card.dofs)
    {
        const int field = firstFieldOf(index++);
        writer.integer(field, dof.ring);
        writer.integer(field + 1, dof.harmonic);
        writer.integer(field + 2, dof.components);
    }
}

std::uint32_t AxisymOmittedDofSetTable::append(const AxisymOmittedDofSetCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_count.push(static_cast<std::int32_t>(card.dofs.size()));
    m_cards.push(card);
    return row;
}

void AxisymOmittedDofSetTable::store(std::uint32_t row, const AxisymOmittedDofSetCard& card)
{
    m_count.set(row, static_cast<std::int32_t>(card.dofs.size()));
    m_cards.set(row, card);
}

void AxisymOmittedDofSetTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_count.keepRows(rows);
    m_cards.keepRows(rows);
}

void AxisymOmittedDofSetTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    for (AxisymOmittedDofSetDof& dof : m_cards.edit(row).dofs)
        visitId(visitor, "ringId", ReferenceTarget::Node, dof.ring);
}

} // namespace oor::model
