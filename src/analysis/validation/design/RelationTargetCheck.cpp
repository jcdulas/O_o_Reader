#include "oor/analysis/validation/design/DesignChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/design/DesignModel.hpp"

namespace oor::analysis
{

namespace
{

// The PNAME or FID of a DVPREL, the MPNAME of a DVMREL and the CPNAME of a DVCREL must be a field of the card they
// drive (T on a PROD, A(B) on a PBAR with no end B, the 12I/T**3 of a PSHELL by name): the relation drives nothing the
// model can write. The functions of each card type that has them say which names and positions are fields
// (model/design); a card type without functions is not judged, since the library does not know its fields.
class RelationTargetCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"design.relation-target", base::Severity::Warning, "design",
                                    "A design relation names a field that the card it drives does not have, or that a "
                                    "relation cannot drive."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::DesignModel design = model::DesignModel::read(context.model());
        for (const model::DesignRelation& relation : design.relations())
        {
            if (!relation.badField)
                continue;
            const char* field = relation.targetKind == model::DesignTargetKind::Property   ? "fieldName"
                                : relation.targetKind == model::DesignTargetKind::Material ? "mpNameValue"
                                                                                           : "cpNameValue";
            sink.reportCard(model::CardLocation{relation.cardType, relation.row}, field, relation.problem,
                            "name a field of the card, by its name in the format reference or by its position");
        }
    }
};

} // namespace

void addRelationTargetCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<RelationTargetCheck>());
}

} // namespace oor::analysis
