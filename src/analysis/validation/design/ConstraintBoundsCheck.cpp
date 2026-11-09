#include "oor/analysis/validation/design/DesignChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/base/text/MessageNumber.hpp"
#include "oor/model/cards/optimisation/designConstr/DesignConstrTable.hpp"

namespace oor::analysis
{

namespace
{

// A response cannot be at least LALLOW and at most UALLOW when LALLOW is above UALLOW: no design satisfies the
// constraint. A bound that is a table (TABLEDi, an integer in the card) is a function of frequency and is not compared.
class ConstraintBoundsCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"design.constraint-bounds", base::Severity::Error, "design",
                                    "A design constraint has a lower bound above its upper bound."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        forEachCard<model::DesignConstrTable>(
            context.model(),
            [&](const model::DesignConstrCard& card, model::CardLocation where)
            {
                if (card.lowerAllowableTableId == 0 && card.upperAllowableTableId == 0 &&
                    card.lowerAllowable > card.upperAllowable)
                    sink.reportCard(where, "lowerAllowable",
                                    "the lower bound " + base::MessageNumber::general(card.lowerAllowable) +
                                        " is above the upper bound " +
                                        base::MessageNumber::general(card.upperAllowable),
                                    "swap the bounds");
            });
    }
};

} // namespace

void addConstraintBoundsCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<ConstraintBoundsCheck>());
}

} // namespace oor::analysis
