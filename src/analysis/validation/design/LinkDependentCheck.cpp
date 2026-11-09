#include "oor/analysis/validation/design/DesignChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/optimisation/designVarLink/DesignVarLinkTable.hpp"

#include <set>

namespace oor::analysis
{

namespace
{

// a design variable that depends on a DLINK cannot be independent in another one. The check reads it
// for the card of the independent variable, and so also finds a DLINK that lists its own dependent variable among the
// independent ones (the variable would depend on itself).
class LinkDependentCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"design.link-dependent", base::Severity::Error, "design",
                                    "A design variable is both independent and dependent in design variable links."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        std::set<base::EntityId> dependent;
        forEachCard<model::DesignVarLinkTable>(context.model(),
                                               [&](const model::DesignVarLinkCard& card, model::CardLocation)
                                               { dependent.insert(card.dependent); });
        forEachCard<model::DesignVarLinkTable>(
            context.model(),
            [&](const model::DesignVarLinkCard& card, model::CardLocation where)
            {
                for (const model::DesignTerm& term : card.terms)
                {
                    if (dependent.contains(term.designVarId))
                        sink.reportCard(where, "independentIds",
                                        "design variable " + std::to_string(term.designVarId) +
                                            " is independent here and dependent in a " + std::string("DesignVarLink"),
                                        "link the variables to one dependent variable at a time: replace it by the "
                                        "variables it depends on");
                }
            });
    }
};

} // namespace

void addLinkDependentCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<LinkDependentCheck>());
}

} // namespace oor::analysis
