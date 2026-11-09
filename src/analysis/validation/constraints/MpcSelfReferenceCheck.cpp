#include "oor/analysis/validation/constraints/ConstraintChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/constraints/multiPointConstr/MultiPointConstrTable.hpp"

namespace oor::analysis
{

namespace
{

// An MPC equation whose dependent degree of freedom (the first term) appears again among its independent terms.
class MpcSelfReferenceCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"constraint.mpc-self-reference", base::Severity::Error, "constraints",
                                    "The dependent degree of freedom of a multipoint constraint appears again in the "
                                    "same equation."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        forEachCard<model::MultiPointConstrTable>(
            context.model(),
            [&](const model::MultiPointConstrCard& card, model::CardLocation where)
            {
                for (std::size_t index = 1; index < card.terms.size(); ++index)
                {
                    if (card.terms[index].node == card.terms[0].node &&
                        card.terms[index].component == card.terms[0].component)
                    {
                        sink.reportCard(
                            where, "node",
                            "term " + std::to_string(index + 1) + " is the dependent degree of freedom of the equation",
                            "remove the term, or use another dependent degree of freedom");
                        return;
                    }
                }
            });
    }
};

} // namespace

void addMpcSelfReferenceCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<MpcSelfReferenceCheck>());
}

} // namespace oor::analysis
