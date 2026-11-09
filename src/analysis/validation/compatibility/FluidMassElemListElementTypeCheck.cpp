#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"

#include <string>

namespace oor::analysis
{

namespace
{

// An ELIST is the list of the quadrilateral and triangular shells that carry virtual fluid mass (format reference
// ELIST: the list holds CQUAD4 and CTRIA3 elements only); an element of another type in it has no wetted side to
// give.
class FluidMassElemListElementTypeCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"compat.fluid-mass-elem-list-element-type", base::Severity::Warning,
                                    "compatibility",
                                    "A fluid mass element list names an element that is not a "
                                    "four-node quadrilateral or a three-node triangle."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        for (const CardReference& use : context.references().elementListMembers)
        {
            const std::optional<model::CardLocation> element = model.locate(base::IdSpace::Element, use.id);
            if (!element)
                continue; // an element that does not exist (or is the end of a THRU range with gaps): not this check's
                          // business
            const std::string_view name = cardNameAt(model, *element);
            if (isOneOf(name, {"ShellQuad4", "ShellTria3"}))
                continue;
            sink.reportCard(use.location, use.field,
                            "lists element " + std::to_string(use.id) + ", which is a " + std::string(name) + ": a " +
                                std::string("FluidMassElemList") + " holds " +
                                cardLabels(model, {"ShellQuad4", "ShellTria3"}, "and") + " elements",
                            "list only " + cardLabels(model, {"ShellQuad4", "ShellTria3"}, "and") + " elements");
        }
    }
};

} // namespace

void addFluidMassElemListElementTypeCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<FluidMassElemListElementTypeCheck>());
}

} // namespace oor::analysis
