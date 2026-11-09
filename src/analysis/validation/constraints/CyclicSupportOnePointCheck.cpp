#include "oor/analysis/validation/constraints/ConstraintChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/constraints/cyclicSupport/CyclicSupportTable.hpp"

#include <optional>
#include <string>

namespace oor::analysis
{

namespace
{

// CYSUP cards that name more than one grid point: the fictitious supports of a cyclic symmetry model are applied at one
// grid point (a model has at most one such point), all the
// degrees of freedom of which are on one card or on several cards of the same point.
class CyclicSupportOnePointCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"constraint.cyclic-support-one-point", base::Severity::Error, "constraints",
                                    "The cyclic support cards of a model name more than one grid point."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        std::optional<base::EntityId> first;
        forEachCard<model::CyclicSupportTable>(
            context.model(),
            [&](const model::CyclicSupportCard& card, model::CardLocation where)
            {
                if (!first)
                {
                    first = card.nodeId;
                    return;
                }
                if (card.nodeId != *first)
                    sink.reportCard(where, "nodeId",
                                    "the support is at the grid point " + std::to_string(card.nodeId) +
                                        " and an earlier " + std::string("CyclicSupport") + " is at " +
                                        std::to_string(*first),
                                    "support one grid point only");
            });
    }
};

} // namespace

void addCyclicSupportOnePointCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<CyclicSupportOnePointCheck>());
}

} // namespace oor::analysis
