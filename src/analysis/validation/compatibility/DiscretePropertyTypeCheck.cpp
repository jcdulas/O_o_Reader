#include "TypeRules.hpp"
#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"

namespace oor::analysis
{

namespace
{

// A bush, spring, damper, viscous damper, shear panel, mass, gap or fastener whose property is of another kind.
class DiscretePropertyTypeCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"compat.discrete-property-type", base::Severity::Error, "compatibility",
                                    "A bush, spring, damper, viscous damper, shear panel, scalar mass, gap or fastener "
                                    "element uses a property of another kind."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        reportWrongPropertyType(context, sink, {"Bushing"}, {"BushProp"});
        reportWrongPropertyType(context, sink, {"SpringDofProperty", "SpringScalarProperty"}, {"SpringProp"});
        reportWrongPropertyType(context, sink, {"DamperDofProperty", "DamperScalarProperty"}, {"DamperProp"});
        // format reference PVISC1: the nonlinear damper of SOL 700, the counterpart of the linear PVISC of a CVISC.
        reportWrongPropertyType(context, sink, {"ViscousDamper2"},
                                {"ViscousDamperProp", "ViscousDamperPropNonlinear"});
        reportWrongPropertyType(context, sink, {"ShearPanel4"}, {"ShearPanelProp"});
        reportWrongPropertyType(context, sink, {"MassDofProperty", "MassScalarProperty"}, {"MassProp"});
        reportWrongPropertyType(context, sink, {"Gap2"}, {"GapProp"});
        reportWrongPropertyType(context, sink, {"Fastener"}, {"FastenerProp"});
    }
};

} // namespace

void addDiscretePropertyTypeCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<DiscretePropertyTypeCheck>());
}

} // namespace oor::analysis
