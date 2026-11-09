#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/solution_control/wettedElemSide/WettedElemSideTable.hpp"

#include <optional>
#include <string>

namespace oor::analysis
{

namespace
{

// A WETELME gives a side of a structural element as a wetted element. The format reference (WETELME, EID) allows the
// shells CQUAD4, CQUAD8, CQUADR, CTRIA3, CTRIA6 and CTRIAR and the solids CTETRA, CPENTA and CHEXA; the card cannot
// tell the kind of the element it names, so the rule is a check of the model.
class WettedElementTypeCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"compat.wetted-element-type", base::Severity::Warning, "compatibility",
                                    "A wetted element side card names an element that is not a shell, a tetrahedron, a "
                                    "pentahedron or a hexahedron."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        forEachCard<model::WettedElemSideTable>(
            model,
            [&](const model::WettedElemSideCard& card, model::CardLocation where)
            {
                const std::optional<model::CardLocation> element = model.locate(base::IdSpace::Element, card.elemId);
                if (!element)
                    return; // an element that does not exist is the business of `ref.unresolved`
                const std::string_view name = cardNameAt(model, *element);
                if (isOneOf(name, {"ShellQuad4", "ShellQuad8", "ShellQuad4Drilling", "ShellTria3", "ShellTria6",
                                   "ShellTria3Drilling", "SolidTetra", "SolidPenta", "SolidHexa"}))
                    return;
                sink.reportCard(where, "elemId",
                                "names element " + std::to_string(card.elemId) + ", which is a " + std::string(name) +
                                    ": a wetted element is a side of a shell, a " +
                                    cardLabels(model, {"SolidTetra", "SolidPenta", "SolidHexa"}),
                                "name a shell or a solid element");
            });
    }
};

} // namespace

void addWettedElementTypeCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<WettedElementTypeCheck>());
}

} // namespace oor::analysis
