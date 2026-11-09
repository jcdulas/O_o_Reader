#include "TypeRules.hpp"
#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"

namespace oor::analysis
{

namespace
{

// A shell element (CQUAD4, CTRIA3, ...) whose property is not a shell property (the format reference gives PSHELL,
// PCOMP, PCOMPG, PSHELL1 for SOL 700, PSHLPF for the perforated shell of a trim component and, for the plane elements
// of a nonlinear analysis, PLPLANE and PLCOMP).
class ShellPropertyTypeCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "compat.shell-property-type", base::Severity::Error, "compatibility",
        "A shell element uses a property that is not a shell, nodal shell, perforated shell, composite, nonlinear "
        "plane or plane composite property."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        reportWrongPropertyType(context, sink,
                                {"ShellQuad4", "ShellTria3", "ShellQuad8", "ShellTria6", "ShellQuad4Drilling",
                                 "ShellTria3Drilling", "PlaneStrainQuad"},
                                {"ShellProp", "ShellPropNodal", "PerforatedShellProp", "CompProp", "CompPropGlobal",
                                 "PlanePropNonlinear", "PlaneCompProp"});
    }
};

} // namespace

void addShellPropertyTypeCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<ShellPropertyTypeCheck>());
}

} // namespace oor::analysis
