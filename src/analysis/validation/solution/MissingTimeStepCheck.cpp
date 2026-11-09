#include "SolutionRules.hpp"
#include "oor/analysis/validation/solution/SolutionChecks.hpp"

namespace oor::analysis
{

namespace
{

// A transient solution (SOL 109, 112, 129) with no TSTEP or TSTEPNL command: no time steps.
class MissingTimeStepCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"solution.missing-time-step",
                                    base::Severity::Error,
                                    "solution",
                                    "SOL 109, 112 or 129 and no TSTEP (or TSTEPNL) command in any subcase.",
                                    false,
                                    CheckScope::Deck};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        reportMissingSelection(context, sink, {109, 112, 129}, {"TimeStepSelection", "NonlinearTimeStepSelection"},
                               "the time steps");
    }
};

} // namespace

void addMissingTimeStepCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<MissingTimeStepCheck>());
}

} // namespace oor::analysis
