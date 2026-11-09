#include "SolutionRules.hpp"
#include "oor/analysis/validation/solution/SolutionChecks.hpp"

namespace oor::analysis
{

namespace
{

// A frequency response solution (SOL 108, 111) with no FREQUENCY command: no frequencies to solve at.
class MissingFrequencyCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"solution.missing-frequency",
                                    base::Severity::Error,
                                    "solution",
                                    "SOL 108 or 111 and no FREQUENCY command in any subcase.",
                                    false,
                                    CheckScope::Deck};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        reportMissingSelection(context, sink, {108, 111}, {"FreqSetSelection"},
                               "the " + std::string("FreqList") + " set to solve at");
    }
};

} // namespace

void addMissingFrequencyCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<MissingFrequencyCheck>());
}

} // namespace oor::analysis
