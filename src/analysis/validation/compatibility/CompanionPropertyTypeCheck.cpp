#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"

#include <string>

namespace oor::analysis
{

namespace
{

struct CompanionRule
{
    std::string_view companion; // the card that extends a property
    std::string_view first;     // the property cards it may extend (the second may be empty)
    std::string_view second;
};

// The property companions name the property they extend by its PID (the format reference says which cards it must be):
// the nonlinear extensions of a PBAR or PBARL, a PBEAM or PBEAML, a PROD, a PSHEAR and a PSOLID, the layered composite
// data of a PCOMP or PCOMPG (PCOMPA of SOL 700, PCOMPF and PCOMPFQ).
const CompanionRule RULES[] = {
    {"BarPropNonlinear", "BarProp", "BarShapeProp"},
    {"BeamPropNonlinear", "BeamProp", "BeamShapeProp"},
    {"RodPropNonlinear", "RodProp", ""},
    {"ShearPanelPropNonlinear", "ShearPanelProp", ""},
    {"SolidPropNonlinear", "SolidProp", ""},
    {"CompPropExplicit", "CompProp", "CompPropGlobal"},
    {"CompPropIntegration", "CompProp", "CompPropGlobal"},
    {"CompPropFreqDamping", "CompProp", "CompPropGlobal"},
};

// A property extension that names a property of another kind.
class CompanionPropertyTypeCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "compat.companion-property-type", base::Severity::Error, "compatibility",
        "A nonlinear or composite extension of a property names a property of the wrong kind."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        for (const CardReference& use : context.references().propertyExtensions)
        {
            const std::string_view companion = cardNameAt(model, use.location);
            for (const CompanionRule& rule : RULES)
            {
                if (companion != rule.companion)
                    continue;
                const std::optional<model::CardLocation> property = model.locate(base::IdSpace::Property, use.id);
                if (!property)
                    break; // a property that does not exist is the business of ref.unresolved
                const std::string_view name = cardNameAt(model, *property);
                if (isOneOf(name, {rule.first, rule.second}))
                    break;
                // A message names the card types by their deck keyword.
                const std::string needs =
                    rule.second.empty() ? std::string(rule.first) : cardLabels(model, {rule.first, rule.second});
                sink.reportCard(use.location, use.field,
                                "extends property " + std::to_string(use.id) + ", which is a " + std::string(name) +
                                    ": " + std::string(companion) + " needs a " + needs,
                                "point the card at a property of the right type");
                break;
            }
        }
    }
};

} // namespace

void addCompanionPropertyTypeCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<CompanionPropertyTypeCheck>());
}

} // namespace oor::analysis
