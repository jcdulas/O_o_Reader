#include "TypeRules.hpp"
#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"

namespace oor::analysis
{

namespace
{

// A CBAR whose property is not a PBAR or a PBARL (a CBAR with a PSHELL).
class BarPropertyTypeCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"compat.bar-property-type", base::Severity::Error, "compatibility",
                                    "A bar element uses a property that is not a bar, bar shape or bar arbitrary "
                                    "section property."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        // format reference CBAR: PID names a PBAR, PBARL or PBRSECT.
        reportWrongPropertyType(context, sink, {"Bar2"}, {"BarProp", "BarShapeProp", "BarArbitrarySectionProp"});
    }
};

} // namespace

void addBarPropertyTypeCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<BarPropertyTypeCheck>());
}

} // namespace oor::analysis
