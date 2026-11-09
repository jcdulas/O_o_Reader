#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/solution/SolutionChecks.hpp"
#include "oor/model/cards/solution_control/rotorRelativeSpin/RotorRelativeSpinTable.hpp"
#include "oor/model/cards/solution_control/rotorTransientSpin/RotorTransientSpinTable.hpp"

#include <set>
#include <utility>

namespace oor::analysis
{

namespace
{

// The pair of a rotor and the superelement it is defined in must be unique among the RSPINR cards, and among the RSPINT
// cards (the ROTORID and the ROTRSEID fields need not be unique one by one). An
// RSPINR and an RSPINT of one rotor are for different analyses and do not count.
class RotorSpinRepeatedCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"solution.rotor-spin-repeated", base::Severity::Error, "solution",
                                    "Two relative spin cards, or two transient spin cards, have the same rotor and "
                                    "rotor superelement numbers."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        using Pair = std::pair<base::EntityId, std::int32_t>;
        const model::Model& model = context.model();
        std::set<Pair> relative;
        const std::string relativeField = std::string("rotorSuperelementId");
        forEachCard<model::RotorRelativeSpinTable>(
            model, [&](const model::RotorRelativeSpinCard& card, model::CardLocation where)
            { report(sink, where, relative, card.rotorId, card.rotorSuperelementId, relativeField); });
        std::set<Pair> transient;
        const std::string transientField = std::string("rotorSuperelementId");
        forEachCard<model::RotorTransientSpinTable>(
            model, [&](const model::RotorTransientSpinCard& card, model::CardLocation where)
            { report(sink, where, transient, card.rotorId, card.rotorSuperelementId, transientField); });
    }

private:
    // Reports the card when an earlier card of its kind has the same rotor and superelement; `superelementField` is
    // the keyword of the superelement field of the card.
    static void report(FindingSink& sink, model::CardLocation where,
                       std::set<std::pair<base::EntityId, std::int32_t>>& seen, base::EntityId rotorid,
                       std::int32_t superelement, const std::string& superelementField)
    {
        if (seen.emplace(rotorid, superelement).second)
            return;
        sink.reportCard(where, "rotorId",
                        "an earlier card has the rotor " + std::to_string(rotorid) + " in the superelement " +
                            std::to_string(superelement),
                        "give the card another rotor, or another " + superelementField + ", or remove it");
    }
};

} // namespace

void addRotorSpinRepeatedCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<RotorSpinRepeatedCheck>());
}

} // namespace oor::analysis
