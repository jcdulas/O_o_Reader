#include "SolutionRules.hpp"
#include "oor/analysis/validation/solution/SolutionChecks.hpp"
#include "oor/model/cards/constraints/enforcedMotionConstr/EnforcedMotionConstrTable.hpp"
#include "oor/model/cards/constraints/geomSinglePointConstr/GeomSinglePointConstrTable.hpp"
#include "oor/model/cards/constraints/singlePointConstr/SinglePointConstrTable.hpp"
#include "oor/model/cards/constraints/singlePointConstrList/SinglePointConstrListTable.hpp"
#include "oor/model/cards/constraints/singlePointConstrUnion/SinglePointConstrUnionTable.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/model/cards/thermal/forcedConvection/ForcedConvectionTable.hpp"
#include "oor/model/cards/thermal/freeConvection/FreeConvectionTable.hpp"
#include "oor/model/cards/thermal/radiationToSpace/RadiationToSpaceTable.hpp"
#include "oor/model/cards/thermal/temperatureBoundary/TemperatureBoundaryTable.hpp"

namespace oor::analysis
{

namespace
{

template <typename TableT>
bool hasCards(const model::Model& model)
{
    const TableT* table = model.findTable<TableT>();
    return table != nullptr && table->rows().aliveCount() > 0;
}

// True when the model holds something that fixes a temperature or ties the model to an ambient one: an SPC, SPC1,
// SPCADD or SPCD (a temperature constraint on a grid point), a PS field on a grid point, a TEMPBC, a convection (CONV,
// CONVM) or a radiation to space (RADBC), a GMSPC, or a card the library keeps as text that is a boundary condition of
// the p-version elements (GMBC): the check cannot tell what it fixes.
bool hasTemperatureBoundary(const model::Model& model)
{
    if (hasCards<model::SinglePointConstrTable>(model) || hasCards<model::SinglePointConstrListTable>(model) ||
        hasCards<model::SinglePointConstrUnionTable>(model) || hasCards<model::EnforcedMotionConstrTable>(model) ||
        hasCards<model::GeomSinglePointConstrTable>(model) || hasCards<model::TemperatureBoundaryTable>(model) ||
        hasCards<model::FreeConvectionTable>(model) || hasCards<model::ForcedConvectionTable>(model) ||
        hasCards<model::RadiationToSpaceTable>(model))
        return true;
    bool pinned = false;
    forEachCard<model::NodeTable>(model, [&](const model::NodeCard& grid, model::CardLocation)
                                  { pinned = pinned || grid.permanentConstraints > 0; });
    if (pinned)
        return true;
    for (const std::string& name : unknownCardNames(model))
    {
        if (name == "GMBC")
            return true;
    }
    return false;
}

// A steady-state heat transfer analysis (APP HEAT with SOL 101 or SOL 153) that has nothing which holds a temperature:
// the conduction matrix is singular, because only the temperature differences are determined. A transient analysis (SOL
// 159) has the heat capacity to hold the temperatures and is not judged.
class HeatNoBoundaryCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "solution.heat-no-boundary",
        base::Severity::Warning,
        "solution",
        "A steady-state heat transfer analysis has no temperature constraint, convection or "
        "radiation to space.",
        false,
        CheckScope::Deck};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const std::optional<std::int64_t> solution = context.solution();
        if (!solution || (*solution != 101 && *solution != 153) || !isHeatTransfer(context.root()))
            return;
        for (const model::Model* model : context.allModels())
        {
            if (hasTemperatureBoundary(*model))
                return;
        }
        sink.reportStatement("SolutionSequence", *solution,
                             "a steady-state heat transfer analysis has no " + std::string("SinglePointConstr") + ", " +
                                 std::string("SinglePointConstrList") + ", " + std::string("TemperatureBoundary") +
                                 ", " + std::string("FreeConvection") + ", " + std::string("ForcedConvection") +
                                 " or " + std::string("RadiationToSpace") +
                                 ": the "
                                 "temperatures are not determined",
                             "constrain a temperature, or add a convection or a radiation boundary");
    }
};

} // namespace

void addHeatNoBoundaryCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<HeatNoBoundaryCheck>());
}

} // namespace oor::analysis
