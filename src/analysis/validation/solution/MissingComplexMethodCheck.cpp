#include "SolutionRules.hpp"
#include "oor/analysis/validation/solution/SolutionChecks.hpp"

namespace oor::analysis
{

namespace
{

// A complex eigenvalue solution (SOL 107, 110) with no CMETHOD command.
class MissingComplexMethodCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"solution.missing-complex-method",
                                    base::Severity::Error,
                                    "solution",
                                    "SOL 107 or 110 and no CMETHOD command in any subcase.",
                                    false,
                                    CheckScope::Deck};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        reportMissingSelection(context, sink, {107, 110}, {"ComplexEigenMethodSelection"},
                               "the " + std::string("ComplexEigenMethod") + " to use");
    }
};

} // namespace

void addMissingComplexMethodCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<MissingComplexMethodCheck>());
}

} // namespace oor::analysis
