#include "oor/analysis/adjacency/ElementTopology.hpp"
#include "oor/analysis/geometry/ElementGeometry.hpp"
#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/base/errors/Errors.hpp"
#include "oor/model/cards/properties/shellProp/ShellPropTable.hpp"

#include <cmath>
#include <unordered_map>

namespace oor::analysis
{

namespace
{

// A PSHELL with no thickness (T blank or zero) used by a shell element that gives none of its own: the element has no
// thickness at all.
class ShellThicknessCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"compat.shell-thickness", base::Severity::Error, "compatibility",
                                    "A shell property has a thickness of zero and an element that uses it gives none."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        // The properties without a thickness, by PID, with the location of the card.
        std::unordered_map<base::EntityId, model::CardLocation> thin;
        forEachCard<model::ShellPropTable>(
            model,
            [&](const model::ShellPropCard& card, model::CardLocation where)
            {
                if (card.thickness <= 0.0 && (card.membraneMatId > 0 || card.bendingMatId > 0))
                    thin.emplace(card.id, where);
            });
        if (thin.empty())
            return;
        // The first element of each such property whose effective thickness is zero or undefined.
        std::unordered_map<base::EntityId, base::EntityId> firstElement;
        for (const CardReference& use : context.references().elementProperties)
        {
            if (!thin.contains(use.id) || firstElement.contains(use.id))
                continue;
            if (dimensionOf(ElementTopology::shapeOf(use.location.table)) != 2)
                continue;
            try
            {
                const ElementValues values = ElementGeometry::of(model, use.location, bit(Quantity::Thickness));
                if (std::isfinite(values.thickness) && values.thickness > 0.0)
                    continue;
            }
            catch (const base::OorError&)
            {
                continue; // a node that does not exist: ref.unresolved says so
            }
            firstElement.emplace(use.id, model.findTable(use.location.table)->idAt(use.location.row));
        }
        for (const auto& [pid, element] : firstElement)
            sink.reportCard(thin.at(pid), "thickness",
                            std::string("thickness") + " is zero, so element " + std::to_string(element) +
                                " (and perhaps others) has no thickness",
                            "give " + std::string("thickness") + " on the " + std::string("ShellProp") +
                                ", or the thicknesses on the elements");
    }
};

} // namespace

void addShellThicknessCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<ShellThicknessCheck>());
}

} // namespace oor::analysis
