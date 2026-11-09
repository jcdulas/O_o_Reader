#include "ConstraintDofs.hpp"

#include "oor/analysis/validation/constraints/ConstraintChecks.hpp"

#include <algorithm>
#include <set>
#include <unordered_map>

namespace oor::analysis
{

namespace
{

// Rigid elements that make a degree of freedom depend on itself through a chain of other degrees of freedom (an RBE2
// makes the translation of A depend on B, another makes B depend on A). The degrees of freedom are the vertices of a
// graph, each link of a rigid element an edge from the dependent degree of freedom to an independent one; a link inside
// a strongly connected set of more than one vertex (or from a vertex to itself) closes a loop.
class RigidLoopCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "constraint.rigid-loop", base::Severity::Error, "constraints",
        "Rigid elements make a degree of freedom depend on itself through a loop of links."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const std::vector<DofLink> links = rigidDofLinks(context.model());
        if (links.empty())
            return;
        std::unordered_map<std::uint64_t, int> vertex;
        for (const DofLink& each : links)
        {
            vertex.emplace(each.dependent, static_cast<int>(vertex.size()));
            vertex.emplace(each.independent, static_cast<int>(vertex.size()));
        }
        const std::size_t count = vertex.size();
        std::vector<std::vector<int>> edges(count);
        for (const DofLink& each : links)
            edges[static_cast<std::size_t>(vertex.at(each.dependent))].push_back(vertex.at(each.independent));
        const std::vector<int> component = stronglyConnected(edges);
        std::vector<int> size(count, 0);
        for (const int id : component)
            ++size[static_cast<std::size_t>(id)];
        std::set<std::pair<std::uint32_t, std::uint32_t>> reported;
        for (const DofLink& each : links)
        {
            const int from = vertex.at(each.dependent);
            const int to = vertex.at(each.independent);
            const bool loops =
                component[static_cast<std::size_t>(from)] == component[static_cast<std::size_t>(to)] &&
                (from == to || size[static_cast<std::size_t>(component[static_cast<std::size_t>(from)])] > 1);
            if (!loops || !reported.emplace(each.where.table, each.where.row).second)
                continue;
            sink.reportCard(each.where, "",
                            "degree of freedom " + std::to_string(componentOfDof(each.dependent)) + " of grid point " +
                                std::to_string(nodeOfDof(each.dependent)) + " depends on degree of freedom " +
                                std::to_string(componentOfDof(each.independent)) + " of grid point " +
                                std::to_string(nodeOfDof(each.independent)) + ", which depends on it in turn",
                            "break the loop: a degree of freedom cannot depend on itself");
        }
    }

private:
    // The strongly connected component of each vertex (Tarjan's algorithm, without recursion: a chain of rigid elements
    // can be long).
    static std::vector<int> stronglyConnected(const std::vector<std::vector<int>>& edges)
    {
        const std::size_t count = edges.size();
        std::vector<int> order(count, -1);
        std::vector<int> low(count, 0);
        std::vector<int> component(count, -1);
        std::vector<char> onStack(count, 0);
        std::vector<int> stack;
        int counter = 0;
        int components = 0;
        struct Frame
        {
            int vertex;
            std::size_t next;
        };
        for (std::size_t root = 0; root < count; ++root)
        {
            if (order[root] >= 0)
                continue;
            std::vector<Frame> calls{{static_cast<int>(root), 0}};
            order[root] = low[root] = counter++;
            stack.push_back(static_cast<int>(root));
            onStack[root] = 1;
            while (!calls.empty())
            {
                Frame& frame = calls.back();
                const auto vertex = static_cast<std::size_t>(frame.vertex);
                if (frame.next < edges[vertex].size())
                {
                    const int target = edges[vertex][frame.next++];
                    const auto next = static_cast<std::size_t>(target);
                    if (order[next] < 0)
                    {
                        order[next] = low[next] = counter++;
                        stack.push_back(target);
                        onStack[next] = 1;
                        calls.push_back(Frame{target, 0});
                    }
                    else if (onStack[next] != 0)
                    {
                        low[vertex] = std::min(low[vertex], order[next]);
                    }
                    continue;
                }
                if (low[vertex] == order[vertex])
                {
                    int member = -1;
                    while (member != frame.vertex)
                    {
                        member = stack.back();
                        stack.pop_back();
                        onStack[static_cast<std::size_t>(member)] = 0;
                        component[static_cast<std::size_t>(member)] = components;
                    }
                    ++components;
                }
                const int finished = frame.vertex;
                calls.pop_back();
                if (!calls.empty())
                {
                    const auto parent = static_cast<std::size_t>(calls.back().vertex);
                    low[parent] = std::min(low[parent], low[static_cast<std::size_t>(finished)]);
                }
            }
        }
        return component;
    }
};

} // namespace

void addRigidLoopCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<RigidLoopCheck>());
}

} // namespace oor::analysis
