#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/superelement/SuperelementChecks.hpp"
#include "oor/model/cards/superelements/superelementBulkOptions/SuperelementBulkOptionsTable.hpp"
#include "oor/model/cards/superelements/superelementConnection/SuperelementConnectionTable.hpp"

#include <set>

namespace oor::analysis
{

namespace
{

// A superelement whose boundary is searched manually (SEBULK METHOD = MANUAL) and that no SECONCT connects: with MANUAL
// the boundary is what the SECONCT cards say, so a superelement with none is joined
// to nothing.
class ManualWithoutConnectionCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"superelement.manual-without-superelement-connection", base::Severity::Error,
                                    "superelement",
                                    "The options of a superelement ask for a manual boundary search and no "
                                    "superelement connection names it."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        if (model.partInfo())
            return;
        std::set<base::EntityId> connected;
        forEachCard<model::SuperelementConnectionTable>(
            model,
            [&](const model::SuperelementConnectionCard& card, model::CardLocation)
            {
                connected.insert(card.superelementIdA);
                connected.insert(card.superelementIdB);
            });
        forEachCard<model::SuperelementBulkOptionsTable>(
            model,
            [&](const model::SuperelementBulkOptionsCard& card, model::CardLocation where)
            {
                if (card.method == model::SuperelementBulkOptionsMethod::Manual &&
                    !connected.contains(card.superelementId))
                    sink.reportCard(where, "method",
                                    "superelement " + std::to_string(card.superelementId) +
                                        " is searched manually and no " + std::string("SuperelementConnection") +
                                        " connects it",
                                    "add the " + std::string("SuperelementConnection") +
                                        " cards of the superelement, or leave " + std::string("method") + " to AUTO");
            });
    }
};

} // namespace

void addManualWithoutConnectionCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<ManualWithoutConnectionCheck>());
}

} // namespace oor::analysis
