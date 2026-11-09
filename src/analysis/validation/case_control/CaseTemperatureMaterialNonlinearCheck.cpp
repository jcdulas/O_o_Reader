#include "CaseRules.hpp"
#include "oor/analysis/validation/case_control/CaseControlChecks.hpp"
#include "oor/model/control/case_control/CaseLineages.hpp"

namespace oor::analysis
{

namespace
{

// TEMPERATURE(MATERIAL) or TEMPERATURE(BOTH) in a nonlinear static analysis (SOL 106):
// either form stops the run with a fatal error. TEMPERATURE(LOAD) updates the material
// properties itself there.
class CaseTemperatureMaterialNonlinearCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"case.temperature-material-nonlinear",
                                    base::Severity::Error,
                                    "case_control",
                                    "A SOL 106 subcase has TEMPERATURE(MATERIAL) or TEMPERATURE(BOTH).",
                                    false,
                                    CheckScope::Deck};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    static constexpr std::string_view TEMPERATURE = "TemperatureSelection";

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const std::optional<std::int64_t> solution = context.solution();
        if (!solution || *solution != 106)
            return;
        const model::CaseControl& caseControl = context.root().control().caseControl;
        const model::CaseLineages lineages(caseControl);
        for (const model::Subcase* subcase : analysisSubcases(caseControl))
        {
            const std::uint32_t key = subcase->key;
            const bool material = lineages.command(key, TEMPERATURE, "MaterialTemperature") != nullptr;
            const bool both = lineages.command(key, TEMPERATURE, "BothLoadAndInitial") != nullptr;
            if (!material && !both)
                continue;
            sink.reportStatement(statementOf(*subcase), subcase->id,
                                 commandLabel(TEMPERATURE, both ? "BothLoadAndInitial" : "MaterialTemperature") +
                                     " is a fatal error in a nonlinear static analysis (SOL 106)",
                                 "use " + commandLabel(TEMPERATURE, "TemperatureLoadPart") +
                                     ", which also updates the material properties");
        }
    }
};

} // namespace

void addCaseTemperatureMaterialNonlinearCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<CaseTemperatureMaterialNonlinearCheck>());
}

} // namespace oor::analysis
