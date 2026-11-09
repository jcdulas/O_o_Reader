#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/superelement/SuperelementChecks.hpp"
#include "oor/model/cards/superelements/superelementInteriorNodes/SuperelementInteriorNodesTable.hpp"

#include <algorithm>

namespace oor::analysis
{

namespace
{

// One run of points of an SESET card, with the superelement and the card it came from.
struct ListedRun
{
    base::EntityId first;
    base::EntityId last;
    base::EntityId seid;
    model::CardLocation where;
};

// A grid point that two SESET lists name: a point is interior to one superelement only. The
// lists are the runs as written, a THRU range included, so that a range over millions of points costs one
// comparison. The finding is on the later card of the pair, with the first point they share.
class NodeInTwoCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"superelement.node-in-two", base::Severity::Error, "superelement",
                                    "A grid point is named by two superelement interior point lists; a point is "
                                    "interior to one superelement only."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        if (model.partInfo())
            return; // an SESET in a part is ignored: superelement.card-in-part says so
        std::vector<ListedRun> runs;
        forEachCard<model::SuperelementInteriorNodesTable>(
            model,
            [&](const model::SuperelementInteriorNodesCard& card, model::CardLocation where)
            {
                for (const model::IdRun& run : card.grids)
                    runs.push_back(ListedRun{run.first, run.last, card.superelementId, where});
            });
        // In order of the first point, the run that reaches farthest so far is the only one a later run can meet.
        std::stable_sort(runs.begin(), runs.end(),
                         [](const ListedRun& a, const ListedRun& b) { return a.first < b.first; });
        const ListedRun* reaching = nullptr;
        for (const ListedRun& run : runs)
        {
            if (reaching != nullptr && run.first <= reaching->last)
            {
                const bool sameCard = run.where.table == reaching->where.table && run.where.row == reaching->where.row;
                sink.reportCard(run.where, "nodes",
                                "point " + std::to_string(run.first) + " is in the list of superelement " +
                                    std::to_string(run.seid) + " and in the list of superelement " +
                                    std::to_string(reaching->seid) + (sameCard ? " (the same card)" : ""),
                                "list a point in one " + std::string("SuperelementInteriorNodes") + " only");
            }
            if (reaching == nullptr || run.last > reaching->last)
                reaching = &run;
        }
    }
};

} // namespace

void addNodeInTwoCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<NodeInTwoCheck>());
}

} // namespace oor::analysis
