#include "AeroCheckSupport.hpp"
#include "oor/analysis/validation/aero/AeroChecks.hpp"
#include "oor/analysis/validation/engine/ValidationContext.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/matrices/monitorDisplacement/MonitorDisplacementTable.hpp"
#include "oor/model/cards/matrices/monitorIntegratedLoad1/MonitorIntegratedLoad1Table.hpp"

#include <string>
#include <unordered_set>

namespace oor::analysis
{

namespace
{

// The COMP of a MONPNT1 or a MONDSP1 is the name of the AECOMP or AECOMPL that says which grids or boxes the point
// sums. A blank COMP is not judged.
class MonitorComponentUndefinedCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"aero.monitor-component-undefined", base::Severity::Error, "aero",
                                    "A monitor point of integrated loads or of displacements names a component that no "
                                    "aerodynamic component or component union defines."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const std::unordered_set<std::string> names = componentNames(context.model());
        const auto look = [&](const std::string& component, model::CardLocation where)
        {
            if (!component.empty() && !names.contains(std::string(component)))
                sink.reportCard(where, "component",
                                "the component " + component + " is defined by no " + std::string("AeroComponent") +
                                    " or " + std::string("AeroComponentUnion"),
                                "define the component, or correct the name");
        };
        forEachCard<model::MonitorIntegratedLoad1Table>(
            context.model(), [&](const model::MonitorIntegratedLoad1Card& card, model::CardLocation where)
            { look(card.component, where); });
        forEachCard<model::MonitorDisplacementTable>(
            context.model(), [&](const model::MonitorDisplacementCard& card, model::CardLocation where)
            { look(card.component, where); });
    }
};

} // namespace

void addMonitorComponentUndefinedCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<MonitorComponentUndefinedCheck>());
}

} // namespace oor::analysis
