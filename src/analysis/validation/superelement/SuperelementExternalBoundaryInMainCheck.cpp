#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/superelement/SuperelementChecks.hpp"
#include "oor/model/cards/superelements/superelementExternalBoundary/SuperelementExternalBoundaryTable.hpp"

namespace oor::analysis
{

namespace
{

// A SuperelementExternalBoundary in the main Bulk Data. The card lists the connection points of an external
// superelement and is for the partitioned Bulk Data section of that superelement; in the main Bulk Data it is ignored.
class SuperelementExternalBoundaryInMainCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"superelement.superelement-external-boundary-in-main", base::Severity::Warning,
                                    "superelement",
                                    "A superelement external boundary card is in the main Bulk Data, "
                                    "where it is ignored."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        if (context.model().partInfo())
            return;
        forEachCard<model::SuperelementExternalBoundaryTable>(
            context.model(),
            [&](const model::SuperelementExternalBoundaryCard&, model::CardLocation where)
            {
                sink.reportCard(
                    where, "",
                    std::string("SuperelementExternalBoundary") + " is in the main Bulk Data, which ignores it",
                    "put the card in the BEGIN SUPER section of the external "
                    "superelement");
            });
    }
};

} // namespace

void addSuperelementExternalBoundaryInMainCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<SuperelementExternalBoundaryInMainCheck>());
}

} // namespace oor::analysis
