#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/properties/compProp/CompPropTable.hpp"
#include "oor/model/cards/properties/compPropGlobal/CompPropGlobalTable.hpp"

namespace oor::analysis
{

namespace
{

// A laminate whose plies have no thickness at all.
class LaminateThicknessCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"compat.laminate-thickness", base::Severity::Error, "compatibility",
                                    "A composite property has a total thickness of zero."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        forEachCard<model::CompPropTable>(context.model(),
                                          [&](const model::CompPropCard& card, model::CardLocation where)
                                          {
                                              if (!(card.totalThickness() > 0.0))
                                                  sink.reportCard(where, "plyThickness",
                                                                  "the plies add up to a thickness of zero",
                                                                  "give the plies a thickness");
                                          });
        forEachCard<model::CompPropGlobalTable>(context.model(),
                                                [&](const model::CompPropGlobalCard& card, model::CardLocation where)
                                                {
                                                    if (!(card.totalThickness() > 0.0))
                                                        sink.reportCard(where, "plyThickness",
                                                                        "the plies add up to a thickness of zero",
                                                                        "give the plies a thickness");
                                                });
    }
};

} // namespace

void addLaminateThicknessCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<LaminateThicknessCheck>());
}

} // namespace oor::analysis
