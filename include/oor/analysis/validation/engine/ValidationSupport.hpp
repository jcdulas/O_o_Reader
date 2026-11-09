#pragma once

#include "oor/analysis/validation/engine/ValidationCheck.hpp"
#include "oor/analysis/validation/engine/ValidationContext.hpp"
#include "oor/model/cards/parameters/parameter/ParameterCard.hpp"
#include "oor/model/control/case_control/CaseControl.hpp"
#include "oor/model/control/case_control/CaseLineages.hpp"
#include "oor/model/core/CardRegistry.hpp"
#include "oor/model/core/Model.hpp"
#include "oor/model/references/ReferenceVisitor.hpp"

#include <cstdint>
#include <initializer_list>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

// Small helpers the checks share: how to walk the cards of a type, the subcases of the Case Control, and how to say
// things in findings. Nothing here describes the fields of a card.

namespace oor::analysis
{

// Calls fn(card, location) for every live card of a table type, in row order.
template <typename TableT, typename Fn>
void forEachCard(const model::Model& model, Fn&& fn)
{
    const TableT* table = model.findTable<TableT>();
    if (table == nullptr)
        return;
    const std::uint16_t type = model::typeIndexOf<TableT>();
    const std::uint32_t count = static_cast<std::uint32_t>(table->rows().rowCount());
    for (std::uint32_t row = 0; row < count; ++row)
    {
        if (table->rows().isAlive(row))
            fn(table->card(row), model::CardLocation{type, row});
    }
}

// Calls fn(location) for every live card of the card types of a category (and, when given, an ID space), in the order
// of the registry then of the rows.
template <typename Fn>
void forEachCardOfCategory(const model::Model& model, model::CardCategory category, base::IdSpace space, Fn&& fn)
{
    const model::CardRegistry& registry = model::CardRegistry::instance();
    for (std::uint16_t type = 1; type < registry.size(); ++type)
    {
        const model::CardTable* table = model.findTable(type);
        if (table == nullptr || registry.entry(type).info.category != category ||
            registry.entry(type).info.idSpace != space)
            continue;
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (table->rows().isAlive(row))
                fn(model::CardLocation{type, row});
        }
    }
}

// True when a location is a card of this table type.
template <typename TableT>
[[nodiscard]] bool isType(model::CardLocation location)
{
    return location.table == model::typeIndexOf<TableT>();
}

// The card of this table type at a location (which must be one).
template <typename TableT>
[[nodiscard]] typename TableT::CardType cardAt(const model::Model& model, model::CardLocation location)
{
    return static_cast<const TableT&>(*model.findTable(location.table)).card(location.row);
}

// What a reference target is, for a message: "node", "property", "load set", ...
[[nodiscard]] std::string_view describeTarget(model::ReferenceTarget target) noexcept;

// The name of the card type of a location ("PSHELL").
[[nodiscard]] std::string_view cardNameAt(const model::Model& model, model::CardLocation location);
// The neutral identifier field of this card type, as declared first in its compact layout.
[[nodiscard]] std::string idFieldAt(const model::Model& model, model::CardLocation location);
[[nodiscard]] std::string cardLabels(const model::Model& model, std::initializer_list<std::string_view> cardTypes,
                                     std::string_view conjunction = "or");
// The keyword of a field of a card type in a message ("ZOFFS" for the field `offset` of ShellQuad4).
// True when `name` is one of `names`.
[[nodiscard]] bool isOneOf(std::string_view name, std::initializer_list<std::string_view> names) noexcept;
// The PARAM of this name (the last one when it is given twice), nothing when the model has none.
[[nodiscard]] std::optional<model::ParameterCard> findParameter(const model::Model& model, std::string_view name);

// The names of the cards of the model that the library keeps as text (it does not read them), upper case.
[[nodiscard]] std::set<std::string> unknownCardNames(const model::Model& model);
// True when the deck is a heat transfer analysis (APP HEAT): the structural rules about constraints do not apply.
[[nodiscard]] bool isHeatTransfer(const model::Model& root);

// True when the Bulk Data has PARAM,HEATSTAT,YES: a SOL 101 deck that computes a steady-state heat transfer in its
// first subcase and a thermal stress analysis in the second (format reference, PARAM,HEATSTAT), so that it holds
// thermal and structural materials, and its TEMPERATURE(LOAD) names the heat subcase.
[[nodiscard]] bool isHeatThenStress(const model::Model& root);

// The part that a part copies: when the superelement of `model` is a REPEAT or MIRROR image of another (its SEBULK says
// so), the cards of the image name the IDs of the original (the PLOAD2 of a mirrored superelement loads the elements of
// the part it mirrors). Null for the residual structure, for a part that is not an image and for an image of nothing.
[[nodiscard]] const model::Model* imageOfPart(const model::Model& model);

// True when a node ID is a grid point of the model (a scalar point is not).
[[nodiscard]] bool isGridPoint(const model::Model& model, base::EntityId nid);
// True when a node ID is a scalar point.
[[nodiscard]] bool isScalarPoint(const model::Model& model, base::EntityId nid);

// The sections of the Case Control that hold an analysis: the top-level subcases (SUBCASE, SUBCOM, SYM, SYMCOM,
// REPCASE), in file order. When there is no subcase at all, the global section stands for the one analysis (the
// commands above the first subcase make a deck with no SUBCASE a single-case deck). The sections after an AUXCASE
// analyse an auxiliary model and are not among them.
[[nodiscard]] std::vector<const model::Subcase*> analysisSubcases(const model::CaseControl& caseControl);
// The sections that hold an analysis of an auxiliary model, in file order: for each AUXCASE section its top-level
// subcases, or the AUXCASE section itself when it has none (as the global section stands for a deck with no SUBCASE).
[[nodiscard]] std::vector<const model::Subcase*> auxiliarySubcases(const model::CaseControl& caseControl);
// The effective integer value of a command of a subcase (its own, else the one it inherits); nothing when the
// command is absent or not an integer. The lineages of the Case Control are made once for a loop over its subcases.
[[nodiscard]] std::optional<std::int64_t> effectiveInteger(const model::CaseLineages& lineages,
                                                           const model::Subcase& subcase, std::string_view command);
// True when the subcase has the command, its own or inherited.
[[nodiscard]] bool hasEffectiveCommand(const model::CaseLineages& lineages, const model::Subcase& subcase,
                                       std::string_view command);

// The solution sequences by kind, for the checks that depend on the type of analysis.
[[nodiscard]] bool isStaticSolution(std::int64_t sol) noexcept;  // 101, 106, 153, 400 (static subcases), 144, 153
[[nodiscard]] bool isDynamicSolution(std::int64_t sol) noexcept; // normal modes and the dynamic response sequences

// The sets that the constraint set `id` stands for: itself and, through the SPCADD (command
// model::SINGLE_POINT_CONSTR_SELECTION) or MPCADD (command model::MULTI_POINT_CONSTR_SELECTION) cards that combine
// sets, every set it contains, sorted.
[[nodiscard]] std::vector<base::EntityId> expandedConstraintSet(const model::Model& model, std::string_view command,
                                                                base::EntityId id);

// The constraint sets the Case Control selects with the command model::SINGLE_POINT_CONSTR_SELECTION or
// model::MULTI_POINT_CONSTR_SELECTION (`command`), one selection for each distinct choice of the analysis subcases: the
// sets of the card IDs the command names, each expanded through the SPCADD or MPCADD that combines them (the leaf IDs
// and the combining ID, sorted). A subcase without the command makes no selection. When no subcase has the command (a
// deck read without Case Control, or a model made in memory) the one selection is every set of the Bulk Data cards of
// that command (SPC and SPC1 cards, or MPC cards).
[[nodiscard]] std::vector<std::vector<base::EntityId>> constraintSelections(const model::Model& model,
                                                                            std::string_view command);

} // namespace oor::analysis
