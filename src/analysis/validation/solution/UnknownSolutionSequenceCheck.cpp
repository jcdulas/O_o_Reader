#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/solution/SolutionChecks.hpp"

namespace oor::analysis
{

namespace
{

// A SOL statement whose number or name is not a solution sequence of the format reference.
class UnknownSolutionSequenceCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"solution.unknown-solution-sequence",
                                    base::Severity::Warning,
                                    "solution",
                                    "The SOL statement names a solution sequence that is not in the format reference.",
                                    false,
                                    CheckScope::Deck};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::ExecutiveControl& executive = context.root().control().executive;
        if (executive.findLast("SolutionSequence") != nullptr && !executive.solutionNumber())
            sink.reportStatement("SolutionSequence", 0,
                                 "the solution sequence is not one of the format reference (a number such as 101, or a "
                                 "name such as SESTATIC)",
                                 "use a solution sequence of the format reference, or an alter for a user sequence");
    }
};

} // namespace

void addUnknownSolutionSequenceCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<UnknownSolutionSequenceCheck>());
}

} // namespace oor::analysis
