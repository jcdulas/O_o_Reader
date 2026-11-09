#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/loads/LoadChecks.hpp"
#include "oor/model/cards/dynamics/transientTableLoad1/TransientTableLoad1Table.hpp"

#include <optional>
#include <string>
#include <unordered_set>

namespace oor::analysis
{

namespace
{

// A TLOAD1 with no function of time: neither a table TID nor a constant F. The format reference wants one of them,
// except where the excitation is a flow boundary or a user-defined load of SOL 700 (FLOW, FLOWT, FLOWUDS, FORCUDS),
// whose formats say the TLOAD1 that names them has a blank TID: those cards define the excitation ID, and the check
// lets them through.
class TransientTableLoad1NoFunctionCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"load.transient-table-load1-no-function", base::Severity::Error, "loads",
                                    "A transient table load has neither a table nor a constant value, and its "
                                    "excitation names no flow boundary or user-defined load."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        std::optional<std::unordered_set<base::EntityId>> boundaries;
        forEachCard<model::TransientTableLoad1Table>(
            model,
            [&](const model::TransientTableLoad1Card& card, model::CardLocation where)
            {
                if (card.tableId != 0 || card.value != 0.0)
                    return;
                // The EXCITEID is looked up among the IDs that the flow boundaries and user loads define, and not as an
                // excitation set first: the set of DAREA or load cards of the same number (the TLOAD1 itself is one,
                // with its SID) would hide the FLOW of that number (TYPE 4, blank
                // TID).
                if (!boundaries)
                    boundaries = definedExcitationIds(model);
                if (boundaries->contains(card.excitationId))
                    return;
                // TID and the constant F share one field of the deck: its keyword names both.
                const std::string tableField = std::string("tableId");
                sink.reportCard(where, "tableId",
                                "the card has neither a table nor a non-zero constant in " + tableField,
                                "give in " + tableField + " the table of the function of time, or a constant");
            });
    }

private:
    // The excitation IDs that the cards which define one have (FLOW, FLOWT, FLOWC, FLOWUDS, FORCUDS, LSEQ, ...).
    static std::unordered_set<base::EntityId> definedExcitationIds(const model::Model& model)
    {
        std::unordered_set<base::EntityId> ids;
        const model::CardRegistry& registry = model::CardRegistry::instance();
        for (std::uint16_t type = 1; type < registry.size(); ++type)
        {
            const model::CardTable* table = model.findTable(type);
            if (table == nullptr || !table->definesExcitationIds())
                continue;
            for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
            {
                if (table->rows().isAlive(row))
                    ids.insert(table->definedExcitationId(row));
            }
        }
        return ids;
    }
};

} // namespace

void addTransientTableLoad1NoFunctionCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<TransientTableLoad1NoFunctionCheck>());
}

} // namespace oor::analysis
