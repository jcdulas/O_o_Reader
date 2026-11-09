#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/fields/FieldChecks.hpp"
#include "oor/model/cards/properties/shellProp/ShellPropTable.hpp"

namespace oor::analysis
{

namespace
{

// A PSHELL with neither a membrane material (MID1) nor a bending one (MID2): the shell has no stiffness at all.
class ShellNoMaterialCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"field.shell-no-material", base::Severity::Warning, "fields",
                                    "A shell property has neither a membrane nor a bending material: the shell has no "
                                    "stiffness."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        const std::string membrane = std::string("membraneMatId");
        const std::string bending = std::string("bendingMatId");
        forEachCard<model::ShellPropTable>(
            model,
            [&](const model::ShellPropCard& card, model::CardLocation where)
            {
                if (card.membraneMatId <= 0 && card.bendingMatId <= 0)
                    sink.reportCard(where, "membraneMatId",
                                    membrane + " and " + bending + " are blank: no membrane and no bending stiffness",
                                    "give " + membrane + " (membrane) and " + bending + " (bending)");
            });
    }
};

} // namespace

void addShellNoMaterialCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<ShellNoMaterialCheck>());
}

} // namespace oor::analysis
