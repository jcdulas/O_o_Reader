#include "oor/analysis/validation/design/DesignChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/optimisation/designElemRelation2/DesignElemRelation2Table.hpp"
#include "oor/model/cards/optimisation/designEquation/DesignEquationTable.hpp"
#include "oor/model/cards/optimisation/designMatRelation2/DesignMatRelation2Table.hpp"
#include "oor/model/cards/optimisation/designPropRelation2/DesignPropRelation2Table.hpp"
#include "oor/model/cards/optimisation/designResp1/DesignResp1Table.hpp"
#include "oor/model/cards/optimisation/designResp2/DesignResp2Table.hpp"
#include "oor/model/expressions/EquationSet.hpp"

#include <string>
#include <unordered_map>

namespace oor::analysis
{

namespace
{

// The relations hand their values to the arguments of the function of the DEQATN by position: the design variables,
// then the constants, for a DVPREL2, DVMREL2 or DVCREL2; the groups of a DRESP2 in
// the order of the card, a grid point and its component being one value. An argument that no value
// reaches has nothing to compute with. The other way round (more values than arguments) the format reference says
// nothing and the check says nothing either.
//
// when the DRESP1 responses of the card are frequency responses (an RTYPE that begins with FR or PSD),
// the equation may have more arguments than the card has values: the missing ones, at the end, are the forcing
// frequencies of the responses. Such a DRESP2 is not judged.
class EquationArgumentsCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{
        "design.equation-arguments", base::Severity::Error, "design",
        "A design relation or response gives its equation fewer values than the equation has "
        "arguments."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        // The number of arguments of each equation that parses (one that does not is `design.equation-syntax`).
        std::unordered_map<base::EntityId, std::size_t> arguments;
        forEachCard<model::DesignEquationTable>(model,
                                                [&](const model::DesignEquationCard& card, model::CardLocation)
                                                {
                                                    const model::EquationParse parsed =
                                                        model::parseEquations(card.text);
                                                    if (parsed.ok())
                                                        arguments[card.id] = parsed.equations->parameters().size();
                                                });
        if (arguments.empty())
            return;

        const auto judge = [&](model::CardLocation where, base::EntityId eqid, std::size_t given)
        {
            const auto found = arguments.find(eqid);
            if (found == arguments.end() || given >= found->second)
                return;
            sink.reportCard(
                where, "equationId",
                std::string("DesignEquation") + " " + std::to_string(eqid) + " has " + std::to_string(found->second) +
                    " arguments and the card gives it " + std::to_string(given) + " values",
                "list the design variables and the constants the equation needs, in the order of its arguments");
        };
        forEachCard<model::DesignPropRelation2Table>(
            model, [&](const model::DesignPropRelation2Card& card, model::CardLocation where)
            { judge(where, card.equationId, card.designVarIds.size() + card.labels.size()); });
        forEachCard<model::DesignMatRelation2Table>(
            model, [&](const model::DesignMatRelation2Card& card, model::CardLocation where)
            { judge(where, card.equationId, card.designVarIds.size() + card.labels.size()); });
        forEachCard<model::DesignElemRelation2Table>(
            model, [&](const model::DesignElemRelation2Card& card, model::CardLocation where)
            { judge(where, card.equationId, card.designVarIds.size() + card.labels.size()); });

        std::unordered_map<base::EntityId, bool> frequencyResponse; // by DRESP1 ID
        forEachCard<model::DesignResp1Table>(model,
                                             [&](const model::DesignResp1Card& card, model::CardLocation)
                                             {
                                                 // the frequency responses (FreqDisplacement, ...), the spectral
                                                 // densities (SpectralDisplacement, ...), and Frequency and
                                                 // FractionalMass
                                                 const std::string& kind = card.responseKind;
                                                 frequencyResponse[card.id] = kind.starts_with("Freq") ||
                                                                              kind.starts_with("Spectral") ||
                                                                              kind == "FractionalMass";
                                             });
        forEachCard<model::DesignResp2Table>(
            model,
            [&](const model::DesignResp2Card& card, model::CardLocation where)
            {
                if (card.equationId == 0)
                    return;
                std::size_t given = 0;
                bool frequencies = false;
                for (const model::DesignGroup& group : card.groups)
                {
                    if (group.keyword == "DesignNodes")
                        given += group.ids.size() / 2;
                    else
                        given += group.ids.size() + group.words.size();
                    if (group.keyword == "DesignResp1")
                    {
                        for (const base::EntityId id : group.ids)
                        {
                            const auto found = frequencyResponse.find(id);
                            frequencies = frequencies || (found != frequencyResponse.end() && found->second);
                        }
                    }
                }
                if (!frequencies)
                    judge(where, card.equationId, given);
            });
    }
};

} // namespace

void addEquationArgumentsCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<EquationArgumentsCheck>());
}

} // namespace oor::analysis
