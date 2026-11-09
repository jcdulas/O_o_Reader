#include "AeroCheckSupport.hpp"
#include "oor/analysis/validation/aero/AeroChecks.hpp"
#include "oor/analysis/validation/engine/ValidationContext.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/aero/aeroPistonPanel5/AeroPistonPanel5Table.hpp"
#include "oor/model/cards/aero/aeroStripPanel4/AeroStripPanel4Table.hpp"

#include <string>

namespace oor::analysis
{

namespace
{

// The strips of a CAERO4 and a CAERO5 are numbered from the EID on, and their numbers must not be those of grid points,
// scalar points or extra points. A warning: only some pre-processors is
// hurt by it (the strips of the model share their numbers with points in the model database it builds).
class StripIdsCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "aero.strip-ids", base::Severity::Warning, "aero",
        "The numbers of the strips of a strip theory or piston theory panel are those of grid, scalar or extra "
        "points."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        forEachCard<model::AeroStripPanel4Table>(
            model, [&](const model::AeroStripPanel4Card& card, model::CardLocation where)
            { look(model, sink, where, card.id, card.spanBoxCount, card.spanDivisionListId); });
        forEachCard<model::AeroPistonPanel5Table>(
            model, [&](const model::AeroPistonPanel5Card& card, model::CardLocation where)
            { look(model, sink, where, card.id, card.spanBoxCount, card.spanDivisionListId); });
    }

private:
    static void look(const model::Model& model, FindingSink& sink, model::CardLocation where, base::EntityId eid,
                     std::int32_t count, base::EntityId list)
    {
        const std::optional<std::int64_t> strips = divisionElements(model, count, list);
        for (std::int64_t offset = 0; strips && offset < *strips; ++offset)
        {
            const std::int64_t id = static_cast<std::int64_t>(eid) + offset;
            if (id > base::MAX_ENTITY_ID || !model.locate(base::IdSpace::Node, static_cast<base::EntityId>(id)))
                continue;
            sink.reportCard(where, "id", "the strip " + std::to_string(id) + " has the number of a point of the model",
                            "number the strips from an ID that no grid, scalar or extra point has");
            return;
        }
    }
};

} // namespace

void addStripIdsCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<StripIdsCheck>());
}

} // namespace oor::analysis
