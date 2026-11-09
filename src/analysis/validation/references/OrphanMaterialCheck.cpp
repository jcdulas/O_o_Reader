#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/references/ReferenceChecks.hpp"

namespace oor::analysis
{

namespace
{

// A material that no property (and no other card) uses.
class OrphanMaterialCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"ref.orphan-material", base::Severity::Info, "references",
                                    "A material card that no property refers to.", true};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const ReferenceCensus& census = context.references();
        forEachCardOfCategory(
            context.model(), model::CardCategory::Material, base::IdSpace::Material,
            [&](model::CardLocation location)
            {
                const base::EntityId mid = context.model().findTable(location.table)->idAt(location.row);
                if (!census.materials.contains(mid))
                    sink.reportCard(location, idFieldAt(context.model(), location), "no property uses this material",
                                    "delete the material if it is not needed");
            });
    }
};

} // namespace

void addOrphanMaterialCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<OrphanMaterialCheck>());
}

} // namespace oor::analysis
