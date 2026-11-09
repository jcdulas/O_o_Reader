#include "oor/model/cards/elements/userDefined8/UserDefined8Table.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_LIST_FIELD = 3;            // G1: the fields before it are EID and PID
constexpr base::EntityId ID_LIMIT = 100000000; // an element ID is below it (format reference, EID)

} // namespace

std::optional<UserDefined8Card> UserDefined8Format::read(FieldReader& reader)
{
    UserDefined8Card card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.requiredInteger(2, "propId");
    card.fields = readDummyFields(reader, FIRST_LIST_FIELD, "nodes/connectionFields");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> UserDefined8Format::check(const UserDefined8Card& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.id >= ID_LIMIT)
        return CardProblem{"card.bad-id", "{field} must be an integer from 1 to 99999999", "id"};
    if (const auto problem = checkRequiredId(card.propId, "propId"))
        return problem;
    // The element connects at least one grid point. Which fields are grid points and which are additional fields is the
    // NG of the ADUM8 card, which a card alone does not have, so nothing else about the list is checked but its
    // numbers.
    if (leadingGridCount(card.fields) == 0)
        return CardProblem{"card.missing-field",
                           "{field} is required: the field after {field:propId} is a positive integer", "node1"};
    return checkDummyFields(card.fields, "nodes/connectionFields");
}

void UserDefined8Format::write(const UserDefined8Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    writeDummyFields(writer, FIRST_LIST_FIELD, card.fields);
}

std::uint32_t UserDefined8Table::append(const UserDefined8Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_nodes.push(leadingGrids(card.fields));
    m_fields.push(card.fields);
    return row;
}

UserDefined8Card UserDefined8Table::card(std::uint32_t row) const
{
    return UserDefined8Card{m_id.get(row), m_propId.get(row), m_fields.get(row)};
}

void UserDefined8Table::store(std::uint32_t row, const UserDefined8Card& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_nodes.set(row, leadingGrids(card.fields));
    m_fields.set(row, card.fields);
}

void UserDefined8Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_nodes.keepRows(rows);
    m_fields.keepRows(rows);
}

void UserDefined8Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // PID is not visited: it names a PDUM8 or, when the element needs no property, a material, and the
    // card cannot tell which. The grid points are the leading integers of the list (see UserDefined8Card).
    const std::span<const base::EntityId> grids = m_nodes.get(row);
    std::vector<base::EntityId> updated(grids.begin(), grids.end());
    bool changed = false;
    for (base::EntityId& grid : updated)
    {
        const base::EntityId result = visitor.visit("node1", ReferenceTarget::Node, grid);
        changed = changed || result != grid;
        grid = result;
    }
    if (!changed)
        return;
    DummyFieldList fields = m_fields.get(row);
    for (std::size_t index = 0; index < updated.size(); ++index)
        fields[index] = DummyField::ofInteger(updated[index]);
    m_fields.set(row, fields);
    m_nodes.set(row, updated);
}

} // namespace oor::model
