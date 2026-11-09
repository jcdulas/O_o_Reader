#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/superelement/SuperelementChecks.hpp"
#include "oor/model/cards/superelements/SuperelementIds.hpp"
#include "oor/model/cards/superelements/massCaseCombination/MassCaseCombinationTable.hpp"

namespace oor::analysis
{

namespace
{

// A MASSSET whose ID is also the number of a mass increment (the MASSID of a BEGIN MASSID=n or BEGIN SUPER=m MASSID=n
// section): the Case Control MASSSET command selects one of the mass combinations, and the format reference asks for
// numbers that are unique among the mass combinations and the mass increments, so that the number names one thing.
class MassIdRepeatedCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"superelement.mass-id-repeated", base::Severity::Error, "superelement",
                                    "A mass combination has the number of a mass increment (the mass number of a "
                                    "part)."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        if (model.partInfo())
            return;
        const std::unordered_set<base::EntityId> increments = model::massIncrementsOf(model);
        if (increments.empty())
            return;
        forEachCard<model::MassCaseCombinationTable>(
            model,
            [&](const model::MassCaseCombinationCard& card, model::CardLocation where)
            {
                if (increments.contains(card.id))
                    sink.reportCard(where, "id",
                                    "the number " + std::to_string(card.id) +
                                        " is a mass combination here and the MASSID of a part of the deck",
                                    "give the combination a number that no MASSID has");
            });
    }
};

} // namespace

void addMassIdRepeatedCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<MassIdRepeatedCheck>());
}

} // namespace oor::analysis
