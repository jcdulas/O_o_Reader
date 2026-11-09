#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/properties/barProp/BarPropTable.hpp"
#include "oor/model/cards/properties/rodProp/RodPropTable.hpp"

namespace oor::analysis
{

namespace
{

// A rod or bar with no cross-section: it has no axial stiffness and no mass (a PROD with a torsion constant alone is a
// legal shaft, so J counts for a PROD).
class ZeroSectionAreaCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"compat.zero-section-area", base::Severity::Warning, "compatibility",
                                    "A rod or bar property has a cross-section area of zero."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        forEachCard<model::RodPropTable>(context.model(),
                                         [&](const model::RodPropCard& card, model::CardLocation where)
                                         {
                                             if (card.area == 0.0 && card.torsion == 0.0)
                                                 sink.reportCard(where, "area",
                                                                 "the area " + std::string("area") +
                                                                     " and the torsion constant " +
                                                                     std::string("torsionConstant") + " are both zero",
                                                                 "give the rod a cross-section area");
                                         });
        forEachCard<model::BarPropTable>(context.model(),
                                         [&](const model::BarPropCard& card, model::CardLocation where)
                                         {
                                             if (card.area == 0.0)
                                                 sink.reportCard(where, "area",
                                                                 "the area " + std::string("area") + " is zero",
                                                                 "give the bar a cross-section area");
                                         });
    }
};

} // namespace

void addZeroSectionAreaCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<ZeroSectionAreaCheck>());
}

} // namespace oor::analysis
