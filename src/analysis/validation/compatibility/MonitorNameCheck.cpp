#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/matrices/monitorChordNormalCoeffs/MonitorChordNormalCoeffsTable.hpp"
#include "oor/model/cards/matrices/monitorDisplacement/MonitorDisplacementTable.hpp"
#include "oor/model/cards/matrices/monitorElemLoad2/MonitorElemLoad2Table.hpp"
#include "oor/model/cards/matrices/monitorGroup/MonitorGroupTable.hpp"
#include "oor/model/cards/matrices/monitorIntegratedLoad1/MonitorIntegratedLoad1Table.hpp"
#include "oor/model/cards/matrices/monitorNodeForceSum3/MonitorNodeForceSum3Table.hpp"
#include "oor/model/cards/matrices/monitorSum/MonitorSumTable.hpp"
#include "oor/model/cards/matrices/monitorSumLocated/MonitorSumLocatedTable.hpp"
#include "oor/model/cards/matrices/monitorSumTotal/MonitorSumTotalTable.hpp"

#include <map>
#include <string>

namespace oor::analysis
{

namespace
{

// How many cards of the deck define each monitor result name. The name of a monitor point or of a sum is the key of its
// card; a structural and an aerodynamic MONPNT1 may share one, so the cards are
// counted.
using NameCounts = std::map<std::string, int>;

void countDefinitions(NameCounts& counts, const model::Model& model)
{
    forEachCard<model::MonitorIntegratedLoad1Table>(
        model, [&](const model::MonitorIntegratedLoad1Card& card, model::CardLocation) { ++counts[card.name]; });
    forEachCard<model::MonitorElemLoad2Table>(
        model, [&](const model::MonitorElemLoad2Card& card, model::CardLocation) { ++counts[card.name]; });
    forEachCard<model::MonitorNodeForceSum3Table>(
        model, [&](const model::MonitorNodeForceSum3Card& card, model::CardLocation) { ++counts[card.name]; });
    forEachCard<model::MonitorDisplacementTable>(
        model, [&](const model::MonitorDisplacementCard& card, model::CardLocation) { ++counts[card.name]; });
    forEachCard<model::MonitorChordNormalCoeffsTable>(
        model, [&](const model::MonitorChordNormalCoeffsCard& card, model::CardLocation) { ++counts[card.name]; });
    forEachCard<model::MonitorSumTable>(
        model, [&](const model::MonitorSumCard& card, model::CardLocation) { ++counts[card.name]; });
    forEachCard<model::MonitorSumLocatedTable>(
        model, [&](const model::MonitorSumLocatedCard& card, model::CardLocation) { ++counts[card.name]; });
    forEachCard<model::MonitorSumTotalTable>(
        model, [&](const model::MonitorSumTotalCard& card, model::CardLocation) { ++counts[card.entryName]; });
}

// The monitor results that a MONSUM, MONSUM1, MONSUMT or MONGRP names are matched by NAME, not by an ID, against the
// cards that define a monitor result: the monitor points MONPNT1, MONPNT2, MONPNT3, MONDSP1 and MONCNCM and the sums
// themselves (a sum may add the result of another one). The names of the whole deck
// count, because a deck may keep its monitor points in a part. A sum whose NAME is the one it adds scales that monitor
// result: another card has to define it.
class MonitorNameCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"compat.monitor-name", base::Severity::Warning, "compatibility",
                                    "A monitor sum or monitor group names a monitor result that no card of the deck "
                                    "defines."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        NameCounts counts;
        for (const model::Model* each : context.allModels())
            countDefinitions(counts, *each);
        const model::Model& model = context.model();

        // `owner` is the NAME of the card that names `name` (empty for a monitor group, whose own name is not a monitor
        // result).
        const auto report =
            [&](const std::string& owner, const std::string& name, model::CardLocation where, const char* field)
        {
            const std::string key = std::string(name);
            const auto found = counts.find(key);
            int definitions = found == counts.end() ? 0 : found->second;
            if (key == owner)
                --definitions; // the sum is one of the cards that has the name: it does not define it for itself
            if (definitions > 0)
                return;
            sink.reportCard(where, field, "names the monitor result " + name + ", which no card of the deck defines",
                            "define it with a " +
                                cardLabels(model, {"MonitorIntegratedLoad1", "MonitorElemLoad2", "MonitorNodeForceSum3",
                                                   "MonitorDisplacement", "MonitorChordNormalCoeffs"}) +
                                " card, or correct the name");
        };

        forEachCard<model::MonitorSumTable>(model,
                                            [&](const model::MonitorSumCard& card, model::CardLocation where)
                                            {
                                                for (const model::MonitorSumTerm& term : card.body.terms)
                                                    report(card.name, term.name, where, "termNames");
                                            });
        forEachCard<model::MonitorSumLocatedTable>(
            model,
            [&](const model::MonitorSumLocatedCard& card, model::CardLocation where)
            {
                for (const model::MonitorSumTerm& term : card.body.terms)
                    report(card.name, term.name, where, "termNames");
            });
        forEachCard<model::MonitorSumTotalTable>(model,
                                                 [&](const model::MonitorSumTotalCard& card, model::CardLocation where)
                                                 {
                                                     for (const model::MonitorSumTotalMember& member : card.members)
                                                         report(card.entryName, member.name, where, "memberNames");
                                                 });
        forEachCard<model::MonitorGroupTable>(model,
                                              [&](const model::MonitorGroupCard& card, model::CardLocation where)
                                              {
                                                  for (const model::MonitorGroupMember& member : card.members)
                                                      report({}, member.name, where, "memberNames");
                                              });
    }
};

} // namespace

void addMonitorNameCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<MonitorNameCheck>());
}

} // namespace oor::analysis
