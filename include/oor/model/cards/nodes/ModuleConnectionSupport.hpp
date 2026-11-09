#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/CardProblem.hpp"
#include "oor/model/references/ReferenceVisitor.hpp"

#include <optional>
#include <string>
#include <string_view>

// What the cards that connect modules (MDFAST, MDMPC, MDRBE2, MDRBE3, MDRJNT, MDRROD, MDSEAM, MDWELD) share, for one
// kind of value: an ID that lives in the module that another field of the same card names (the IDA of an MDFAST in
// MODA, the GA of an MDRJNT in MODA). ModuleIds.hpp holds the pair for the cards whose module field and ID field sit
// side by side; these cards put the module in a field of its own, often far from the ID, so the two are separate
// members of the card.
//
// A module is a BEGIN MODULE section of the deck, a part of the deck (a model of its own); module 0 (a blank module
// field) is the main Bulk Data. An ID that a card places in module n is looked up in the part of module n
// (ReferenceVisitor::visitInPart).
namespace oor::model
{

// The problem with the number of a module, if there is one: 0 is the main Bulk Data and a blank field, any other module
// is a positive number.
[[nodiscard]] inline std::optional<CardProblem> checkModuleNumber(base::EntityId module, const char* field)
{
    if (module < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must not be negative", field};
    return std::nullopt;
}

// Visits an ID whose card is in the module `module` as a reference to `target` there (the main Bulk Data for module 0,
// the part of the module otherwise), and stores what the visitor returns. A blank ID (0) is not visited.
inline void visitIdInModule(ReferenceVisitor& visitor, std::string_view field, ReferenceTarget target,
                            base::EntityId module, base::EntityId& id)
{
    if (id <= 0)
        return;
    id = module == 0 ? visitor.visit(field, target, id)
                     : visitor.visitInPart(field, target, id, "ModuleSection", module);
}

} // namespace oor::model
