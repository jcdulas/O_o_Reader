#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/loads/axisymConePressureLoad/AxisymConePressureLoadTable.hpp"
#include "oor/model/cards/loads/axisymRingMomentLoad/AxisymRingMomentLoadTable.hpp"
#include "oor/model/cards/nodes/acousticSlotSurfacePoint/AcousticSlotSurfacePointTable.hpp"
#include "oor/model/cards/nodes/axisymConePoint/AxisymConePointTable.hpp"
#include "oor/model/cards/nodes/axisymFluidFreePoint/AxisymFluidFreePointTable.hpp"
#include "oor/model/cards/nodes/axisymFluidPressurePoint/AxisymFluidPressurePointTable.hpp"

#include <string>
#include <string_view>

namespace oor::analysis
{

namespace
{

// A field of the axisymmetric cards that names a point of another kind than the one the format reference says: the ring
// of a POINTAX, MOMAX or PRESAX is a RINGAX, the fluid point of a FREEPT or a PRESPT a RINGFL and the interface point
// of a GRIDS a GRIDF. The points share the node ID space, so the reference resolves to whatever card has the number; a
// number that names no card is reported by `ref.unresolved`.
class AxisymmetricPointKindCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "compat.axisymmetric-point-kind", base::Severity::Error, "compatibility",
        "An axisymmetric point, load or fluid point card names a point that is not of the kind it needs (a conical "
        "shell ring, a fluid ring or a slot fluid point)."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        forEachCard<model::AxisymConePointTable>(
            model, [&](const model::AxisymConePointCard& card, model::CardLocation where)
            { expect(model, sink, where, "ringId", card.ringId, "AxisymConeRing"); });
        forEachCard<model::AxisymRingMomentLoadTable>(
            model, [&](const model::AxisymRingMomentLoadCard& card, model::CardLocation where)
            { expect(model, sink, where, "ringId", card.ringId, "AxisymConeRing"); });
        forEachCard<model::AxisymConePressureLoadTable>(
            model,
            [&](const model::AxisymConePressureLoadCard& card, model::CardLocation where)
            {
                expect(model, sink, where, "ringId1", card.ringId1, "AxisymConeRing");
                expect(model, sink, where, "ringId2", card.ringId2, "AxisymConeRing");
            });
        forEachCard<model::AxisymFluidFreePointTable>(
            model, [&](const model::AxisymFluidFreePointCard& card, model::CardLocation where)
            { expect(model, sink, where, "fluidPointId", card.fluidPointId, "AxisymFluidRing"); });
        forEachCard<model::AxisymFluidPressurePointTable>(
            model, [&](const model::AxisymFluidPressurePointCard& card, model::CardLocation where)
            { expect(model, sink, where, "fluidPointId", card.fluidPointId, "AxisymFluidRing"); });
        forEachCard<model::AcousticSlotSurfacePointTable>(
            model, [&](const model::AcousticSlotSurfacePointCard& card, model::CardLocation where)
            { expect(model, sink, where, "fluidPointId", card.fluidPointId, "AcousticSlotFluidPoint"); });
    }

private:
    static void expect(const model::Model& model, FindingSink& sink, model::CardLocation where, const char* field,
                       base::EntityId id, std::string_view kind)
    {
        if (id == 0)
            return;
        const std::optional<model::CardLocation> found = model.locate(base::IdSpace::Node, id);
        if (!found)
            return; // ref.unresolved reports it
        const std::string_view actual = cardNameAt(model, *found);
        if (actual == kind)
            return;
        const std::string needed = std::string(kind);
        sink.reportCard(where, field,
                        std::string(cardNameAt(model, where)) + " names the point " + std::to_string(id) +
                            ", which is a " + std::string(actual) + ", and the card needs a " + needed,
                        "name the number of a " + needed + " card");
    }
};

} // namespace

void addAxisymmetricPointKindCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<AxisymmetricPointKindCheck>());
}

} // namespace oor::analysis
