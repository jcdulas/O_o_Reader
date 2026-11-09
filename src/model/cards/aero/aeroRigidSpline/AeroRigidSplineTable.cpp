#include "oor/model/cards/aero/aeroRigidSpline/AeroRigidSplineTable.hpp"

#include "oor/base/text/DigitSets.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <array>
#include <string_view>

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 16;

// The field of the grid point of each pair: G1 and G2 end the first line, the other pairs are on the second one. The
// components are in the field after it.
constexpr std::array<int, AeroRigidSplineCard::MAX_PAIRS> GRID_FIELDS{5, 7, 9, 11, 13, 15};
constexpr std::array<const char*, AeroRigidSplineCard::MAX_PAIRS> GRID_NAMES{"node1", "node2", "node3",
                                                                             "node4", "node5", "node6"};
constexpr std::array<const char*, AeroRigidSplineCard::MAX_PAIRS> COMPONENT_NAMES{
    "components1", "components2", "components3", "components4", "components5", "components6"};
constexpr std::array<std::string_view, AeroRigidSplineCard::MAX_PAIRS> GRID_REFERENCES{"node1", "node2", "node3",
                                                                                       "node4", "node5", "node6"};

// How many degrees of freedom a set of components names: its digits.
int countComponents(std::int32_t components)
{
    int count = 0;
    for (std::int32_t rest = components; rest > 0; rest /= 10)
        ++count;
    return count;
}

} // namespace

std::optional<AeroRigidSplineCard> AeroRigidSplineFormat::read(FieldReader& reader)
{
    AeroRigidSplineCard card;
    card.id = reader.requiredInteger(1, "id");
    card.aeroPanelId = reader.integer(2, "aeroPanelId", 0);
    card.aeroBoxListId = reader.requiredInteger(3, "aeroBoxListId");
    card.usage = readKeyword<SplineUsage>(reader, 4, "usage", SPLINE_USAGE_WORDS, SplineUsage::Both);
    for (std::size_t index = 0; index < AeroRigidSplineCard::MAX_PAIRS && !reader.failed(); ++index)
    {
        const int field = GRID_FIELDS[index];
        if (reader.isBlank(field) && reader.isBlank(field + 1))
            continue; // padding
        DofPair pair;
        pair.node = reader.requiredInteger(field, GRID_NAMES[index]);
        pair.components = base::sortedDigits(reader.requiredInteger(field + 1, COMPONENT_NAMES[index]));
        card.dofs.push_back(pair);
    }
    requireBlankFields(reader, {}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroRigidSplineFormat::check(const AeroRigidSplineCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkOptionalId(card.aeroPanelId, "aeroPanelId"))
        return problem;
    if (const auto problem = checkRequiredId(card.aeroBoxListId, "aeroBoxListId"))
        return problem;
    if (card.dofs.empty())
        return CardProblem{"card.missing-field", "the card needs at least one ({field}, {field:components1}) pair",
                           "node1"};
    if (card.dofs.size() > AeroRigidSplineCard::MAX_PAIRS)
        return CardProblem{"card.bad-value", "the card has at most six ({field}, {field:components1}) pairs", "node1"};
    int total = 0;
    for (std::size_t index = 0; index < card.dofs.size(); ++index)
    {
        const DofPair& pair = card.dofs[index];
        if (pair.node <= 0)
            return CardProblem{"card.bad-id", "a structural grid point ID must be a positive integer",
                               GRID_NAMES[index]};
        if (pair.components <= 0 || !base::isComponentSet(pair.components))
            return CardProblem{"card.bad-value", "{field} must be a list of the digits 1 to 6, each at most once",
                               COMPONENT_NAMES[index]};
        total += countComponents(pair.components);
    }
    // The pairs select exactly six degrees of freedom, the motion of a rigid body.
    if (total != AeroRigidSplineCard::TOTAL_COMPONENTS)
        return CardProblem{"card.bad-value",
                           "the ({field:node1}, {field}) pairs must select exactly six degrees of freedom",
                           "components1"};
    return std::nullopt;
}

void AeroRigidSplineFormat::write(const AeroRigidSplineCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.aeroPanelId, 0);
    writer.integer(3, card.aeroBoxListId);
    writeKeywordUnlessDefault(writer, 4, SPLINE_USAGE_WORDS, card.usage, SplineUsage::Both);
    for (std::size_t index = 0; index < card.dofs.size() && index < AeroRigidSplineCard::MAX_PAIRS; ++index)
    {
        writer.integer(GRID_FIELDS[index], card.dofs[index].node);
        writer.integer(GRID_FIELDS[index] + 1, card.dofs[index].components);
    }
}

std::uint32_t AeroRigidSplineTable::append(const AeroRigidSplineCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void AeroRigidSplineTable::store(std::uint32_t row, const AeroRigidSplineCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void AeroRigidSplineTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void AeroRigidSplineTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void AeroRigidSplineTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    AeroRigidSplineCard& card = m_cards.edit(row);
    visitId(visitor, "aeroPanelId", ReferenceTarget::AeroElement, card.aeroPanelId);
    visitId(visitor, "aeroBoxListId", ReferenceTarget::AeroList, card.aeroBoxListId);
    for (std::size_t index = 0; index < card.dofs.size() && index < AeroRigidSplineCard::MAX_PAIRS; ++index)
        visitId(visitor, GRID_REFERENCES[index], ReferenceTarget::Node, card.dofs[index].node);
}

} // namespace oor::model
