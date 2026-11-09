#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/superelement/SuperelementChecks.hpp"
#include "oor/model/cards/superelements/axisymConeSector/AxisymConeSectorTable.hpp"

#include <string>

namespace oor::analysis
{

namespace
{

// An AxisymConeSector (a sector of a conical shell) whose ring is not an AxisymConeRing, or whose number is also the
// number of an AxisymConeRing or an AxisymConePoint. The format reference asks the numbers of the
// three to be unique: the rings and the points share the node ID space, so the reader sees two of them with one
// number, and the sector is filed under its own card type, which is why the model checks it here. A ring that no card
// has is reported by `ref.unresolved`.
class AxisymConeSectorRingCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "superelement.axisym-cone-sector-ring", base::Severity::Error, "superelement",
        "A conical shell sector has the number of a conical shell ring or point, or names a ring that is not a conical "
        "shell ring."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        forEachCard<model::AxisymConeSectorTable>(
            model,
            [&](const model::AxisymConeSectorCard& card, model::CardLocation where)
            {
                if (const auto same = model.locate(base::IdSpace::Node, card.id))
                {
                    const std::string_view name = cardNameAt(model, *same);
                    if (name == "AxisymConeRing" || name == "AxisymConePoint")
                        sink.reportCard(
                            where, "id",
                            "the number " + std::to_string(card.id) + " is also the number of a " + std::string(name),
                            "give the sector a number that no " + std::string("AxisymConeRing") + " or " +
                                std::string("AxisymConePoint") + " has");
                }
                if (const auto ring = model.locate(base::IdSpace::Node, card.ringId))
                {
                    const std::string_view name = cardNameAt(model, *ring);
                    if (name != "AxisymConeRing")
                        sink.reportCard(where, "ringId",
                                        "the ring " + std::to_string(card.ringId) + " is a " + std::string(name) +
                                            ", not a " + std::string("AxisymConeRing"),
                                        "name the number of a " + std::string("AxisymConeRing") + " card");
                }
            });
    }
};

} // namespace

void addAxisymConeSectorRingCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<AxisymConeSectorRingCheck>());
}

} // namespace oor::analysis
