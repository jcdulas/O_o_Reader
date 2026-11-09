#include "oor/model/cards/elements/bushing2D/Bushing2DTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/elements/InterfaceSupport.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <algorithm>

namespace oor::model
{

namespace
{

constexpr int PLANE_FIELD = 6;
constexpr int LAST_FIELD = PLANE_FIELD;

bool isPlaneWord(std::string_view text)
{
    return std::ranges::any_of(BUSH_ELEM_2D_PLANE_WORDS, [text](std::string_view word) { return word == text; });
}

} // namespace

std::optional<Bushing2DCard> Bushing2DFormat::read(FieldReader& reader)
{
    Bushing2DCard card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.requiredInteger(2, "propId");
    readInterfaceNodes(reader, card.nodes, Bushing2DCard::NODE_COUNT, BUSH_ELEM_2D_NODE_NAMES);
    card.frameId = reader.integer(5, "frameId", 0);
    if (const std::string_view plane = reader.text(PLANE_FIELD); !plane.empty())
        card.plane = std::string(plane);
    requireBlankFields(reader, {}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> Bushing2DFormat::check(const Bushing2DCard& card)
{
    if (const auto problem = checkInterfaceElementId(card.id))
        return problem;
    if (const auto problem = checkRequiredId(card.propId, "propId"))
        return problem;
    // GA and GB are two grids that coincide: the same ID twice is not that.
    if (const auto problem = checkInterfaceNodes(card.nodes, Bushing2DCard::NODE_COUNT, true, BUSH_ELEM_2D_NODE_NAMES))
        return problem;
    if (card.frameId < 0)
        return CardProblem{"card.bad-id", "the coordinate system ID {field} must not be negative", "frameId"};
    if (!isPlaneWord(card.plane))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be " + keywordList(BUSH_ELEM_2D_PLANE_WORDS),
                           "plane"};
    return std::nullopt;
}

void Bushing2DFormat::write(const Bushing2DCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    writeInterfaceNodes(writer, card.nodes, Bushing2DCard::NODE_COUNT);
    writer.integerUnlessDefault(5, card.frameId, 0);
    writer.textUnlessDefault(PLANE_FIELD, card.plane, BUSH_ELEM_2D_DEFAULT_PLANE);
}

std::uint32_t Bushing2DTable::append(const Bushing2DCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_nodes.push(card.nodes);
    m_frameId.push(card.frameId);
    m_plane.push(card.plane);
    return row;
}

Bushing2DCard Bushing2DTable::card(std::uint32_t row) const
{
    Bushing2DCard card;
    card.id = m_id.get(row);
    card.propId = m_propId.get(row);
    card.nodes = m_nodes.get(row);
    card.frameId = m_frameId.get(row);
    card.plane = m_plane.get(row);
    return card;
}

void Bushing2DTable::store(std::uint32_t row, const Bushing2DCard& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_nodes.set(row, card.nodes);
    m_frameId.set(row, card.frameId);
    m_plane.set(row, card.plane);
}

void Bushing2DTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_nodes.keepRows(rows);
    m_frameId.keepRows(rows);
    m_plane.keepRows(rows);
}

void Bushing2DTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId propId = m_propId.get(row); propId > 0)
        m_propId.set(row, visitor.visit("propId", ReferenceTarget::Property, propId));
    visitInterfaceNodes(visitor, m_nodes, row);
    // 0 is the basic system: nothing to follow.
    if (const base::EntityId frameId = m_frameId.get(row); frameId > 0)
        m_frameId.set(row, visitor.visit("frameId", ReferenceTarget::Coordinate, frameId));
}

} // namespace oor::model
