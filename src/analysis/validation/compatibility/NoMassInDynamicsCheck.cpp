#include "oor/analysis/mass/MassCalculator.hpp"
#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/base/errors/OorError.hpp"

namespace oor::analysis
{

namespace
{

// A model with elements and not one gram of mass, analysed by a dynamic solution (modes, frequency or transient
// response): the response has no inertia.
class NoMassInDynamicsCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"compat.no-mass-in-dynamics",
                                    base::Severity::Warning,
                                    "compatibility",
                                    "The SOL is a dynamic solution and the model has no mass.",
                                    false,
                                    CheckScope::Deck};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const std::optional<std::int64_t> solution = context.solution();
        if (!solution || !isDynamicSolution(*solution))
            return;
        const model::Model& model = context.model();
        try
        {
            const std::vector<RowSet> sets = MassCalculator::allElements(model);
            const MassTotals totals = MassCalculator::total(model, sets);
            // An element whose mass the library does not compute (a CCONEAX, the fluid elements of a hydroelastic
            // model, an element on GRIDB points: MassTotals::excluded) may carry the mass: the model is not judged.
            if (totals.elements > 0 && totals.excluded.empty() && !(totals.sums.mass() > 0.0))
                sink.reportStatement("SolutionSequence", *solution,
                                     "the model has " + std::to_string(totals.elements) +
                                         " elements and no mass: a dynamic analysis has no inertia",
                                     "give the materials a density, or add concentrated masses");
        }
        catch (const base::OorError&)
        {
            // A material or grid point that is not there, a section that is impossible: the other checks say so.
        }
    }
};

} // namespace

void addNoMassInDynamicsCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<NoMassInDynamicsCheck>());
}

} // namespace oor::analysis
