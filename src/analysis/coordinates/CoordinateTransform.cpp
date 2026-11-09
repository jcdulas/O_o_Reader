#include "oor/analysis/coordinates/CoordinateTransform.hpp"

#include "oor/analysis/coordinates/CoordinateErrors.hpp"
#include "oor/model/cards/coordinate_systems/cylindricalFrame1/CylindricalFrame1Table.hpp"
#include "oor/model/cards/coordinate_systems/cylindricalFrame2/CylindricalFrame2Table.hpp"
#include "oor/model/cards/coordinate_systems/rectFrame1/RectFrame1Table.hpp"
#include "oor/model/cards/coordinate_systems/rectFrame2/RectFrame2Table.hpp"
#include "oor/model/cards/coordinate_systems/rectFrame3/RectFrame3Table.hpp"
#include "oor/model/cards/coordinate_systems/sphericalFrame1/SphericalFrame1Table.hpp"
#include "oor/model/cards/coordinate_systems/sphericalFrame2/SphericalFrame2Table.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/model/core/Model.hpp"

#include <string>

namespace oor::analysis
{

namespace
{

const CoordinateFrame BASIC_FRAME{};

// Adds the systems of one CORD1x table.
template <typename TableT>
void addThreeGridSystems(const model::Model& model, CoordinateKind kind, SystemDefinitions& definitions)
{
    const TableT* table = model.findTable<TableT>();
    if (table == nullptr)
        return;
    for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
    {
        if (!table->rows().isAlive(row))
            continue;
        const auto card = table->card(row);
        SystemDefinition definition;
        definition.cardName = TableT::NAME;
        definition.kind = kind;
        definition.form = SystemDefinition::Form::ThreeGrids;
        definition.id = card.id;
        definition.nodes = card.nodes;
        definitions[card.id] = definition;
    }
}

// Adds the systems of one CORD2x table.
template <typename TableT>
void addThreePointSystems(const model::Model& model, CoordinateKind kind, SystemDefinitions& definitions)
{
    const TableT* table = model.findTable<TableT>();
    if (table == nullptr)
        return;
    for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
    {
        if (!table->rows().isAlive(row))
            continue;
        const auto card = table->card(row);
        SystemDefinition definition;
        definition.cardName = TableT::NAME;
        definition.kind = kind;
        definition.form = SystemDefinition::Form::ThreePoints;
        definition.id = card.id;
        definition.referenceFrameId = card.referenceFrameId;
        definition.points = {card.pointA, card.pointB, card.pointC};
        definitions[card.id] = definition;
    }
}

base::Vec3 asVec3(const std::array<double, 3>& values) noexcept
{
    return base::Vec3{values[0], values[1], values[2]};
}

// The card of a system as a message names it: its deck keyword ("CORD2R 5"), from the dictionary of the model.
std::string cardText(const model::Model*, const SystemDefinition& definition)
{
    return std::string(definition.cardName) + " " + std::to_string(definition.id);
}

} // namespace

SystemDefinitions readSystemDefinitions(const model::Model& model)
{
    SystemDefinitions definitions;
    addThreePointSystems<model::RectFrame2Table>(model, CoordinateKind::Rectangular, definitions);
    addThreePointSystems<model::CylindricalFrame2Table>(model, CoordinateKind::Cylindrical, definitions);
    addThreePointSystems<model::SphericalFrame2Table>(model, CoordinateKind::Spherical, definitions);
    addThreeGridSystems<model::RectFrame1Table>(model, CoordinateKind::Rectangular, definitions);
    addThreeGridSystems<model::CylindricalFrame1Table>(model, CoordinateKind::Cylindrical, definitions);
    addThreeGridSystems<model::SphericalFrame1Table>(model, CoordinateKind::Spherical, definitions);
    // A CORD3R is a CORD1R whose points move with the grid points in SOL 700: the system of the model as it is read is
    // the one the three grid points give where they are.
    addThreeGridSystems<model::RectFrame3Table>(model, CoordinateKind::Rectangular, definitions);
    return definitions;
}

CoordinateTransform::CoordinateTransform(const model::Model& model) : m_model(&model)
{
    m_definitions = readSystemDefinitions(model);
    m_resolver = [this](base::EntityId nid) { return basicPositionOfNode(nid); };
}

std::string CoordinateTransform::describe(const StackEntry& entry) const
{
    if (entry.thing == Thing::Node)
        return "grid " + std::to_string(entry.id);
    const auto found = m_definitions.find(entry.id);
    if (found == m_definitions.end())
        return "coordinate system " + std::to_string(entry.id);
    return cardText(m_model, found->second);
}

CoordinateTransform::Visit CoordinateTransform::visit(Thing thing, base::EntityId id)
{
    for (std::size_t index = 0; index < m_stack.size(); ++index)
    {
        if (m_stack[index].thing == thing && m_stack[index].id == id)
        {
            std::string message = "coordinate system loop: ";
            for (std::size_t step = index; step < m_stack.size(); ++step)
                message += describe(m_stack[step]) + " -> ";
            message += describe(StackEntry{thing, id});
            throw CoordinateSystemError(message);
        }
    }
    m_stack.push_back(StackEntry{thing, id});
    return Visit(this);
}

CoordinateFrame CoordinateTransform::resolve(const SystemDefinition& definition)
{
    base::Vec3 points[3];
    if (definition.form == SystemDefinition::Form::ThreeGrids)
    {
        if (!m_resolver)
            throw CoordinateSystemError(cardText(m_model, definition) +
                                        " is defined by grid points, and no grid points are available");
        for (std::size_t index = 0; index < 3; ++index)
            points[index] = m_resolver(definition.nodes[index]);
    }
    else
    {
        // The points are coordinates in the system RID, in the kind of that system.
        if (definition.referenceFrameId != 0 && !m_definitions.contains(definition.referenceFrameId))
            throw CoordinateSystemError(cardText(m_model, definition) + " refers to coordinate system " +
                                        std::to_string(definition.referenceFrameId) + ", which does not exist");
        const CoordinateFrame& reference = frame(definition.referenceFrameId);
        for (std::size_t index = 0; index < 3; ++index)
            points[index] = reference.toBasic(asVec3(definition.points[index]));
    }
    CoordinateFrame resolved;
    if (!frameFromThreePoints(definition.kind, points[0], points[1], points[2], resolved))
        throw CoordinateSystemError(cardText(m_model, definition) +
                                    ": the three points do not define a coordinate system (two of them coincide, or "
                                    "the third is on the z axis)");
    return resolved;
}

const CoordinateFrame& CoordinateTransform::frame(base::EntityId cid)
{
    if (cid == 0)
        return BASIC_FRAME;
    if (const auto known = m_frames.find(cid); known != m_frames.end())
        return known->second;
    const auto definition = m_definitions.find(cid);
    if (definition == m_definitions.end())
        throw CoordinateSystemError("coordinate system " + std::to_string(cid) + " does not exist");
    const Visit visiting = visit(Thing::System, cid);
    const CoordinateFrame resolved = resolve(definition->second);
    ++m_resolvedCount;
    return m_frames.insert_or_assign(cid, resolved).first->second;
}

base::Vec3 CoordinateTransform::toBasic(base::EntityId cp, const base::Vec3& coordinates)
{
    if (cp == 0)
        return coordinates;
    return frame(cp).toBasic(coordinates);
}

base::Vec3 CoordinateTransform::fromBasic(base::EntityId cp, const base::Vec3& basicPoint)
{
    if (cp == 0)
        return basicPoint;
    return frame(cp).fromBasic(basicPoint);
}

base::Vec3 CoordinateTransform::basicPositionOfNode(base::EntityId nid)
{
    if (const auto known = m_positions.find(nid); known != m_positions.end())
        return known->second;
    if (m_model == nullptr)
        throw CoordinateSystemError("grid " + std::to_string(nid) + ": no model to take its position from");
    const model::NodeTable* grids = m_model->findTable<model::NodeTable>();
    const auto location = m_model->locate(base::IdSpace::Node, nid);
    if (grids == nullptr || !location || location->table != model::typeIndexOf<model::NodeTable>())
        throw CoordinateSystemError("grid " + std::to_string(nid) + " does not exist");
    const Visit visiting = visit(Thing::Node, nid);
    const base::EntityId cp = grids->positionFrameId(location->row);
    if (cp != 0 && !hasSystem(cp))
        throw CoordinateSystemError("grid " + std::to_string(nid) + " refers to coordinate system " +
                                    std::to_string(cp) + ", which does not exist");
    const base::Vec3 position = toBasic(cp, grids->xyz(location->row));
    m_positions[nid] = position;
    return position;
}

base::Mat3 CoordinateTransform::displacementRotation(base::EntityId cd, const base::Vec3& basicPosition)
{
    if (cd <= 0)
        return base::Mat3::identity();
    return frame(cd).directionsAt(basicPosition);
}

} // namespace oor::analysis
