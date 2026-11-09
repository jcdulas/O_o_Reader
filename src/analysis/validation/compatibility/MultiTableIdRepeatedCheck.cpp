#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/engine/SharedIds.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"

namespace oor::analysis
{

namespace
{

// A table number that two kinds of multi-dimensional table have: TABL3D0, TABL3D1, TABL3D2 and TABLE3D all stand where
// a material, a contact entry or a CORD3G names a table of several variables by its number, and a field that may name a
// TABLEMi or a TABL3Di cannot tell which was meant when both have the number. Two cards of one type are the reader's
// business (`ref.duplicate-id`).
class MultiTableIdRepeatedCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "compat.multi-table-id-repeated", base::Severity::Error, "compatibility",
        "A multi-variable or scattered 3D table has the number of a table of another kind."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        reportIdsSharedByTypes(context, sink, model::CardCategory::MultiDimensionalTable, "multi-dimensional table");
    }
};

} // namespace

void addMultiTableIdRepeatedCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<MultiTableIdRepeatedCheck>());
}

} // namespace oor::analysis
