#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/superelement/SuperelementChecks.hpp"
#include "oor/model/cards/superelements/superelementInteriorNodes/SuperelementInteriorNodesTable.hpp"
#include "oor/model/references/ReferenceChecker.hpp"

namespace oor::analysis
{

namespace
{

// How many missing points a message names before it says "and others".
constexpr std::size_t MAX_NAMED = 5;

// A grid point or scalar point named by a SuperelementInteriorNodes that no card of the main Bulk Data defines. The
// format reference does not make it an error: the point is simply not in the superelement, which is
// rarely what the list meant. The points inside a THRU range are not looked at, a range may skip IDs. A deck that holds
// grid cards the library does not read (GRIDG, a GRID with an ID that does not fit 32 bits) may define the point there,
// so it is left alone: the names of the unread cards are deck keywords.
class SuperelementInteriorNodesMissingNodeCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "superelement.superelement-interior-nodes-missing-node", base::Severity::Warning, "superelement",
        "A superelement interior point list names a grid or scalar point that no card of the main Bulk Data defines."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        if (model.partInfo() || readsPointsAsText(model))
            return;
        forEachCard<model::SuperelementInteriorNodesTable>(
            model,
            [&](const model::SuperelementInteriorNodesCard& card, model::CardLocation where)
            {
                std::string missing;
                std::size_t count = 0;
                for (const model::IdRun& run : card.grids)
                {
                    if (run.first != run.last || model::referenceExists(model, model::ReferenceTarget::Node, run.first))
                        continue;
                    if (count++ < MAX_NAMED)
                        missing += (missing.empty() ? "" : ", ") + std::to_string(run.first);
                }
                if (count == 0)
                    return;
                sink.reportCard(where, "nodes",
                                "superelement " + std::to_string(card.superelementId) + " lists the point " + missing +
                                    (count > MAX_NAMED ? " and " + std::to_string(count - MAX_NAMED) + " others" : "") +
                                    ", which no " + std::string("Node") + " or " + std::string("ScalarPoint") +
                                    " of the main Bulk Data defines",
                                "define the point, or take it out of the list");
            });
    }

private:
    static bool readsPointsAsText(const model::Model& model)
    {
        const std::set<std::string> unread = unknownCardNames(model);
        return unread.contains("GRID") || unread.contains("GRIDG") || unread.contains("GRIDB") ||
               unread.contains("SPOINT");
    }
};

} // namespace

void addSuperelementInteriorNodesMissingNodeCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<SuperelementInteriorNodesMissingNodeCheck>());
}

} // namespace oor::analysis
