#include "CaseRules.hpp"
#include "oor/analysis/validation/case_control/CaseControlChecks.hpp"
#include "oor/model/control/case_control/CaseLineages.hpp"

namespace oor::analysis
{

namespace
{

// A subcase with both TEMPERATURE(MATERIAL) (or TEMPERATURE(BOTH), which includes it) and TEMPERATURE(INITIAL), its own
// or inherited: the two requests exclude each other within one run.
class CaseTemperatureMaterialInitialCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"case.temperature-material-initial",
                                    base::Severity::Error,
                                    "case_control",
                                    "A subcase has both TEMPERATURE(MATERIAL) and TEMPERATURE(INITIAL).",
                                    false,
                                    CheckScope::Deck};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    static constexpr std::string_view TEMPERATURE = "TemperatureSelection";

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::CaseControl& caseControl = context.root().control().caseControl;
        const model::CaseLineages lineages(caseControl);
        for (const model::Subcase* subcase : analysisSubcases(caseControl))
        {
            const std::uint32_t key = subcase->key;
            const bool material = lineages.command(key, TEMPERATURE, "MaterialTemperature") != nullptr ||
                                  lineages.command(key, TEMPERATURE, "BothLoadAndInitial") != nullptr;
            if (!material || lineages.command(key, TEMPERATURE, "InitialTemperature") == nullptr)
                continue;
            const std::string materialLabel = commandLabel(TEMPERATURE, "MaterialTemperature");
            const std::string initialLabel = commandLabel(TEMPERATURE, "InitialTemperature");
            sink.reportStatement(
                statementOf(*subcase), subcase->id,
                materialLabel + " and " + initialLabel + " are both selected: the solver allows one of them in a run",
                "keep " + initialLabel + " for the initial temperature, or " + materialLabel +
                    " for the material properties");
        }
    }
};

} // namespace

void addCaseTemperatureMaterialInitialCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<CaseTemperatureMaterialInitialCheck>());
}

} // namespace oor::analysis
