#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/solution/SolutionChecks.hpp"

namespace oor::analysis
{

namespace
{

// A deck with Executive Control (it has a CEND) and no SOL statement: the solver does not know which analysis to run.
class MissingSolutionSequenceCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"solution.missing-solution-sequence",
                                    base::Severity::Warning,
                                    "solution",
                                    "The deck has an Executive Control section and no SOL statement.",
                                    false,
                                    CheckScope::Deck};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::ControlSections& control = context.root().control();
        if (control.executive.isEmpty() || control.executive.findLast("SolutionSequence") != nullptr)
            return;
        sink.reportStatement("SolutionSequence", 0, "the executive section has no SolutionSequence record",
                             "add a SolutionSequence record with the required solution");
    }
};

} // namespace

void addMissingSolutionSequenceCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<MissingSolutionSequenceCheck>());
}

} // namespace oor::analysis
