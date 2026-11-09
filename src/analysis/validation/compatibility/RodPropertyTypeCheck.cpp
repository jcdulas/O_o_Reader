#include "TypeRules.hpp"
#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"

namespace oor::analysis
{

namespace
{

// A CROD that does not use a PROD, a CTUBE that does not use a PTUBE.
class RodPropertyTypeCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"compat.rod-property-type", base::Severity::Error, "compatibility",
                                    "A rod element uses a property that is not a rod or belt property, or a tube "
                                    "element one that is not a tube property."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        // format reference PBELT: the belt property of SOL 700 is referenced by a CROD.
        reportWrongPropertyType(context, sink, {"Rod2"}, {"RodProp", "BeltProp"});
        reportWrongPropertyType(context, sink, {"Tube2"}, {"TubeProp"});
    }
};

} // namespace

void addRodPropertyTypeCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<RodPropertyTypeCheck>());
}

} // namespace oor::analysis
