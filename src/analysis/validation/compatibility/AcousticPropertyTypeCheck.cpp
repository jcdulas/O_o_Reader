#include "TypeRules.hpp"
#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"

namespace oor::analysis
{

namespace
{

// The acoustic elements and their properties (format reference): a CAABSF uses a PAABSF, a CHACAB a PACABS, a CHACBR a
// PACBAR.
class AcousticPropertyTypeCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "compat.acoustic-property-type", base::Severity::Error, "compatibility",
        "An acoustic absorber or barrier element uses a property that is not an acoustic absorber or barrier "
        "property."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        reportWrongPropertyType(context, sink, {"AcousticAbsorber"}, {"AcousticAbsorberProp"});
        reportWrongPropertyType(context, sink, {"AcousticAbsorberHexa"}, {"AcousticAbsorberBrickProp"});
        reportWrongPropertyType(context, sink, {"AcousticBarrierHexa"}, {"AcousticBarrierProp"});
    }
};

} // namespace

void addAcousticPropertyTypeCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<AcousticPropertyTypeCheck>());
}

} // namespace oor::analysis
