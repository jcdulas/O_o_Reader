#include "SolutionRules.hpp"
#include "oor/analysis/validation/solution/SolutionChecks.hpp"

namespace oor::analysis
{

namespace
{

// A normal modes, buckling or modal response solution (SOL 103, 105, 111, 112) with no METHOD command: no EIGRL is
// selected, so the eigenvalue problem has no method.
class MissingMethodCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"solution.missing-method",
                                    base::Severity::Error,
                                    "solution",
                                    "SOL 103, 105, 111 or 112 and no METHOD command in any subcase.",
                                    false,
                                    CheckScope::Deck};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        reportMissingSelection(context, sink, {103, 105, 111, 112}, {"EigenMethodSelection"},
                               "the " + std::string("LanczosEigenMethod") + " to use");
    }
};

} // namespace

void addMissingMethodCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<MissingMethodCheck>());
}

} // namespace oor::analysis
