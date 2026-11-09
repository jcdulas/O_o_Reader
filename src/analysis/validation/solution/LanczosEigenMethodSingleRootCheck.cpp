#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/solution/SolutionChecks.hpp"
#include "oor/model/cards/dynamics/lanczosEigenMethod/LanczosEigenMethodTable.hpp"

namespace oor::analysis
{

namespace
{

// An EIGRL with no V1, no V2 and no ND: the format reference gives the lowest root only, which is rarely what was
// meant.
class LanczosEigenMethodSingleRootCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"solution.lanczos-eigen-method-single-root", base::Severity::Warning, "solution",
                                    "A Lanczos eigenvalue method has no frequency range and no number of roots: it "
                                    "extracts the lowest root only."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        const std::string lower = std::string("lowerFrequency");
        const std::string upper = std::string("upperFrequency");
        const std::string count = std::string("desiredModeCount");
        forEachCard<model::LanczosEigenMethodTable>(
            model,
            [&](const model::LanczosEigenMethodCard& card, model::CardLocation where)
            {
                if (!card.lowerFrequency && !card.upperFrequency && card.desiredModeCount == 0)
                    sink.reportCard(
                        where, "desiredModeCount",
                        lower + ", " + upper + " and " + count + " are blank: only the lowest root is found",
                        "give " + count + " (the number of roots) or the range " + lower + ", " + upper);
            });
    }
};

} // namespace

void addLanczosEigenMethodSingleRootCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<LanczosEigenMethodSingleRootCheck>());
}

} // namespace oor::analysis
