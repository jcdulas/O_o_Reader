#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/loads/LoadChecks.hpp"

#include <set>

namespace oor::analysis
{

namespace
{

// A set of loads that no subcase selects (LOAD, DLOAD, TEMPERATURE) and no LOAD or dynamic load combines. Only a deck
// with a Case Control section is judged: without one nothing selects anything.
class UnusedLoadSetCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"load.unused-set",
                                    base::Severity::Info,
                                    "loads",
                                    "A set of load cards is selected by no Case Control command and used by no other "
                                    "card.",
                                    true,
                                    CheckScope::Deck};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        if (model.control().caseControl.isEmpty())
            return;
        const ReferenceCensus& census = context.references();
        std::set<base::EntityId> reported;
        forEachCardOfCategory(model, model::CardCategory::Load, base::IdSpace::Group,
                              [&](model::CardLocation where)
                              {
                                  const base::EntityId sid = model.findTable(where.table)->idAt(where.row);
                                  if (census.loadSets.contains(sid) || !reported.insert(sid).second)
                                      return;
                                  sink.reportCard(where, idFieldAt(model, where),
                                                  "no subcase selects load set " + std::to_string(sid),
                                                  "select it with LOAD, or delete it");
                              });
    }
};

} // namespace

void addUnusedLoadSetCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<UnusedLoadSetCheck>());
}

} // namespace oor::analysis
