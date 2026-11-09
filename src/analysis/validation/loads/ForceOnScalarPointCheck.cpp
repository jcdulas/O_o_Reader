#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/loads/LoadChecks.hpp"
#include "oor/model/cards/loads/pointForceLoad/PointForceLoadTable.hpp"
#include "oor/model/cards/loads/pointForceLoad1/PointForceLoad1Table.hpp"
#include "oor/model/cards/loads/pointForceLoad2/PointForceLoad2Table.hpp"
#include "oor/model/cards/loads/pointMomentLoad/PointMomentLoadTable.hpp"
#include "oor/model/cards/loads/pointMomentLoad1/PointMomentLoad1Table.hpp"
#include "oor/model/cards/loads/pointMomentLoad2/PointMomentLoad2Table.hpp"

namespace oor::analysis
{

namespace
{

// A force or a moment on a scalar point: it has a single component and no direction, so FORCE and MOMENT cannot act
// there (SLOAD is the load of a scalar point).
class ForceOnScalarPointCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"load.force-on-non-grid", base::Severity::Error, "loads",
                                    "A point force or point moment card is applied to a scalar point, not a grid "
                                    "point."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        scan<model::PointForceLoadTable>(context, sink);
        scan<model::PointMomentLoadTable>(context, sink);
        scan<model::PointForceLoad1Table>(context, sink);
        scan<model::PointForceLoad2Table>(context, sink);
        scan<model::PointMomentLoad1Table>(context, sink);
        scan<model::PointMomentLoad2Table>(context, sink);
    }

private:
    template <typename LoadCombinationTable>
    static void scan(const ValidationContext& context, FindingSink& sink)
    {
        const model::Model& model = context.model();
        forEachCard<LoadCombinationTable>(
            model,
            [&](const typename LoadCombinationTable::CardType& card, model::CardLocation where)
            {
                if (isScalarPoint(model, card.node))
                    sink.reportCard(where, "node", "point " + std::to_string(card.node) + " is a scalar point",
                                    "use " + std::string("ScalarPointLoad") + " for a load on a scalar point");
            });
    }
};

} // namespace

void addForceOnScalarPointCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<ForceOnScalarPointCheck>());
}

} // namespace oor::analysis
