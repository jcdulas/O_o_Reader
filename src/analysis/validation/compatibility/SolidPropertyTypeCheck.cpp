#include "TypeRules.hpp"
#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"

namespace oor::analysis
{

namespace
{

// A solid element whose property is not a solid property.
class SolidPropertyTypeCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "compat.solid-property-type", base::Severity::Error, "compatibility",
        "A solid element uses a property that is not a solid or hyperelastic solid property (a hexahedron may also use "
        "a composite solid or nonlinear composite solid property, a pentahedron a composite solid property, and each "
        "of them an Euler or Euler region property in SOL 700)."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        // format reference PEULER and PEULER1 (SOL 700): "the elements that reference this property use the Eulerian
        // formulation". The formats do not list the element types; the library takes the solid elements of an Euler
        // mesh, CHEXA (the elements a MESH generates), CPENTA and CTETRA.
        reportWrongPropertyType(context, sink, {"SolidTetra"},
                                {"SolidProp", "SolidPropHyperelastic", "EulerProp", "EulerRegionProp"});
        reportWrongPropertyType(context, sink, {"SolidPyramid"}, {"SolidProp", "SolidPropHyperelastic"});
        // The layered solid composite property can only be named by a CHEXA. The
        // PCOMPS of another implementation of the format (not in the format reference) is named by a CHEXA or a CPENTA.
        reportWrongPropertyType(
            context, sink, {"SolidPenta"},
            {"SolidProp", "SolidPropHyperelastic", "CompSolidProp", "EulerProp", "EulerRegionProp"});
        reportWrongPropertyType(context, sink, {"SolidHexa"},
                                {"SolidProp", "SolidPropHyperelastic", "CompSolidPropNonlinear", "CompSolidProp",
                                 "EulerProp", "EulerRegionProp"});
    }
};

} // namespace

void addSolidPropertyTypeCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<SolidPropertyTypeCheck>());
}

} // namespace oor::analysis
