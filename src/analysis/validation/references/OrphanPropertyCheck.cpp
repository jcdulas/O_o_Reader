#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/references/ReferenceChecks.hpp"

namespace oor::analysis
{

namespace
{

// A property that no element (and no other card) uses. Opt-in: a library of properties is a normal thing to keep.
class OrphanPropertyCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"ref.orphan-property", base::Severity::Info, "references",
                                    "A property card that no element refers to.", true};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const ReferenceCensus& census = context.references();
        forEachCardOfCategory(
            context.model(), model::CardCategory::Property, base::IdSpace::Property,
            [&](model::CardLocation location)
            {
                const base::EntityId pid = context.model().findTable(location.table)->idAt(location.row);
                if (!census.properties.contains(pid))
                    sink.reportCard(location, idFieldAt(context.model(), location), "no element uses this property",
                                    "delete the property if it is not needed");
            });
    }
};

} // namespace

void addOrphanPropertyCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<OrphanPropertyCheck>());
}

} // namespace oor::analysis
