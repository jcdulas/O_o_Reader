#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/references/ReferenceChecks.hpp"

namespace oor::analysis
{

namespace
{

// A coordinate system that no grid point, element, load or other coordinate system uses.
class OrphanCoordinateCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"ref.orphan-coordinate", base::Severity::Info, "references",
                                    "A coordinate system card that nothing refers to.", true};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const ReferenceCensus& census = context.references();
        forEachCardOfCategory(context.model(), model::CardCategory::Coordinate, base::IdSpace::Coordinate,
                              [&](model::CardLocation location)
                              {
                                  const base::EntityId cid =
                                      context.model().findTable(location.table)->idAt(location.row);
                                  if (!census.coordinates.contains(cid))
                                      sink.reportCard(location, idFieldAt(context.model(), location),
                                                      "nothing uses this coordinate system",
                                                      "delete the coordinate system if it is not needed");
                              });
    }
};

} // namespace

void addOrphanCoordinateCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<OrphanCoordinateCheck>());
}

} // namespace oor::analysis
