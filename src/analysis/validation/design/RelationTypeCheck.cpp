#include "oor/analysis/validation/design/DesignChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/optimisation/designPropRelation2/DesignPropRelation2Table.hpp"

namespace oor::analysis
{

namespace
{

// the property cards PBEND, PBARL and PBEAML are not supported by a relation that has an equation. (A
// DVPREL1 designs them: their dimensions are the fields the DVPREL1 names.)
class RelationTypeCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "design.relation-type", base::Severity::Error, "design",
        "A design property relation by equation names a property card that the format reference says it does not "
        "support."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        forEachCard<model::DesignPropRelation2Table>(
            context.model(),
            [&](const model::DesignPropRelation2Card& card, model::CardLocation where)
            {
                if (isOneOf(card.kind, {"CurvedBeamProp", "BarShapeProp", "BeamShapeProp"}))
                {
                    const model::Model& model = context.model();
                    sink.reportCard(
                        where, "kind",
                        "a " + std::string("DesignPropRelation2") + " does not support the property type " + card.kind,
                        "use a " + std::string("DesignPropRelation1") + " for the dimensions, or a " +
                            std::string("DesignPropRelation2") + " on a " + cardLabels(model, {"BarProp", "BeamProp"}));
                }
            });
    }
};

} // namespace

void addRelationTypeCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<RelationTypeCheck>());
}

} // namespace oor::analysis
