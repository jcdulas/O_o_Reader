#include "oor/analysis/validation/contact/ContactChecks.hpp"
#include "oor/analysis/validation/engine/SharedIds.hpp"

namespace oor::analysis
{

namespace
{

// A contact table ID that a BCTABL1 and a BCTABLE both have: the BCONTACT command names a table by its ID and cannot
// tell the two cards apart.
class TableIdRepeatedCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "contact.table-id-repeated", base::Severity::Error, "contact",
        "Two contact tables of different kinds have the same ID, which the BCONTACT command cannot tell apart."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        reportIdsSharedByTypes(context, sink, model::CardCategory::ContactTable, "contact table");
    }
};

} // namespace

void addTableIdRepeatedCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<TableIdRepeatedCheck>());
}

} // namespace oor::analysis
