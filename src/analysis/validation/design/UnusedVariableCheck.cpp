#include "oor/analysis/validation/design/DesignChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/optimisation/designVar/DesignVarTable.hpp"
#include "oor/model/design/DesignModel.hpp"

namespace oor::analysis
{

namespace
{

// A design variable that no relation, DLINK, DVGRID, DVSHAP, DVBSHAP, DRESP2 or DRESP3 uses changes nothing in the
// model and costs the optimiser a column of the sensitivity matrix. A deck sometimes keeps such variables on purpose,
// so this is information, and a variable that a DLINK makes dependent on others and that nothing uses counts as unused.
class UnusedVariableCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"design.unused-variable", base::Severity::Info, "design",
                                    "A design variable that no relation, link or response uses."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::DesignModel design = model::DesignModel::read(context.model());
        for (const model::DesignVariable& variable : design.variables())
        {
            if (!design.isUsed(variable.id))
                sink.reportCard(model::CardLocation{model::typeIndexOf<model::DesignVarTable>(), variable.row}, "id",
                                "no relation, " + std::string("DesignVarLink") + ", " + std::string("DesignNodeMove") +
                                    ", " + std::string("DesignShapeBasis") + ", " + std::string("DesignLoadRelation1") +
                                    ", " + std::string("DesignControlSurface") + ", " + std::string("DesignResp2") +
                                    " or " + std::string("DesignResp3") + " uses the design variable " +
                                    std::to_string(variable.id) + " (" + variable.label + ")",
                                "remove the " + std::string("DesignVar") + ", or add the relation that uses it");
        }
    }
};

} // namespace

void addUnusedVariableCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<UnusedVariableCheck>());
}

} // namespace oor::analysis
