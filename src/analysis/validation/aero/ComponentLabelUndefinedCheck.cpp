#include "AeroCheckSupport.hpp"
#include "oor/analysis/validation/aero/AeroChecks.hpp"
#include "oor/analysis/validation/engine/ValidationContext.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/aero/aeroComponentUnion/AeroComponentUnionTable.hpp"

#include <string>
#include <unordered_set>

namespace oor::analysis
{

namespace
{

// An AECOMPL joins components by name: a name that no AECOMP or AECOMPL has joins
// nothing.
class ComponentLabelUndefinedCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"aero.component-label-undefined", base::Severity::Error, "aero",
                                    "An aerodynamic component union joins a component that no aerodynamic component or "
                                    "component union defines."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const std::unordered_set<std::string> names = componentNames(context.model());
        forEachCard<model::AeroComponentUnionTable>(
            context.model(),
            [&](const model::AeroComponentUnionCard& card, model::CardLocation where)
            {
                for (const std::string& label : card.labels)
                {
                    if (!names.contains(std::string(label)))
                        sink.reportCard(where, "labels",
                                        "the component " + label + " is defined by no " + std::string("AeroComponent") +
                                            " or " + std::string("AeroComponentUnion"),
                                        "define the component, or correct the label");
                }
            });
    }
};

} // namespace

void addComponentLabelUndefinedCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<ComponentLabelUndefinedCheck>());
}

} // namespace oor::analysis
