#include "oor/analysis/validation/design/DesignChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/optimisation/designConstants/DesignConstantsTable.hpp"
#include "oor/model/cards/optimisation/designElemRelation2/DesignElemRelation2Table.hpp"
#include "oor/model/cards/optimisation/designFieldConstants/DesignFieldConstantsTable.hpp"
#include "oor/model/cards/optimisation/designMatRelation2/DesignMatRelation2Table.hpp"
#include "oor/model/cards/optimisation/designPropRelation2/DesignPropRelation2Table.hpp"
#include "oor/model/cards/optimisation/designResp2/DesignResp2Table.hpp"
#include "oor/model/cards/optimisation/designResp3/DesignResp3Table.hpp"

#include <set>
#include <string>
#include <vector>

namespace oor::analysis
{

namespace
{

// The constants of the "DTABLE" lines of a DVPREL2, DVMREL2, DVCREL2, DRESP2 or DRESP3 are named by label, which is not
// an ID that `ref.unresolved` follows: the label must be one a DTABLE or a DTABLE2 entry defines,
// whatever the case of the letters. The check needs the whole list of constants, so a deck that has a
// DTABLE or DTABLE2 the library keeps as text (one it could not read) is not judged.
class UndefinedConstantCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "design.undefined-constant", base::Severity::Error, "design",
        "A design relation or response names a constant that no design constant table defines."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        // a card kept as text has the name the deck gives it: the keyword of its type
        const std::set<std::string> unread = unknownCardNames(model);
        if (unread.contains(std::string("DesignConstants")) || unread.contains(std::string("DesignFieldConstants")))
            return;
        std::set<std::string> defined;
        forEachCard<model::DesignConstantsTable>(model,
                                                 [&](const model::DesignConstantsCard& card, model::CardLocation)
                                                 {
                                                     for (const model::DesignConstantsEntry& entry : card.entries)
                                                         defined.insert(std::string(entry.label));
                                                 });
        forEachCard<model::DesignFieldConstantsTable>(
            model,
            [&](const model::DesignFieldConstantsCard& card, model::CardLocation)
            {
                for (const model::DesignFieldConstantsEntry& entry : card.entries)
                    defined.insert(std::string(entry.label));
            });

        const auto judge = [&](model::CardLocation where, const std::vector<std::string>& labels)
        {
            for (const std::string& label : labels)
            {
                if (!defined.contains(std::string(label)))
                    sink.reportCard(
                        where, "constantLabels",
                        "the constant " + label + " is not defined by a " + std::string("DesignConstants") + " or a " +
                            std::string("DesignFieldConstants"),
                        "define it with a " + std::string("DesignConstants") + " entry, or correct the label");
            }
        };
        forEachCard<model::DesignPropRelation2Table>(
            model,
            [&](const model::DesignPropRelation2Card& card, model::CardLocation where) { judge(where, card.labels); });
        forEachCard<model::DesignMatRelation2Table>(
            model,
            [&](const model::DesignMatRelation2Card& card, model::CardLocation where) { judge(where, card.labels); });
        forEachCard<model::DesignElemRelation2Table>(
            model,
            [&](const model::DesignElemRelation2Card& card, model::CardLocation where) { judge(where, card.labels); });
        forEachCard<model::DesignResp2Table>(
            model,
            [&](const model::DesignResp2Card& card, model::CardLocation where)
            {
                if (const model::DesignGroup* group = model::findDesignGroup(card.groups, "DesignConstants"))
                    judge(where, group->words);
            });
        forEachCard<model::DesignResp3Table>(
            model,
            [&](const model::DesignResp3Card& card, model::CardLocation where)
            {
                if (const model::DesignGroup* group = model::findDesignGroup(card.groups, "DesignConstants"))
                    judge(where, group->words);
            });
    }
};

} // namespace

void addUndefinedConstantCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<UndefinedConstantCheck>());
}

} // namespace oor::analysis
