#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/nodes/moduleMoveSequence/ModuleMoveSequenceTable.hpp"
#include "oor/model/references/ReferenceChecker.hpp"

#include <string>

namespace oor::analysis
{

namespace
{

// A ModuleMoveSequence that names a mirror (ModuleMirror1 or ModuleMirror2) after its first move: the format reference
// wants the MVID of a
// mirror in the field MVID1, because the mirror is applied to the module as it stands and the moves after it are the
// ones that put it in place.
class ModuleMoveOrderCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"compat.module-move-order", base::Severity::Error, "compatibility",
                                    "A module move sequence names a module mirror in a field other than the first "
                                    "move."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        forEachCard<model::ModuleMoveSequenceTable>(
            model,
            [&](const model::ModuleMoveSequenceCard& card, model::CardLocation where)
            {
                for (std::size_t index = 1; index < card.moveIds.size(); ++index)
                {
                    for (const model::CardLocation& move :
                         model::cardsOfSet(model, model::CardCategory::ModuleMove, card.moveIds[index]))
                    {
                        const std::string_view name = cardNameAt(model, move);
                        if (name != "ModuleMirror1" && name != "ModuleMirror2")
                            continue;
                        sink.reportCard(where, "moveId",
                                        "the move " + std::to_string(card.moveIds[index]) + ", an " +
                                            std::string(name) + ", is in the field " + std::string("moveId") +
                                            std::to_string(index + 1) + " and a mirror must be in " +
                                            std::string("moveId1"),
                                        "put the mirror first in the list of moves");
                    }
                }
            });
    }
};

} // namespace

void addModuleMoveOrderCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<ModuleMoveOrderCheck>());
}

} // namespace oor::analysis
