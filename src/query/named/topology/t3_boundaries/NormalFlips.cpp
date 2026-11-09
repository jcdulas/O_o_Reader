#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/ShellEdges.hpp"
#include "oor/query/named/topology/t3_boundaries/BoundaryQueries.hpp"

namespace oor::query
{

namespace
{

// normalFlips: the neighbouring shells that run their common edge in the same direction.
class NormalFlipsQuery final : public QueryBase
{
public:
    NormalFlipsQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const ShellSet shells(run, run.arguments.ids("elementIds"), ShellNeeds{});
        const ShellEdges edges(run, shells);
        std::vector<std::int32_t> n1;
        std::vector<std::int32_t> n2;
        std::vector<std::int32_t> eid1;
        std::vector<std::int32_t> eid2;
        std::vector<std::int32_t> pid1;
        std::vector<std::int32_t> pid2;
        for (const ShellEdge& edge : edges.edges())
        {
            if (edge.count != 2)
                continue;
            const analysis::EdgeKey key = edges.key(edge);
            const auto pair = edges.shellsOf(edge);
            const bool firstForward = shells.runsForward(pair[0], key.first, key.second);
            const bool secondForward = shells.runsForward(pair[1], key.first, key.second);
            const bool firstBackward = shells.runsForward(pair[0], key.second, key.first);
            const bool secondBackward = shells.runsForward(pair[1], key.second, key.first);
            // Consistent shells go in opposite directions along the edge they share; the same direction is a flip.
            const bool same = (firstForward && secondForward) || (firstBackward && secondBackward);
            if (!same)
                continue;
            n1.push_back(key.first);
            n2.push_back(key.second);
            eid1.push_back(shells.id(pair[0]));
            eid2.push_back(shells.id(pair[1]));
            pid1.push_back(shells.property(pair[0]));
            pid2.push_back(shells.property(pair[1]));
        }
        std::vector<std::uint8_t> valid1(pid1.size());
        std::vector<std::uint8_t> valid2(pid2.size());
        for (std::size_t index = 0; index < pid1.size(); ++index)
        {
            valid1[index] = pid1[index] != 0 ? 1 : 0;
            valid2[index] = pid2[index] != 0 ? 1 : 0;
        }
        TableBuilder builder;
        builder.int32("n1", std::move(n1));
        builder.int32("n2", std::move(n2));
        builder.int32("elementId1", std::move(eid1));
        builder.int32("elementId2", std::move(eid2));
        builder.int32("propId1", std::move(pid1), std::move(valid1));
        builder.int32("propId2", std::move(pid2), std::move(valid2));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "normalFlips";
        info.family = "T3";
        info.description =
            "One row for each edge between exactly two shells whose two shells run it in the same direction (n1 -> n2 "
            "or "
            "n2 -> n1 in both): the orientation of the pair is inconsistent, one of them has a reversed normal. "
            "elementId1 < "
            "elementId2, "
            "n1 < n2. With elementIds only those shells are looked at.";
        info.parameters = {parameter("elementIds", ParameterKind::IdList, "None", "only these elements are looked at")};
        info.defaultSort = "(edge order: by n1, n2)";
        ColumnListBuilder columns;
        columns.int32("n1").int32("n2").int32("elementId1").int32("elementId2").int32("propId1").int32("propId2");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addNormalFlipsQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<NormalFlipsQuery>());
}

} // namespace oor::query
