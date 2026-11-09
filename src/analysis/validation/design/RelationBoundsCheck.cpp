#include "oor/analysis/validation/design/DesignChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/base/errors/Errors.hpp"
#include "oor/base/text/MessageNumber.hpp"
#include "oor/model/design/DesignModel.hpp"

namespace oor::analysis
{

namespace
{

// A relation gives its field a value at the initial values of the design variables; the limits of the relation (PMIN
// and PMAX, or the default PMIN of the field) must not exclude it, or the optimiser starts from a design that violates
// its own side constraint (and, with the default limits, from a thickness that cannot be positive). The relations that
// cannot be applied are not judged, and neither is a model whose design variables are not defined (a DLINK that names a
// variable that is not there: `ref.unresolved` reports it).
class RelationBoundsCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"design.relation-bounds", base::Severity::Warning, "design",
                                    "The value a design relation gives its field at the initial values of the design "
                                    "variables is outside the limits of the relation."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        const model::DesignModel design = model::DesignModel::read(model);
        if (design.relations().empty() || !design.problems().empty())
            return;
        model::DesignEvaluation evaluation;
        try
        {
            evaluation = design.evaluate(model, design.initialPoint(), false);
        }
        catch (const base::OorError&)
        {
            return;
        }
        for (std::size_t index = 0; index < design.relations().size(); ++index)
        {
            const model::DesignRelation& relation = design.relations()[index];
            const model::DesignRelationValue& value = evaluation.relations[index];
            if (!relation.applicable || !value.problem.empty() || value.limit == model::DesignLimitHit::None)
                continue;
            const bool below = value.limit == model::DesignLimitHit::Minimum;
            // The name of the limit on the card of the relation: PMIN and PMAX, MPMIN and MPMAX, CPMIN and CPMAX.
            const char* limitName = relation.targetKind == model::DesignTargetKind::Property ? (below ? "PMIN" : "PMAX")
                                    : relation.targetKind == model::DesignTargetKind::Material
                                        ? (below ? "MPMIN" : "MPMAX")
                                        : (below ? "CPMIN" : "CPMAX");
            const char* field =
                relation.targetKind == model::DesignTargetKind::Property   ? (below ? "pMinimum" : "pMaximum")
                : relation.targetKind == model::DesignTargetKind::Material ? (below ? "mpMinimum" : "mpMaximum")
                                                                           : (below ? "cpMinimum" : "cpMaximum");
            sink.reportCard(model::CardLocation{relation.cardType, relation.row}, field,
                            "at the initial values the relation gives " + relation.targetCard + " " +
                                std::to_string(relation.targetId) + " " + std::string(relation.facts.name) +
                                " the value " + base::MessageNumber::general(value.requested) + ", " +
                                (below ? "below" : "above") + " its " + limitName + " limit " +
                                base::MessageNumber::general(below ? relation.lowest : relation.highest),
                            "change the limit, the coefficients or the initial values of the design variables");
        }
    }
};

} // namespace

void addRelationBoundsCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<RelationBoundsCheck>());
}

} // namespace oor::analysis
