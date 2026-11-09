#include "oor/analysis/validation/aero/AeroChecks.hpp"
#include "oor/analysis/validation/engine/ValidationContext.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/aero/aeroComponent/AeroComponentTable.hpp"
#include "oor/model/cards/aero/aeroComponentUnion/AeroComponentUnionTable.hpp"

#include <string>
#include <unordered_set>

namespace oor::analysis
{

namespace
{

// The names of the AECOMP and AECOMPL cards are one set: a
// monitor point or an AECOMPL finds a component by its name, compared without regard to the case.
class ComponentNameRepeatedCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"aero.component-name-repeated", base::Severity::Error, "aero",
                                    "An aerodynamic component or component union has the name of another component."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        std::unordered_set<std::string> seen;
        const auto look = [&](const std::string& name, std::string_view cardType, model::CardLocation where)
        {
            if (!seen.insert(std::string(name)).second)
            {
                sink.reportCard(where, "name",
                                "the component name " + name + " is already the name of a " +
                                    cardLabels(context.model(), {"AeroComponent", "AeroComponentUnion"}) + " (" +
                                    std::string(cardType) + ")",
                                "give each component its own name");
            }
        };
        forEachCard<model::AeroComponentTable>(context.model(),
                                               [&](const model::AeroComponentCard& card, model::CardLocation where)
                                               { look(card.name, "AeroComponent", where); });
        forEachCard<model::AeroComponentUnionTable>(
            context.model(), [&](const model::AeroComponentUnionCard& card, model::CardLocation where)
            { look(card.name, "AeroComponentUnion", where); });
    }
};

} // namespace

void addComponentNameRepeatedCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<ComponentNameRepeatedCheck>());
}

} // namespace oor::analysis
