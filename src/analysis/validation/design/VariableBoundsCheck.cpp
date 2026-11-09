#include "oor/analysis/validation/design/DesignChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/base/text/MessageNumber.hpp"
#include "oor/model/cards/optimisation/designVar/DesignVarTable.hpp"

namespace oor::analysis
{

namespace
{

// A design variable must satisfy XLB <= XINIT <= XUB (format reference, DESVAR: "XLB < XINIT < XUB"). The card reads a
// value that breaks it, because a model being built passes through such a state, but the optimiser refuses to start
// from it. A variable that has no bound (-1.0E20, 1.0E20, the defaults) cannot break the rule on that side.
class VariableBoundsCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "design.variable-bounds", base::Severity::Error, "design",
        "A design variable has an initial value outside its bounds, or a lower bound above its "
        "upper bound."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        forEachCard<model::DesignVarTable>(
            context.model(),
            [&](const model::DesignVarCard& card, model::CardLocation where)
            {
                if (card.lowerBound > card.upperBound)
                {
                    sink.reportCard(where, "lowerBound",
                                    "the lower bound " + base::MessageNumber::general(card.lowerBound) +
                                        " is above the upper bound " + base::MessageNumber::general(card.upperBound),
                                    "swap the bounds");
                }
                else if (card.initialValue < card.lowerBound)
                {
                    sink.reportCard(where, "initialValue",
                                    "the initial value " + base::MessageNumber::general(card.initialValue) +
                                        " is below the lower bound " + base::MessageNumber::general(card.lowerBound),
                                    "raise " + std::string("initialValue") + " to the lower bound or above, or lower " +
                                        std::string("lowerBound"));
                }
                else if (card.initialValue > card.upperBound)
                {
                    sink.reportCard(where, "initialValue",
                                    "the initial value " + base::MessageNumber::general(card.initialValue) +
                                        " is above the upper bound " + base::MessageNumber::general(card.upperBound),
                                    "lower " + std::string("initialValue") + " to the upper bound or below, or raise " +
                                        std::string("upperBound"));
                }
            });
    }
};

} // namespace

void addVariableBoundsCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<VariableBoundsCheck>());
}

} // namespace oor::analysis
