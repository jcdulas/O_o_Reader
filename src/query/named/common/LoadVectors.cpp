#include "oor/query/named/common/LoadVectors.hpp"

#include "oor/base/errors/Errors.hpp"
#include "oor/model/cards/loads/gravityLoad/GravityLoadTable.hpp"
#include "oor/model/cards/loads/pointForceLoad/PointForceLoadTable.hpp"
#include "oor/model/cards/loads/pointForceLoad1/PointForceLoad1Table.hpp"
#include "oor/model/cards/loads/pointForceLoad2/PointForceLoad2Table.hpp"
#include "oor/model/cards/loads/pointMomentLoad/PointMomentLoadTable.hpp"
#include "oor/model/cards/loads/pointMomentLoad1/PointMomentLoad1Table.hpp"
#include "oor/model/cards/loads/pointMomentLoad2/PointMomentLoad2Table.hpp"
#include "oor/model/cards/loads/scalarPointLoad/ScalarPointLoadTable.hpp"

namespace oor::query
{

namespace
{

template <typename TableT>
const TableT& tableOf(const model::Model& cards, std::uint16_t type)
{
    (void)type;
    return static_cast<const TableT&>(*cards.findTable(model::typeIndexOf<TableT>()));
}

} // namespace

LoadVectors::LoadVectors(const model::Model& model, base::ThreadPool* pool)
    : m_model(model), m_pool(pool), m_transform(model)
{
}

const NodePositions& LoadVectors::positions() const
{
    if (!m_positions)
        m_positions.emplace(m_model, m_pool);
    return *m_positions;
}

base::Vec3 LoadVectors::inBasic(std::int32_t cid, const base::Vec3& point, const base::Vec3& components)
{
    if (cid == 0 || !m_transform.hasSystem(cid))
        return components;
    try
    {
        return m_transform.frame(cid).directionsAt(point) * components;
    }
    catch (const base::OorError&)
    {
        return components;
    }
}

std::optional<std::int32_t> LoadVectors::systemOf(const analysis::ElementaryLoad& load) const
{
    using namespace oor::model;
    const std::uint16_t type = load.card.table;
    if (type == typeIndexOf<PointForceLoadTable>())
        return tableOf<PointForceLoadTable>(m_model, type).card(load.card.row).frameId;
    if (type == typeIndexOf<PointMomentLoadTable>())
        return tableOf<PointMomentLoadTable>(m_model, type).card(load.card.row).frameId;
    if (type == typeIndexOf<GravityLoadTable>())
        return tableOf<GravityLoadTable>(m_model, type).card(load.card.row).frameId;
    return std::nullopt;
}

std::vector<NodalLoad> LoadVectors::nodal(const analysis::ElementaryLoad& load)
{
    using namespace oor::model;
    std::vector<NodalLoad> result;
    const std::uint16_t type = load.card.table;
    const std::uint32_t row = load.card.row;
    const double scale = load.scale;
    const auto line = [&](std::int32_t from, std::int32_t to)
    { return positions().position(to) - positions().position(from); };
    if (type == typeIndexOf<PointForceLoadTable>() || type == typeIndexOf<PointMomentLoadTable>())
    {
        const bool isForce = type == typeIndexOf<PointForceLoadTable>();
        std::int32_t node = 0;
        std::int32_t cid = 0;
        double magnitude = 0.0;
        std::array<double, 3> direction{};
        if (isForce)
        {
            const PointForceLoadCard card = tableOf<PointForceLoadTable>(m_model, type).card(row);
            node = card.node;
            cid = card.frameId;
            magnitude = card.magnitude;
            direction = card.direction;
        }
        else
        {
            const PointMomentLoadCard card = tableOf<PointMomentLoadTable>(m_model, type).card(row);
            node = card.node;
            cid = card.frameId;
            magnitude = card.magnitude;
            direction = card.direction;
        }
        // the point matters only to a system of the model (the directions of a cylindrical one depend on it)
        const base::Vec3 point = cid != 0 && m_transform.hasSystem(cid) ? positions().position(node) : base::Vec3{};
        const base::Vec3 vector =
            scale * magnitude * inBasic(cid, point, base::Vec3{direction[0], direction[1], direction[2]});
        result.push_back(isForce ? NodalLoad{node, vector, {}} : NodalLoad{node, {}, vector});
    }
    else if (type == typeIndexOf<PointForceLoad1Table>() || type == typeIndexOf<PointMomentLoad1Table>())
    {
        const bool isForce = type == typeIndexOf<PointForceLoad1Table>();
        std::int32_t node = 0;
        double magnitude = 0.0;
        std::int32_t g1 = 0;
        std::int32_t g2 = 0;
        if (isForce)
        {
            const PointForceLoad1Card card = tableOf<PointForceLoad1Table>(m_model, type).card(row);
            node = card.node;
            magnitude = card.magnitude;
            g1 = card.node1;
            g2 = card.node2;
        }
        else
        {
            const PointMomentLoad1Card card = tableOf<PointMomentLoad1Table>(m_model, type).card(row);
            node = card.node;
            magnitude = card.magnitude;
            g1 = card.node1;
            g2 = card.node2;
        }
        const base::Vec3 vector = scale * magnitude * base::normalized(line(g1, g2));
        result.push_back(isForce ? NodalLoad{node, vector, {}} : NodalLoad{node, {}, vector});
    }
    else if (type == typeIndexOf<PointForceLoad2Table>() || type == typeIndexOf<PointMomentLoad2Table>())
    {
        const bool isForce = type == typeIndexOf<PointForceLoad2Table>();
        std::int32_t node = 0;
        double magnitude = 0.0;
        std::array<std::int32_t, 4> g{};
        if (isForce)
        {
            const PointForceLoad2Card card = tableOf<PointForceLoad2Table>(m_model, type).card(row);
            node = card.node;
            magnitude = card.magnitude;
            g = {card.node1, card.node2, card.node3, card.node4};
        }
        else
        {
            const PointMomentLoad2Card card = tableOf<PointMomentLoad2Table>(m_model, type).card(row);
            node = card.node;
            magnitude = card.magnitude;
            g = {card.node1, card.node2, card.node3, card.node4};
        }
        const base::Vec3 vector = scale * magnitude * base::normalized(base::cross(line(g[0], g[1]), line(g[2], g[3])));
        result.push_back(isForce ? NodalLoad{node, vector, {}} : NodalLoad{node, {}, vector});
    }
    else if (type == typeIndexOf<ScalarPointLoadTable>())
    {
        for (const PointLoad& point : tableOf<ScalarPointLoadTable>(m_model, type).card(row).loads)
            result.push_back(NodalLoad{point.node, base::Vec3{scale * point.magnitude, 0.0, 0.0}, {}});
    }
    return result;
}

std::optional<base::Vec3> LoadVectors::acceleration(const analysis::ElementaryLoad& load)
{
    using namespace oor::model;
    if (load.card.table != typeIndexOf<GravityLoadTable>())
        return std::nullopt;
    const GravityLoadCard card = tableOf<GravityLoadTable>(m_model, load.card.table).card(load.card.row);
    const base::Vec3 direction =
        inBasic(card.frameId, base::Vec3{}, base::Vec3{card.direction[0], card.direction[1], card.direction[2]});
    return load.scale * card.magnitude * direction;
}

} // namespace oor::query
