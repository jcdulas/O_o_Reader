#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/explicit/ExplicitChecks.hpp"
#include "oor/model/cards/explicit/eulerMeshGenerator/EulerMeshGeneratorTable.hpp"
#include "oor/model/cards/explicit/eulerProp/EulerPropTable.hpp"
#include "oor/model/cards/explicit/eulerRegionProp/EulerRegionPropTable.hpp"

#include <string>

namespace oor::analysis
{

namespace
{

// A MESH whose PID names a property that is not an Eulerian one. The PID field says "a PEULER or PEULER1" and the
// property space is shared by every property card, so a PSHELL with that number resolves as a reference: only the kind
// of card tells the mistake.
class EulerMeshPropertyKindCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"explicit.euler-mesh-property-kind", base::Severity::Error, "explicit",
                                    "The property of an Euler mesh is not an Euler or Euler region property."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        forEachCard<model::EulerMeshGeneratorTable>(
            model,
            [&](const model::EulerMeshGeneratorCard& card, model::CardLocation where)
            {
                if (card.propId == 0)
                    return;
                const std::optional<model::CardLocation> property = model.locate(base::IdSpace::Property, card.propId);
                // A PID that names no card is for ref.unresolved.
                if (!property || isType<model::EulerPropTable>(*property) ||
                    isType<model::EulerRegionPropTable>(*property))
                    return;
                sink.reportCard(where, "propId",
                                "the " + std::string("propId") + " " + std::to_string(card.propId) + " is a " +
                                    std::string(cardNameAt(model, *property)) + ", not a " +
                                    cardLabels(model, {"EulerProp", "EulerRegionProp"}),
                                "name a " + cardLabels(model, {"EulerProp", "EulerRegionProp"}) +
                                    ", which gives the Eulerian elements their material");
            });
    }
};

} // namespace

void addEulerMeshPropertyKindCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<EulerMeshPropertyKindCheck>());
}

} // namespace oor::analysis
