#include "TypeRules.hpp"
#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"

namespace oor::analysis
{

namespace
{

// The elements whose property card is of a kind of its own: the cohesive interface elements (PCOHE), the
// acoustic infinite elements (PACINF), the two-dimensional bush (PBUSH2D), the crack tip and seam elements (PRAC2D,
// PRAC3D, PSEAM), the axisymmetric line element and the SOL 700 spring (PAXISYM, PSPRMAT), and the SOL 700 scalar
// springs and dampers. The format reference of each element names the property card it takes; PELAS1 may only be
// named by a CELAS1D, which the check of the ordinary scalar elements already
// refuses.
class SpecialPropertyTypeCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "compat.special-property-type", base::Severity::Error, "compatibility",
        "A cohesive, infinite acoustic, two-dimensional bush, crack tip, seam, axisymmetric "
        "line, SOL 700 spring or SOL 700 damper element uses a property of another kind."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        reportWrongPropertyType(context, sink,
                                {"CohesiveHexa", "CohesivePenta", "CohesiveAxisymQuad", "CohesiveQuad"},
                                {"CohesiveProp"});
        reportWrongPropertyType(context, sink, {"AcousticInfiniteTria3", "AcousticInfiniteQuad4"},
                                {"AcousticInfiniteProp"});
        reportWrongPropertyType(context, sink, {"Bushing2D"}, {"BushProp2D"});
        reportWrongPropertyType(context, sink, {"CrackTip2D"}, {"CrackTipProp2D"});
        reportWrongPropertyType(context, sink, {"CrackTip3D"}, {"CrackTipProp3D"});
        reportWrongPropertyType(context, sink, {"Seam"}, {"SeamProp"});
        reportWrongPropertyType(context, sink, {"ShellAxisym"}, {"AxisymShellProp"});
        reportWrongPropertyType(context, sink, {"SpringOffsetExplicit"}, {"SpringOffsetProp"});
        reportWrongPropertyType(context, sink, {"SpringDofPropertyExplicit"}, {"SpringProp", "SpringPropTable"});
        // The format reference names "a PDAMPn" for the SOL 700 damper; PVISC1 is the nonlinear damper property of SOL
        // 700.
        reportWrongPropertyType(context, sink, {"DamperDofPropertyExplicit"},
                                {"DamperProp", "HeatCapacityProp5", "ViscousDamperPropNonlinear"},
                                {"DamperProp", "ViscousDamperPropNonlinear"});
    }
};

} // namespace

void addSpecialPropertyTypeCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<SpecialPropertyTypeCheck>());
}

} // namespace oor::analysis
