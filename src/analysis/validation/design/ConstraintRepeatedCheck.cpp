#include "oor/analysis/validation/design/DesignChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/optimisation/designConstr/DesignConstrTable.hpp"

#include <set>
#include <utility>

namespace oor::analysis
{

namespace
{

// in one set of constraints (one DCID) a response is bounded once. The second DCONSTR is reported.
class ConstraintRepeatedCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"design.constraint-repeated", base::Severity::Error, "design",
                                    "A set of design constraints bounds the same response twice."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        std::set<std::pair<base::EntityId, base::EntityId>> seen;
        forEachCard<model::DesignConstrTable>(context.model(),
                                              [&](const model::DesignConstrCard& card, model::CardLocation where)
                                              {
                                                  if (!seen.emplace(card.constraintSetId, card.responseId).second)
                                                      sink.reportCard(where, "responseId",
                                                                      "response " + std::to_string(card.responseId) +
                                                                          " is bounded twice in the constraint set " +
                                                                          std::to_string(card.constraintSetId),
                                                                      "merge the two bounds into one " +
                                                                          std::string("DesignConstr") +
                                                                          ", or put them in different sets");
                                              });
    }
};

} // namespace

void addConstraintRepeatedCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<ConstraintRepeatedCheck>());
}

} // namespace oor::analysis
