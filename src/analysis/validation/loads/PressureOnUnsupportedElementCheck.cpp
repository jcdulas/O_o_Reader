#include "ElementsInRange.hpp"
#include "oor/analysis/adjacency/ElementTopology.hpp"
#include "oor/analysis/validation/loads/LoadChecks.hpp"
#include "oor/model/cards/loads/facePressureLoad/FacePressureLoadTable.hpp"

namespace oor::analysis
{

namespace
{

// A PLOAD4 pressure on an element that has no surface to press on: a rod, bar, bush, spring, mass or rigid element. The
// element types the library does not know the shape of are not judged.
class PressureOnUnsupportedElementCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"load.pressure-on-unsupported-element", base::Severity::Error, "loads",
                                    "A face pressure load puts a surface pressure on an element that is not a shell or "
                                    "a solid."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        forEachCard<model::FacePressureLoadTable>(
            model,
            [&](const model::FacePressureLoadCard& card, model::CardLocation where)
            {
                if (card.kind == model::PressureKind::Line)
                    return; // a line load acts along an edge: the bars take it
                std::string refused;
                forEachElementInRange(model, card.elemId, card.isRange() ? card.elemId2 : card.elemId,
                                      [&](model::CardLocation element)
                                      {
                                          const int dimension = dimensionOf(ElementTopology::shapeOf(element.table));
                                          if (dimension == 0 || dimension == 1)
                                          {
                                              if (refused.empty())
                                                  refused =
                                                      std::string(cardNameAt(model, element)) + " " +
                                                      std::to_string(model.findTable(element.table)->idAt(element.row));
                                          }
                                      });
                if (!refused.empty())
                    sink.reportCard(where, "elemId", "the pressure is on " + refused + ", which has no surface",
                                    "apply the pressure to a shell or solid element");
            });
    }
};

} // namespace

void addPressureOnUnsupportedElementCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<PressureOnUnsupportedElementCheck>());
}

} // namespace oor::analysis
