#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/superelement/SuperelementChecks.hpp"
#include "oor/model/cards/superelements/superelementBoundarySearch/SuperelementBoundarySearchTable.hpp"
#include "oor/model/cards/superelements/superelementBulkOptions/SuperelementBulkOptionsTable.hpp"
#include "oor/model/cards/superelements/superelementExcludedNodes/SuperelementExcludedNodesTable.hpp"

#include <set>

namespace oor::analysis
{

namespace
{

// A SuperelementBoundarySearch or a SuperelementExcludedNodes for a superelement that is searched manually: both cards
// steer the automatic search of the boundary, and with METHOD = MANUAL on its SuperelementBulkOptions they are a fatal
// error.
class ManualSearchCardsCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"superelement.manual-search-cards", base::Severity::Error, "superelement",
                                    "A superelement boundary search or excluded points card names a superelement whose "
                                    "options ask for a manual boundary search."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        if (model.partInfo())
            return;
        std::set<base::EntityId> manual;
        forEachCard<model::SuperelementBulkOptionsTable>(
            model,
            [&](const model::SuperelementBulkOptionsCard& card, model::CardLocation)
            {
                if (card.method == model::SuperelementBulkOptionsMethod::Manual)
                    manual.insert(card.superelementId);
            });
        if (manual.empty())
            return;
        forEachCard<model::SuperelementBoundarySearchTable>(
            model, [&](const model::SuperelementBoundarySearchCard& card, model::CardLocation where)
            { reportIfManual(model, sink, manual, card.superelementIdA, where); });
        forEachCard<model::SuperelementExcludedNodesTable>(
            model, [&](const model::SuperelementExcludedNodesCard& card, model::CardLocation where)
            { reportIfManual(model, sink, manual, card.superelementIdA, where); });
    }

private:
    static void reportIfManual(const model::Model& model, FindingSink& sink, const std::set<base::EntityId>& manual,
                               base::EntityId seid, model::CardLocation where)
    {
        if (manual.contains(seid))
            sink.reportCard(where, "superelementIdA",
                            std::string(cardNameAt(model, where)) + " is for the automatic search and superelement " +
                                std::to_string(seid) + " is searched manually",
                            "connect the superelement with " + std::string("SuperelementConnection") + ", or leave " +
                                std::string("method") + " to AUTO");
    }
};

} // namespace

void addManualSearchCardsCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<ManualSearchCardsCheck>());
}

} // namespace oor::analysis
