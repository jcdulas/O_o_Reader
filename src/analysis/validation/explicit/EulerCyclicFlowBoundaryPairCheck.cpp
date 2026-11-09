#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/explicit/ExplicitChecks.hpp"
#include "oor/model/cards/explicit/eulerCyclicFlowBoundary/EulerCyclicFlowBoundaryTable.hpp"

#include <string>
#include <unordered_map>

namespace oor::analysis
{

namespace
{

// A FLOWC whose pair is missing. The cyclic flow boundaries are defined in pairs: the FID2 of one is the FID of the
// other and the other way round (the format gives FLOWC,10,30,20 with FLOWC,20,40,10), because what flows out of one
// boundary comes in at the other.
class EulerCyclicFlowBoundaryPairCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"explicit.euler-cyclic-flow-boundary-pair", base::Severity::Error, "explicit",
                                    "A cyclic Euler flow boundary has no partner: the partner number of its partner is "
                                    "not its own number."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        const std::string partnerField = std::string("pairedBoundaryId");
        const std::string ownField = std::string("id");
        std::unordered_map<base::EntityId, base::EntityId> partnerOf; // FID -> FID2
        forEachCard<model::EulerCyclicFlowBoundaryTable>(
            model, [&](const model::EulerCyclicFlowBoundaryCard& card, model::CardLocation)
            { partnerOf.emplace(card.id, card.pairedBoundaryId); });
        forEachCard<model::EulerCyclicFlowBoundaryTable>(
            model,
            [&](const model::EulerCyclicFlowBoundaryCard& card, model::CardLocation where)
            {
                const auto partner = partnerOf.find(card.pairedBoundaryId);
                if (partner == partnerOf.end())
                    sink.reportCard(where, "pairedBoundaryId",
                                    partnerField + " " + std::to_string(card.pairedBoundaryId) + " is the " + ownField +
                                        " of no " + std::string("EulerCyclicFlowBoundary"),
                                    "define the other " + std::string("EulerCyclicFlowBoundary") + " of the pair");
                else if (partner->second != card.id)
                    sink.reportCard(
                        where, "pairedBoundaryId",
                        "the " + std::string("EulerCyclicFlowBoundary") + " " + std::to_string(card.pairedBoundaryId) +
                            " names " + std::to_string(partner->second) + " as its pair, not " +
                            std::to_string(card.id),
                        "make the " + partnerField + " of each of the two cards the " + ownField + " of the other");
            });
    }
};

} // namespace

void addEulerCyclicFlowBoundaryPairCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<EulerCyclicFlowBoundaryPairCheck>());
}

} // namespace oor::analysis
