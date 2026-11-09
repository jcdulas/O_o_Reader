#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/fields/FieldChecks.hpp"
#include "oor/model/cards/properties/shellProp/ShellPropTable.hpp"

namespace oor::analysis
{

namespace
{

// format reference, PSHELL MID3: if MID2 is blank or -1, then MID3 must be blank.
class ShellPropShearWithoutBendingCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"field.shell-prop-shear-mat-without-bending-mat", base::Severity::Error, "fields",
                                    "A shell property gives a transverse shear material while the bending material is "
                                    "blank or -1."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        const std::string bending = std::string("bendingMatId");
        const std::string shear = std::string("shearMatId");
        forEachCard<model::ShellPropTable>(model,
                                           [&](const model::ShellPropCard& card, model::CardLocation where)
                                           {
                                               if (card.shearMatId > 0 && card.bendingMatId <= 0)
                                                   sink.reportCard(where, "shearMatId",
                                                                   shear + " is " + std::to_string(card.shearMatId) +
                                                                       " but " + bending + " is blank or -1",
                                                                   "give " + bending + " or leave " + shear + " blank");
                                           });
    }
};

} // namespace

void addShellPropShearWithoutBendingCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<ShellPropShearWithoutBendingCheck>());
}

} // namespace oor::analysis
