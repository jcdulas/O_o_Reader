#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/dynamics/frfAssemblyLoadDelay/FrfAssemblyLoadDelayTable.hpp"
#include "oor/model/cards/dynamics/frfAssemblyLoadPhase/FrfAssemblyLoadPhaseTable.hpp"
#include "oor/model/cards/dynamics/frfAssemblyLoadScale/FrfAssemblyLoadScaleTable.hpp"
#include "oor/model/cards/dynamics/frfComponent/FrfComponentTable.hpp"
#include "oor/model/cards/dynamics/frfConnection/FrfConnectionTable.hpp"
#include "oor/model/cards/dynamics/frfFlexibleConnection/FrfFlexibleConnectionTable.hpp"
#include "oor/model/cards/dynamics/frfReleasedDofs/FrfReleasedDofsTable.hpp"
#include "oor/model/cards/dynamics/frfSinglePointConstr/FrfSinglePointConstrTable.hpp"

#include <set>
#include <string>

namespace oor::analysis
{

namespace
{

// An FRF assembly card that names its component by name: the name must be the COMPNAME of an FRFCOMP. A component that
// is given by its ID is found (or not) by `ref.unresolved`, which cannot see a name: a name is not an ID, so no
// reference is visited for it. The match ignores the case, as the solver's names do. An FRFCONN, FRFFLEX,
// FRFRELS, FRFSPC1 or an FBA load card whose component is not part of the FBA process is ignored with a warning, so the
// finding is a warning too (it is the mistake that a misspelt name makes).
class FrfComponentNameCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"compat.frf-component-name", base::Severity::Warning, "compatibility",
                                    "An FRF assembly card names a component by a name that no FRF component card has."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        std::set<std::string> known; // the names of the FRFCOMP cards, in upper case
        forEachCard<model::FrfComponentTable>(model,
                                              [&](const model::FrfComponentCard& card, model::CardLocation)
                                              {
                                                  if (!card.name.empty())
                                                      known.insert(std::string(card.name));
                                              });

        const auto report = [&](const model::FrfComponentRef& component, model::CardLocation where, const char* field)
        {
            if (component.name.empty() || known.contains(std::string(component.name)))
                return;
            sink.reportCard(
                where, field,
                "the component \"" + component.name + "\" is the name of no " + std::string("FrfComponent") + " card",
                "give the " + std::string("componentName") + " of an " + std::string("FrfComponent") +
                    " card, or the " + std::string("id") + " of one");
        };

        forEachCard<model::FrfConnectionTable>(model,
                                               [&](const model::FrfConnectionCard& card, model::CardLocation where)
                                               {
                                                   report(card.points[0].component, where, "frfComponent1");
                                                   report(card.points[1].component, where, "frfComponent2");
                                               });
        forEachCard<model::FrfFlexibleConnectionTable>(
            model,
            [&](const model::FrfFlexibleConnectionCard& card, model::CardLocation where)
            {
                report(card.points[0].component, where, "frfComponent1");
                report(card.points[1].component, where, "frfComponent2");
            });
        forEachCard<model::FrfReleasedDofsTable>(model,
                                                 [&](const model::FrfReleasedDofsCard& card, model::CardLocation where)
                                                 {
                                                     report(card.points[0].component, where, "frfComponent1");
                                                     report(card.points[1].component, where, "frfComponent2");
                                                     report(card.points[2].component, where, "frfComponent3");
                                                 });
        forEachCard<model::FrfSinglePointConstrTable>(
            model,
            [&](const model::FrfSinglePointConstrCard& card, model::CardLocation where)
            {
                report(card.points[0].component, where, "frfComponent1");
                report(card.points[1].component, where, "frfComponent2");
                report(card.points[2].component, where, "frfComponent3");
            });
        forEachCard<model::FrfAssemblyLoadDelayTable>(
            model, [&](const model::FrfAssemblyLoadDelayCard& card, model::CardLocation where)
            { report(card.frfComponent, where, "frfComponent"); });
        forEachCard<model::FrfAssemblyLoadScaleTable>(
            model, [&](const model::FrfAssemblyLoadScaleCard& card, model::CardLocation where)
            { report(card.frfComponent, where, "frfComponent"); });
        forEachCard<model::FrfAssemblyLoadPhaseTable>(
            model, [&](const model::FrfAssemblyLoadPhaseCard& card, model::CardLocation where)
            { report(card.frfComponent, where, "frfComponent"); });
    }
};

} // namespace

void addFrfComponentNameCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<FrfComponentNameCheck>());
}

} // namespace oor::analysis
