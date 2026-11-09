#include "oor/analysis/adjacency/ElementTopology.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/loads/LoadChecks.hpp"
#include "oor/model/cards/loads/facePressureLoad/FacePressureLoadTable.hpp"

namespace oor::analysis
{

namespace
{

// format reference, PLOAD4: G1 (with G3 or G4) names the face of a solid element the pressure is on, so a PLOAD4 on one
// solid element needs it. (A THRU range on solids is the form that loads the faces of a shell mesh and has no G1.)
class FacePressureLoadSolidFaceCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"load.face-pressure-load-solid-face-missing", base::Severity::Error, "loads",
                                    "A face pressure load on a solid element does not say which face (its first grid "
                                    "point is blank)."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        forEachCard<model::FacePressureLoadTable>(
            model,
            [&](const model::FacePressureLoadCard& card, model::CardLocation where)
            {
                if (card.isRange() || card.node1 != 0)
                    return;
                const std::optional<model::CardLocation> element = model.locate(base::IdSpace::Element, card.elemId);
                if (!element || dimensionOf(ElementTopology::shapeOf(element->table)) != 3)
                    return;
                const std::string node1 = std::string("node1");
                sink.reportCard(where, "node1",
                                "element " + std::to_string(card.elemId) + " is a solid: " + node1 + " gives the face",
                                "give " + node1 + " and " + std::string("node3") + " (or " + std::string("node4") +
                                    ") of the loaded face");
            });
    }
};

} // namespace

void addFacePressureLoadSolidFaceCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<FacePressureLoadSolidFaceCheck>());
}

} // namespace oor::analysis
