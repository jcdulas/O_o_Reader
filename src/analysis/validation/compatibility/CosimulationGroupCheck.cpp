#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/solution_control/cosimulationService/CosimulationServiceTable.hpp"

#include <string>

namespace oor::analysis
{

namespace
{

// A COSMSRV that receives its input in one coupled area and gives its output in another: the format says that
// GRPID1 should be the same as GRPID2 at present, and that one set of coupled area is
// supported. The card reads both numbers as they are; this is the rule of the model.
class CosimulationGroupCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"compat.cosimulation-group-differ", base::Severity::Warning, "compatibility",
                                    "A co-simulation service takes its input from one coupled area and gives its "
                                    "output to another."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        forEachCard<model::CosimulationServiceTable>(
            context.model(),
            [&](const model::CosimulationServiceCard& card, model::CardLocation where)
            {
                if (card.inputAreaId == card.outputAreaId)
                    return;
                sink.reportCard(where, "outputAreaId",
                                "receives its input in the group " + std::to_string(card.inputAreaId) +
                                    " and gives its output in the group " + std::to_string(card.outputAreaId) +
                                    ", and the two must be the same group",
                                "give " + std::string("inputAreaId") + " and " + std::string("outputAreaId") +
                                    " the same " + std::string("CosimulationArea"));
            });
    }
};

} // namespace

void addCosimulationGroupCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<CosimulationGroupCheck>());
}

} // namespace oor::analysis
