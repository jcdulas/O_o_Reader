#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/fields/FieldChecks.hpp"
#include "oor/model/cards/properties/shellProp/ShellPropTable.hpp"

namespace oor::analysis
{

namespace
{

// format reference, PSHELL MID4: it must be blank unless MID1 and MID2 are both given, and it may not equal either of
// them.
class ShellPropCouplingCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"field.shell-prop-coupling-mat-invalid", base::Severity::Error, "fields",
                                    "A shell property gives a membrane-bending coupling material without both the "
                                    "membrane and the bending materials, or equal to one of them."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        const std::string membrane = std::string("membraneMatId");
        const std::string bending = std::string("bendingMatId");
        const std::string coupling = std::string("couplingMatId");
        forEachCard<model::ShellPropTable>(
            model,
            [&](const model::ShellPropCard& card, model::CardLocation where)
            {
                if (card.couplingMatId <= 0)
                    return;
                if (card.membraneMatId <= 0 || card.bendingMatId <= 0)
                    sink.reportCard(where, "couplingMatId", coupling + " needs both " + membrane + " and " + bending,
                                    "leave " + coupling + " blank");
                else if (card.couplingMatId == card.membraneMatId || card.couplingMatId == card.bendingMatId)
                    sink.reportCard(where, "couplingMatId",
                                    coupling + " is the same material as " + membrane + " or " + bending,
                                    "use another material for the coupling, or leave " + coupling + " blank");
            });
    }
};

} // namespace

void addShellPropCouplingCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<ShellPropCouplingCheck>());
}

} // namespace oor::analysis
