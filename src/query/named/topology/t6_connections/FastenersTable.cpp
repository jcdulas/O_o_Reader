#include "oor/analysis/components/ComponentModel.hpp"
#include "oor/analysis/geometry/ElementGeometry.hpp"
#include "oor/base/errors/Errors.hpp"
#include "oor/model/cards/elements/bushing/BushingTable.hpp"
#include "oor/model/cards/elements/bushing1D/Bushing1DTable.hpp"
#include "oor/model/cards/elements/fastener/FastenerTable.hpp"
#include "oor/model/cards/elements/springDofProperty/SpringDofPropertyTable.hpp"
#include "oor/model/cards/elements/springDofInline/SpringDofInlineTable.hpp"
#include "oor/model/cards/elements/springScalarProperty/SpringScalarPropertyTable.hpp"
#include "oor/model/cards/elements/springScalarInline/SpringScalarInlineTable.hpp"
#include "oor/model/cards/elements/weld/WeldTable.hpp"
#include "oor/model/cards/properties/bushProp/BushPropTable.hpp"
#include "oor/model/cards/properties/fastenerProp/FastenerPropTable.hpp"
#include "oor/model/cards/properties/springProp/SpringPropTable.hpp"
#include "oor/model/cards/properties/weldProp/WeldPropTable.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardFacts.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/common/ElementData.hpp"
#include "oor/query/named/common/NodePositions.hpp"
#include "oor/query/named/topology/t6_connections/ConnectionQueries.hpp"

#include <array>
#include <cmath>
#include <limits>
#include <map>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// What a fastener card says: its ends, the scalar components, its property, its system.
struct Fastener
{
    std::int32_t n1 = 0;
    std::int32_t n2 = 0;
    std::int32_t dof1 = 0; // the component of a scalar spring at each end (0: none)
    std::int32_t dof2 = 0;
    std::int32_t pid = 0;
    std::int32_t cid = -1; // -1: none
    bool scalarSpring = false;
    double elementStiffness = NOT_A_NUMBER; // CELAS2 and CELAS4 carry K on the card
};

template <typename TableT>
const TableT& tableOf(const model::Model& cards)
{
    return static_cast<const TableT&>(*cards.findTable(model::typeIndexOf<TableT>()));
}

// The element card at a location, as a Fastener; false for a card type that is not one of the list.
bool read(const model::Model& cards, model::CardLocation location, Fastener& fastener)
{
    using namespace oor::model;
    const std::uint16_t type = location.table;
    if (type == typeIndexOf<BushingTable>())
    {
        const BushingCard card = tableOf<BushingTable>(cards).card(location.row);
        fastener.n1 = card.nodeA;
        fastener.n2 = card.nodeB;
        fastener.pid = card.propId;
        fastener.cid = card.frameId;
    }
    else if (type == typeIndexOf<FastenerTable>())
    {
        const FastenerCard card = tableOf<FastenerTable>(cards).card(location.row);
        fastener.n1 = card.nodeA;
        fastener.n2 = card.nodeB;
        fastener.pid = card.propId;
    }
    else if (type == typeIndexOf<WeldTable>())
    {
        const WeldCard card = tableOf<WeldTable>(cards).card(location.row);
        fastener.n1 = card.nodeA;
        fastener.n2 = card.nodeB;
        fastener.pid = card.propId;
    }
    else if (type == typeIndexOf<SpringDofPropertyTable>())
    {
        const SpringDofPropertyCard card = tableOf<SpringDofPropertyTable>(cards).card(location.row);
        fastener.n1 = card.node1;
        fastener.n2 = card.node2;
        fastener.dof1 = card.components1;
        fastener.dof2 = card.components2;
        fastener.pid = card.propId;
        fastener.scalarSpring = true;
    }
    else if (type == typeIndexOf<SpringDofInlineTable>())
    {
        const SpringDofInlineCard card = tableOf<SpringDofInlineTable>(cards).card(location.row);
        fastener.n1 = card.node1;
        fastener.n2 = card.node2;
        fastener.dof1 = card.components1;
        fastener.dof2 = card.components2;
        fastener.elementStiffness = card.stiffness;
        fastener.scalarSpring = true;
    }
    else if (type == typeIndexOf<SpringScalarPropertyTable>())
    {
        const SpringScalarPropertyCard card = tableOf<SpringScalarPropertyTable>(cards).card(location.row);
        fastener.n1 = card.scalarPoint1;
        fastener.n2 = card.scalarPoint2;
        fastener.pid = card.propId;
        fastener.scalarSpring = true;
    }
    else if (type == typeIndexOf<SpringScalarInlineTable>())
    {
        const SpringScalarInlineCard card = tableOf<SpringScalarInlineTable>(cards).card(location.row);
        fastener.n1 = card.scalarPoint1;
        fastener.n2 = card.scalarPoint2;
        fastener.elementStiffness = card.stiffness;
        fastener.scalarSpring = true;
    }
    else if (type == typeIndexOf<Bushing1DTable>())
    {
        const Bushing1DCard card = tableOf<Bushing1DTable>(cards).card(location.row);
        fastener.n1 = card.nodeA;
        fastener.n2 = card.nodeB;
        fastener.pid = card.propId;
        fastener.cid = card.frameId;
    }
    else
    {
        return false;
    }
    return true;
}

// fastenersTable: bushes, fasteners, welds and springs: ends, stiffnesses, axis and the components they join.
class FastenersTableQuery final : public QueryBase
{
public:
    FastenersTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        model::Model& model = *run.model;
        std::vector<std::uint16_t> types = fastenerElementTypes();
        const std::vector<std::string> typeNames = run.arguments.texts("types");
        if (!typeNames.empty())
        {
            const std::vector<std::uint16_t> wanted = typesNamed(typeNames, model::CardCategory::Element, "types");
            std::vector<std::uint16_t> kept;
            for (const std::uint16_t type : types)
            {
                if (std::find(wanted.begin(), wanted.end(), type) != wanted.end())
                    kept.push_back(type);
            }
            types = kept;
        }
        const CardList list = listCards(model, types, run.arguments.ids("elementIds"));
        const std::size_t count = list.size();
        if (count == 0)
            return emptyTable(info().defaultColumns());
        const NodePositions positions(model, run.context.pool());
        const auto components = analysis::ComponentModel::of(model, run.context.pool());
        CardFacts facts(model);

        // The element axes (where the ends do not give a line): per card type, one call.
        std::vector<base::Vec3> frameAxis(count, base::Vec3{NOT_A_NUMBER, NOT_A_NUMBER, NOT_A_NUMBER});
        {
            std::map<std::uint16_t, std::pair<std::vector<std::uint32_t>, std::vector<std::size_t>>> groups;
            for (std::size_t index = 0; index < count; ++index)
            {
                groups[list.types[index]].first.push_back(list.rows[index]);
                groups[list.types[index]].second.push_back(index);
            }
            for (const auto& [type, group] : groups)
            {
                try
                {
                    const analysis::ElementArrays arrays = analysis::ElementGeometry::compute(
                        model, type, group.first, analysis::bit(analysis::Quantity::Frame), run.context.pool());
                    for (std::size_t item = 0; item < group.second.size() && item < arrays.frames.size(); ++item)
                        frameAxis[group.second[item]] = arrays.frames[item].column(0);
                }
                catch (const base::OperationCancelled&)
                {
                    throw;
                }
                catch (const base::OorError&)
                {
                }
            }
        }

        std::vector<std::string> type(count);
        std::vector<std::int32_t> pid(count);
        std::vector<std::uint8_t> pidValid(count);
        std::vector<std::int32_t> n1(count);
        std::vector<std::int32_t> n2(count);
        std::vector<std::uint8_t> n1Valid(count);
        std::vector<std::uint8_t> n2Valid(count);
        std::vector<std::int32_t> dof1(count);
        std::vector<std::int32_t> dof2(count);
        std::vector<std::uint8_t> dofValid(count);
        std::vector<double> length(count);
        std::array<std::vector<double>, 6> stiffness;
        for (auto& values : stiffness)
            values.assign(count, NOT_A_NUMBER);
        std::vector<double> diameter(count, NOT_A_NUMBER);
        std::vector<std::int32_t> cid(count);
        std::vector<std::uint8_t> cidValid(count);
        std::array<std::vector<double>, 3> axis;
        for (auto& values : axis)
            values.assign(count, NOT_A_NUMBER);
        std::vector<std::int64_t> component1(count);
        std::vector<std::int64_t> component2(count);
        std::vector<std::uint8_t> component1Valid(count);
        std::vector<std::uint8_t> component2Valid(count);
        for (std::size_t index = 0; index < count; ++index)
        {
            Fastener fastener;
            (void)read(model, list.location(index), fastener);
            type[index] = std::string(typeName(list.types[index]));
            pid[index] = fastener.pid;
            pidValid[index] = fastener.pid != 0 ? 1 : 0;
            n1[index] = fastener.n1;
            n2[index] = fastener.n2;
            n1Valid[index] = fastener.n1 != 0 ? 1 : 0;
            n2Valid[index] = fastener.n2 != 0 ? 1 : 0;
            dof1[index] = fastener.dof1;
            dof2[index] = fastener.dof2;
            dofValid[index] = fastener.scalarSpring && (fastener.dof1 != 0 || fastener.dof2 != 0) ? 1 : 0;
            cid[index] = fastener.cid;
            cidValid[index] = fastener.cid >= 0 ? 1 : 0;
            length[index] = positions.distance(fastener.n1, fastener.n2);
            const std::int32_t first = components->componentOfNode(fastener.n1);
            const std::int32_t second = components->componentOfNode(fastener.n2);
            component1[index] = first;
            component2[index] = second;
            component1Valid[index] = first != 0 ? 1 : 0;
            component2Valid[index] = second != 0 ? 1 : 0;

            // The stiffnesses and the diameter come from the property (or from the card: CELAS2, CELAS4).
            if (!std::isnan(fastener.elementStiffness))
                stiffness[0][index] = fastener.elementStiffness;
            if (fastener.pid != 0)
            {
                if (const auto property = model.locate(base::IdSpace::Property, fastener.pid))
                {
                    const std::uint16_t kind = property->table;
                    if (kind == model::typeIndexOf<model::BushPropTable>())
                    {
                        const model::BushPropCard card = tableOf<model::BushPropTable>(model).card(property->row);
                        for (std::size_t item = 0; item < 6; ++item)
                            stiffness[item][index] = card.stiffness[item];
                    }
                    else if (kind == model::typeIndexOf<model::FastenerPropTable>())
                    {
                        const model::FastenerPropCard card =
                            tableOf<model::FastenerPropTable>(model).card(property->row);
                        for (std::size_t item = 0; item < 3; ++item)
                        {
                            stiffness[item][index] = card.translationalStiffness[item];
                            stiffness[3 + item][index] = card.rotationalStiffness[item];
                        }
                        diameter[index] = card.diameter;
                    }
                    else if (kind == model::typeIndexOf<model::WeldPropTable>())
                    {
                        diameter[index] = tableOf<model::WeldPropTable>(model).card(property->row).diameter;
                    }
                    else if (kind == model::typeIndexOf<model::SpringPropTable>())
                    {
                        stiffness[0][index] = tableOf<model::SpringPropTable>(model).card(property->row).stiffness;
                    }
                    else
                    {
                        // PBUSH1D and any other spring property: its stiffness K when it has one.
                        stiffness[0][index] = facts.realField(model, *property, "stiffness");
                    }
                }
            }
            // The axis: the line from the first end to the second when they are apart, else the element x axis.
            if (positions.has(fastener.n1) && positions.has(fastener.n2) && length[index] > 1.0e-12)
            {
                const base::Vec3 direction =
                    base::normalized(positions.position(fastener.n2) - positions.position(fastener.n1));
                axis[0][index] = direction.x;
                axis[1][index] = direction.y;
                axis[2][index] = direction.z;
            }
            else if (!std::isnan(frameAxis[index].x))
            {
                axis[0][index] = frameAxis[index].x;
                axis[1][index] = frameAxis[index].y;
                axis[2][index] = frameAxis[index].z;
            }
        }
        const ListOrigin origin = originOf(list);
        TableBuilder builder;
        builder.int32("elementId", std::vector<std::int32_t>(list.ids));
        builder.dictionary("type", type);
        builder.int32("propId", std::move(pid), std::move(pidValid), ColumnOrigin::Stored, "pid");
        builder.int32("n1", std::move(n1), std::move(n1Valid));
        builder.int32("n2", std::move(n2), std::move(n2Valid));
        builder.int32("dof1", std::move(dof1), dofValid);
        builder.int32("dof2", std::move(dof2), dofValid);
        builder.real("length", std::move(length));
        static const char* const K[6] = {"k1", "k2", "k3", "k4", "k5", "k6"};
        for (std::size_t item = 0; item < 6; ++item)
            builder.real(K[item], std::move(stiffness[item]));
        builder.real("diameter", std::move(diameter));
        builder.int32("frameId", std::move(cid), std::move(cidValid));
        builder.real("axisX", std::move(axis[0]));
        builder.real("axisY", std::move(axis[1]));
        builder.real("axisZ", std::move(axis[2]));
        builder.int64("component1", std::move(component1), std::move(component1Valid));
        builder.int64("component2", std::move(component2), std::move(component2Valid));
        builder.rows(origin.types, origin.keys, run.link);
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "fastenersTable";
        info.family = "T6";
        info.description =
            "The CBUSH, CFAST, CWELD, CELAS1 to CELAS4 and CBUSH1D elements: the two ends (n1, n2), the component a "
            "scalar "
            "spring acts on at each end (dof1, dof2), the length, the stiffnesses of the property (k1 to k6 of a bush, "
            "kt1 to kt3 and kr1 to kr3 of a fastener as k1 to k6; only k1 for a scalar spring or a bush of one "
            "stiffness, taken "
            "from the property or from the card for CELAS2 and CELAS4), the diameter (CFAST, CWELD), the coordinate "
            "system "
            "of the element, the fastener axis in the basic system (the line from n1 to n2 when the ends are apart, "
            "else the "
            "x axis of the element) and the components (T4) that each end belongs to.";
        info.parameters = {parameter("types", ParameterKind::TextList, "None", "keep these card types"),
                           parameter("elementIds", ParameterKind::IdList, "None", "keep these elements")};
        info.defaultSort = "elementId";
        ColumnListBuilder columns;
        columns.int32("elementId")
            .dictionary("type")
            .int32("propId", true)
            .int32("n1")
            .int32("n2")
            .int32("dof1")
            .int32("dof2")
            .real("length");
        columns.reals("k1 k2 k3 k4 k5 k6 diameter");
        columns.int32("frameId").vector3("axis", "XYZ").int64("component1").int64("component2");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addFastenersTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<FastenersTableQuery>());
}

} // namespace oor::query
