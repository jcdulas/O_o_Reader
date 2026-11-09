#include "oor/analysis/coordinates/CoordinateTransform.hpp"
#include "oor/analysis/validation/aero/AeroChecks.hpp"
#include "oor/analysis/validation/engine/ValidationContext.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/aero/aeroStaticParams/AeroStaticParamsTable.hpp"
#include "oor/model/cards/aero/aeroUnsteadyParams/AeroUnsteadyParamsTable.hpp"

#include <string>

namespace oor::analysis
{

namespace
{

// The aerodynamic coordinate system ACSID of an AERO or AEROS card, whose x axis is the direction of the flow, is a
// rectangular system, and so is the RCSID of an AEROS, in which the rigid body
// motions of the trim variables are given. A system that does not exist is `ref.unresolved`; the basic system (0) is
// rectangular.
class SystemRectangularCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "aero.system-rectangular", base::Severity::Error, "aero",
        "The aerodynamic coordinate system of an aerodynamic parameters card, or the reference coordinate system of "
        "the static one, is not a rectangular system."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        const SystemDefinitions systems = readSystemDefinitions(model);
        forEachCard<model::AeroUnsteadyParamsTable>(
            model, [&](const model::AeroUnsteadyParamsCard& card, model::CardLocation where)
            { look(model, systems, sink, where, "aeroFrameId", card.aeroFrameId); });
        forEachCard<model::AeroStaticParamsTable>(
            model,
            [&](const model::AeroStaticParamsCard& card, model::CardLocation where)
            {
                look(model, systems, sink, where, "aeroFrameId", card.aeroFrameId);
                look(model, systems, sink, where, "referenceFrameId", card.referenceFrameId);
            });
    }

private:
    static void look(const model::Model& model, const SystemDefinitions& systems, FindingSink& sink,
                     model::CardLocation where, const char* field, base::EntityId id)
    {
        const auto found = systems.find(id);
        if (found == systems.end() || found->second.kind == CoordinateKind::Rectangular)
            return;
        sink.reportCard(where, field,
                        "the coordinate system " + std::to_string(id) + " is a " + std::string(found->second.cardName) +
                            ", not a rectangular system",
                        "name a " + cardLabels(model, {"RectFrame1", "RectFrame2"}) + " system");
    }
};

} // namespace

void addSystemRectangularCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<SystemRectangularCheck>());
}

} // namespace oor::analysis
