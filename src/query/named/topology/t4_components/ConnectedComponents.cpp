#include "oor/analysis/components/ComponentCache.hpp"
#include "oor/analysis/components/ComponentModel.hpp"
#include "oor/analysis/components/RevisionStamp.hpp"
#include "oor/analysis/connections/ConnectionClusters.hpp"
#include "oor/analysis/mass/NonStructuralMass.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/ComponentNodes.hpp"
#include "oor/query/named/common/ContactArguments.hpp"
#include "oor/query/named/common/ElementData.hpp"
#include "oor/query/named/common/NodePositions.hpp"
#include "oor/query/named/topology/t4_components/ComponentQueries.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <optional>
#include <string>

namespace oor::query
{

namespace
{

constexpr double INFINITE = std::numeric_limits<double>::infinity();

// The bits of a 64-bit stamp as the int64 the table stores.
std::int64_t asInt64(std::uint64_t stamp)
{
    std::int64_t value = 0;
    std::memcpy(&value, &stamp, sizeof(value));
    return value;
}

// What the table of the query depends on besides the content of the model: the detail, the contact options and the
// non-structural mass the Case Control selects (the masses of the components; the Case Control is not part of the
// content).
std::string memoKey(const std::string& detail, const analysis::ContactOptions& contact,
                    std::optional<base::EntityId> nonStructural)
{
    return "connectedComponents|" + detail + "|" + contactKey(contact) + "|" +
           (nonStructural ? std::to_string(*nonStructural) : "-");
}

// connectedComponents: the components of the model, or the component of each element.
class ConnectedComponentsQuery final : public QueryBase
{
public:
    ConnectedComponentsQuery() : QueryBase(makeInfo()) {}

    // The table is made once for a content of the model and the arguments that change it: a call again returns it as
    // it is (the components were cached, but the table, the masses and the boxes were made again at each call: 120 ns
    // an element).
    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const std::string detail = run.arguments.textOr("detail", "components");
        if (detail != "components" && detail != "elements" && detail != "nodes")
            throw QueryError("connectedComponents: detail is \"components\", \"elements\" or \"nodes\", not \"" +
                             detail + "\"");
        const model::Model& model = *run.model;
        const analysis::ContactOptions contact = contactOptionsOf(run.arguments, "connectedComponents");
        analysis::ComponentCache& cache = analysis::ComponentCache::of(model);
        // The NSM set the Case Control selects, looked for in every subcase once for a state of the Case Control (a
        // call again looked for it in every subcase: 5 ns a subcase on 15 microseconds).
        const auto selected = std::static_pointer_cast<const std::optional<base::EntityId>>(cache.memo(
            model, "the NSM set the Case Control selects",
            [&]
            {
                return std::static_pointer_cast<const void>(std::make_shared<const std::optional<base::EntityId>>(
                    analysis::NonStructuralMass::selectedByCaseControl(model)));
            },
            true));
        const std::string key = memoKey(detail, contact, *selected);
        return std::static_pointer_cast<const QueryTable>(
            cache.memo(model, key, [&] { return std::static_pointer_cast<const void>(build(run, detail, contact)); }));
    }

private:
    [[nodiscard]] std::shared_ptr<const QueryTable> build(const QueryRun& run, const std::string& detail,
                                                          const analysis::ContactOptions& contact) const
    {
        const model::Model& model = *run.model;
        const std::shared_ptr<const analysis::ComponentModel> components =
            analysis::ComponentModel::of(model, contact, run.context.pool());
        if (detail == "nodes")
            return componentNodesTable(model, *components, info().columnsFor("nodes"), run.context.pool());
        const CardList elements = listCards(model, typesOfCategory(model::CardCategory::Element));
        ElementData data(run.model, elements, run.context);

        // The component of each element (0: none).
        std::vector<std::int32_t> componentOf(elements.size());
        for (std::size_t index = 0; index < elements.size(); ++index)
            componentOf[index] = components->componentOfElement(elements.location(index));

        TableBuilder builder;
        if (detail == "elements")
        {
            if (elements.size() == 0)
                return emptyTable(info().columnsFor("elements"));
            const std::vector<std::int32_t>& pids = data.properties();
            std::vector<std::string> types(elements.size());
            std::vector<std::int64_t> component(elements.size());
            std::vector<std::uint8_t> componentValid(elements.size());
            std::vector<std::uint8_t> pidValid(elements.size());
            for (std::size_t index = 0; index < elements.size(); ++index)
            {
                types[index] = std::string(typeName(elements.types[index]));
                component[index] = componentOf[index];
                componentValid[index] = componentOf[index] != 0 ? 1 : 0;
                pidValid[index] = pids[index] != 0 ? 1 : 0;
            }
            builder.int32("elementId", std::vector<std::int32_t>(elements.ids));
            builder.dictionary("type", types);
            builder.int32("propId", std::vector<std::int32_t>(pids), std::move(pidValid));
            builder.int64("componentId", std::move(component), std::move(componentValid));
            return builder.finish();
        }

        const std::size_t count = components->componentCount();
        if (count == 0)
            return emptyTable(info().defaultColumns());
        std::vector<double> mass(count, 0.0);
        std::vector<std::vector<std::int32_t>> properties(count);
        if (elements.size() > 0)
        {
            const std::vector<std::int32_t>& pids = data.properties();
            const ElementMassData& masses = data.masses();
            for (std::size_t index = 0; index < elements.size(); ++index)
            {
                if (componentOf[index] == 0)
                    continue;
                const auto at = static_cast<std::size_t>(componentOf[index] - 1);
                if (std::isfinite(masses.mass[index]))
                    mass[at] += masses.mass[index];
                if (pids[index] != 0)
                    properties[at].push_back(pids[index]);
            }
        }
        const NodePositions positions(model, run.context.pool());
        std::array<std::vector<double>, 6> box;
        for (std::size_t item = 0; item < 6; ++item)
            box[item].assign(count, std::numeric_limits<double>::quiet_NaN());
        for (std::size_t at = 0; at < count; ++at)
        {
            double low[3] = {INFINITE, INFINITE, INFINITE};
            double high[3] = {-INFINITE, -INFINITE, -INFINITE};
            bool any = false;
            for (const base::EntityId node : components->nodesOf(static_cast<std::int32_t>(at + 1)))
            {
                if (!positions.has(node))
                    continue;
                const base::Vec3 point = positions.position(node);
                for (std::size_t axis = 0; axis < 3; ++axis)
                {
                    low[axis] = std::min(low[axis], point[axis]);
                    high[axis] = std::max(high[axis], point[axis]);
                }
                any = true;
            }
            if (!any)
                continue;
            for (std::size_t axis = 0; axis < 3; ++axis)
            {
                box[axis][at] = low[axis];
                box[3 + axis][at] = high[axis];
            }
        }
        const std::shared_ptr<const std::vector<std::uint64_t>> stamps =
            analysis::RevisionStamp::of(model, contact, run.context.pool());
        const analysis::ConnectionClusters clusters =
            analysis::ConnectionClusters::build(model, *components, run.context.pool());

        std::vector<std::int64_t> id(count);
        std::vector<std::int64_t> elementCount(count);
        std::vector<std::int64_t> nodeCount(count);
        std::vector<std::int64_t> shellCount(count);
        std::vector<std::int64_t> solidCount(count);
        std::vector<std::int64_t> lineCount(count);
        std::vector<std::int64_t> pointCount(count);
        std::vector<std::int64_t> propertyOffsets{0};
        std::vector<std::int32_t> propertyIds;
        std::vector<std::int64_t> revision(count);
        std::vector<std::int64_t> interfaceCount(count);
        std::vector<std::uint8_t> lineOnly(count);
        std::vector<std::int64_t> linked(count);
        std::vector<std::int64_t> contactPairs(count);
        std::vector<std::int64_t> bodyOffsets{0};
        std::vector<std::int32_t> bodyIds;
        for (std::size_t at = 0; at < count; ++at)
        {
            const analysis::ComponentSummary& summary = components->components()[at];
            id[at] = summary.id;
            elementCount[at] = static_cast<std::int64_t>(summary.elementCount);
            nodeCount[at] = static_cast<std::int64_t>(summary.nodeCount);
            shellCount[at] = static_cast<std::int64_t>(summary.shellCount);
            solidCount[at] = static_cast<std::int64_t>(summary.solidCount);
            lineCount[at] = static_cast<std::int64_t>(summary.lineCount);
            pointCount[at] = static_cast<std::int64_t>(summary.pointCount);
            std::sort(properties[at].begin(), properties[at].end());
            properties[at].erase(std::unique(properties[at].begin(), properties[at].end()), properties[at].end());
            propertyIds.insert(propertyIds.end(), properties[at].begin(), properties[at].end());
            propertyOffsets.push_back(static_cast<std::int64_t>(propertyIds.size()));
            revision[at] = asInt64((*stamps)[at]);
            interfaceCount[at] = static_cast<std::int64_t>(components->interfaceNodes(summary.id).size());
            lineOnly[at] = summary.shellCount == 0 && summary.solidCount == 0 ? 1 : 0;
            linked[at] = static_cast<std::int64_t>(clusters.linkedComponents(*components, summary.id).size());
            const std::vector<std::uint32_t> joins = components->contactJoinsOf(summary.id);
            contactPairs[at] = static_cast<std::int64_t>(joins.size());
            std::vector<std::int32_t> bodies;
            for (const std::uint32_t join : joins)
            {
                const analysis::ContactPair& pair =
                    components->contactSearch()->pairs()[components->contactJoins()[join].pair];
                bodies.push_back(pair.secondary->id);
                bodies.push_back(pair.primary->id);
            }
            std::sort(bodies.begin(), bodies.end());
            bodies.erase(std::unique(bodies.begin(), bodies.end()), bodies.end());
            bodyIds.insert(bodyIds.end(), bodies.begin(), bodies.end());
            bodyOffsets.push_back(static_cast<std::int64_t>(bodyIds.size()));
        }
        builder.int64("componentId", std::move(id));
        builder.int64("elementCount", std::move(elementCount));
        builder.int64("nodeCount", std::move(nodeCount));
        builder.int64("shellCount", std::move(shellCount));
        builder.int64("solidCount", std::move(solidCount));
        builder.int64("lineCount", std::move(lineCount));
        builder.int64("pointCount", std::move(pointCount));
        builder.intList("propertyIds", std::move(propertyOffsets), std::move(propertyIds));
        builder.real("mass", std::move(mass));
        static const char* const NAMES[6] = {"xmin", "ymin", "zmin", "xmax", "ymax", "zmax"};
        for (std::size_t item = 0; item < 6; ++item)
            builder.real(NAMES[item], std::move(box[item]));
        builder.int64("revision", std::move(revision));
        builder.int64("interfaceNodeCount", std::move(interfaceCount));
        builder.flag("isLineOnly", lineOnly);
        builder.int64("linkedComponentCount", std::move(linked));
        builder.int64("contactPairCount", std::move(contactPairs));
        builder.intList("contactBodies", std::move(bodyOffsets), std::move(bodyIds));
        return builder.finish();
    }

    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "connectedComponents";
        info.family = "T4";
        info.description =
            "The components of the model: sets of 2D and 3D elements connected through common nodes, plus the masses "
            "on "
            "their nodes and the 1D elements attached to them. Connectors, rigid elements, MPC equations, and 1D "
            "elements or "
            "masses that bridge two components are connections and belong to none. Grid points that rigid elements of "
            "zero length weld in all six components (one element or several) are one point: the meshes they join are "
            "one component, and the welding elements are members of it (not counted), not connections. Components are "
            "numbered from 1 by "
            "decreasing element count, ties by the smallest element ID. detail=\"components\": one row each, with the "
            "counts, the properties, the mass, the box, the revision stamp (a 64-bit hash of the cards of the "
            "component and "
            "of what they depend on, shown as a signed int64: equal content gives an equal stamp) and the number of "
            "interface nodes, isLineOnly (only 1D and 0D elements) and linkedComponentCount (the other components "
            "its connections link it to, directly or through connections joined at points only connections use: "
            "connectionClusters). detail=\"elements\": the component of every element (null for a connection). "
            "detail=\"nodes\": every grid point of the model and every point an element uses, with its component "
            "(null for a point no component holds) and isConnectionOnly (a point only connections use; a CFAST or "
            "CWELD uses the grid points of the shells it pierces, not GS). Contact between bodies (every contact "
            "table of the Bulk Data, or every two bodies when it has none): contact=\"merge\" makes bodies whose "
            "surfaces meet one component, \"connection\" makes each pair that meets a connection on its node pairs "
            "(contactNodePairs), "
            "\"ignore\" leaves contact out; contactPairCount and contactBodies are the pairs that act on the "
            "component (merged into it, or connections on its grid points) and their body IDs. connections=[elementId, "
            "...] "
            "declares elements connections whatever their type (a CBAR to a free point, a CELAS "
            "chain, a shell strip): part of no component, connections of the kind of their family when it is one "
            "(rigid, fastener, connector), else of kind connector in connectionClusters; the components are made of "
            "the other elements by the same rule and numbered by it, their revision stamps those of their content "
            "(a component the list does not change keeps its stamp to the bit); a declared rigid element of zero "
            "length welds nothing. Every component query takes the same argument.";
        info.detailParameter = "detail";
        info.parameters = {
            parameter("detail", ParameterKind::Text, "\"components\"", "\"components\", \"elements\" or \"nodes\"")};
        const std::vector<ParameterInfo> contact = contactParameters();
        info.parameters.insert(info.parameters.end(), contact.begin(), contact.end());
        info.defaultSort = "componentId";
        ColumnListBuilder components;
        components.int64("componentId")
            .int64("elementCount")
            .int64("nodeCount")
            .int64("shellCount")
            .int64("solidCount");
        components.int64("lineCount").int64("pointCount").intList("propertyIds").real("mass");
        components.reals("xmin ymin zmin xmax ymax zmax");
        components.int64("revision").int64("interfaceNodeCount").flag("isLineOnly").int64("linkedComponentCount");
        components.int64("contactPairCount").intList("contactBodies");
        info.columnSets.push_back(components.build("components"));
        ColumnListBuilder elements;
        elements.int32("elementId").dictionary("type").int32("propId").int64("componentId");
        info.columnSets.push_back(elements.build("elements"));
        ColumnListBuilder nodes;
        addComponentNodeColumns(nodes);
        info.columnSets.push_back(nodes.build("nodes"));
        return info;
    }
};

} // namespace

void addConnectedComponentsQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<ConnectedComponentsQuery>());
}

} // namespace oor::query
