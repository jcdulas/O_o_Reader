#include "oor/model/cards/rigid_elements/rigidInterpolationOverride/RigidInterpolationOverrideTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <string>

namespace oor::model
{

namespace
{

// A group is the first five fields of a line (the element and four reals); the three after them are blank.
constexpr int FIELDS_PER_LINE = 8;
constexpr int GROUP_FIELDS = 5;

} // namespace

std::optional<RigidInterpolationOverrideCard> RigidInterpolationOverrideFormat::read(FieldReader& reader)
{
    RigidInterpolationOverrideCard card;
    const int last = reader.lastUsedField();
    for (int start = 1; start <= last && !reader.failed(); start += FIELDS_PER_LINE)
    {
        bool used = false;
        for (int field = start; field < start + FIELDS_PER_LINE; ++field)
            used = used || !reader.isBlank(field);
        if (!used)
            continue; // a line with nothing on it
        for (int field = start + GROUP_FIELDS; field < start + FIELDS_PER_LINE; ++field)
        {
            if (!reader.isBlank(field))
                reader.error("card.bad-value",
                             "field " + std::to_string(field) + " of a " + std::string(reader.cardName()) +
                                 " is not used and must be blank",
                             "");
        }
        RigidInterpolationOverrideGroup group;
        group.elemId = reader.requiredInteger(start, "elemId");
        group.perturbationX = reader.real(start + 1, "perturbationX", RIGID_OVERRIDE_DEFAULT_PERTURBATION);
        group.perturbationY = reader.real(start + 2, "perturbationY", RIGID_OVERRIDE_DEFAULT_PERTURBATION);
        group.perturbationZ = reader.real(start + 3, "perturbationZ", RIGID_OVERRIDE_DEFAULT_PERTURBATION);
        group.toleranceAdjustment = reader.real(start + 4, "toleranceAdjustment", 0.0);
        card.groups.push_back(group);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RigidInterpolationOverrideFormat::check(const RigidInterpolationOverrideCard& card)
{
    if (card.groups.empty())
        return CardProblem{"card.missing-field", "the card needs at least one element", "elemId"};
    for (const RigidInterpolationOverrideGroup& group : card.groups)
    {
        if (const auto problem = checkRequiredId(group.elemId, "elemId"))
            return problem;
        if (const auto problem = checkReal(group.perturbationX, "perturbationX", RealRule::Finite))
            return problem;
        if (const auto problem = checkReal(group.perturbationY, "perturbationY", RealRule::Finite))
            return problem;
        if (const auto problem = checkReal(group.perturbationZ, "perturbationZ", RealRule::Finite))
            return problem;
        if (const auto problem = checkReal(group.toleranceAdjustment, "toleranceAdjustment", RealRule::Finite))
            return problem;
    }
    return std::nullopt;
}

void RigidInterpolationOverrideFormat::write(const RigidInterpolationOverrideCard& card, FieldWriter& writer)
{
    int start = 1;
    for (const RigidInterpolationOverrideGroup& group : card.groups)
    {
        writer.integer(start, group.elemId);
        writer.realUnlessDefault(start + 1, group.perturbationX, RIGID_OVERRIDE_DEFAULT_PERTURBATION);
        writer.realUnlessDefault(start + 2, group.perturbationY, RIGID_OVERRIDE_DEFAULT_PERTURBATION);
        writer.realUnlessDefault(start + 3, group.perturbationZ, RIGID_OVERRIDE_DEFAULT_PERTURBATION);
        writer.realUnlessDefault(start + 4, group.toleranceAdjustment, 0.0);
        start += FIELDS_PER_LINE;
    }
}

std::uint32_t RigidInterpolationOverrideTable::append(const RigidInterpolationOverrideCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_cards.push(card);
    return row;
}

void RigidInterpolationOverrideTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    for (RigidInterpolationOverrideGroup& group : m_cards.edit(row).groups)
        visitId(visitor, "elemId", ReferenceTarget::Element, group.elemId);
}

} // namespace oor::model
