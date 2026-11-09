#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/solution/SolutionChecks.hpp"
#include "oor/model/cards/solution_control/nonlinearTimeSteps/NonlinearTimeStepsTable.hpp"
#include "oor/model/cards/solution_control/transientTimeSteps/TransientTimeStepsTable.hpp"

namespace oor::analysis
{

namespace
{

// A TSTEP interval (or a TSTEPNL) that outputs every NO-th step with NO above the number of steps: nothing of it is
// output.
class TransientTimeStepsOutputIntervalCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"solution.transient-time-steps-output-interval", base::Severity::Warning,
                                    "solution",
                                    "An interval of transient time steps, or a nonlinear time step card, outputs every "
                                    "n-th step with n above the number of steps."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        forEachCard<model::TransientTimeStepsTable>(
            context.model(),
            [&](const model::TransientTimeStepsCard& card, model::CardLocation where)
            {
                for (std::size_t index = 0; index < card.intervals.size(); ++index)
                {
                    const model::TransientTimeStepsInterval& interval = card.intervals[index];
                    if (interval.outputInterval > interval.stepCount)
                    {
                        sink.reportCard(
                            where, "outputInterval",
                            "interval " + std::to_string(index + 1) + " has " + std::to_string(interval.stepCount) +
                                " steps and outputs every " + std::to_string(interval.outputInterval) + "th",
                            "use " + std::string("outputInterval") + " at most equal to the number of steps");
                        return;
                    }
                }
            });
        forEachCard<model::NonlinearTimeStepsTable>(
            context.model(),
            [&](const model::NonlinearTimeStepsCard& card, model::CardLocation where)
            {
                if (card.stepCount > 0 && card.outputInterval > card.stepCount)
                    sink.reportCard(
                        where, "outputInterval",
                        "there are " + std::to_string(card.stepCount) + " steps and every " +
                            std::to_string(card.outputInterval) + "th is output",
                        "use " + std::string("outputInterval") + " at most equal to " + std::string("stepCount"));
            });
    }
};

} // namespace

void addTransientTimeStepsOutputIntervalCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<TransientTimeStepsOutputIntervalCheck>());
}

} // namespace oor::analysis
