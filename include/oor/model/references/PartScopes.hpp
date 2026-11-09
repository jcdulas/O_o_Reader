#pragma once

#include "oor/model/core/Model.hpp"

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace oor::model
{

// Which models of a deck a reference may find its ID in, besides the model of its own card. Each part of a deck is a
// model with its own ID spaces, but some parts take IDs from other models of the same deck:
//
// - a part superelement that is the REPEAT or MIRROR image of another (SEBULK) has what its image has;
// - a mass increment (BEGIN MASSID=m, BEGIN SUPER=n MASSID=m) adds masses to its base model, whose grid points,
//   coordinate systems and properties its cards name: the residual structure, or the part of superelement n. The format
//   (BEGIN BULK: "incremental mass case") does not say what such a section may name; the library takes what the
//   section does not define from its base model, which is what a mass increment applies to.
// - a module section opened again with APPEND adds to the first section of that module, and a secondary module (an
//   MDBULK with an RMODID) inherits the Bulk Data of its primary module;
// - the active and passive sub-models of single-step TPA (BEGIN ACTIVE, BEGIN PASSIVE) and the main Bulk Data are one
//   model split in three: the connectors of the active side name the interface points of the passive side, and the
//   TPADEFN of the main Bulk Data names them too.
//
// Stateless; the models it returns belong to the deck of the model it is given.
struct PartScopes
{
    // The models whose IDs the cards of `model` may name after its own, in lookup order (none for a model that is not
    // part of a deck, or whose parts share nothing).
    [[nodiscard]] static std::vector<const Model*> sharedModels(const Model& model);

    // The models where an ID that a card places in the part `BEGIN <kind>=<id>` lives, in lookup order: that part and
    // the models it shares IDs with. For a module, the main Bulk Data is module 0 and a module with no section of its
    // own is looked up in the primary module it copies. Nothing (std::nullopt) when the deck cannot tell: a module with
    // no section and no copy in the deck (an external module made by an earlier run, EXTMDOUT). An empty list when the
    // part should be there and is not (a TRMC that the deck does not have).
    [[nodiscard]] static std::optional<std::vector<const Model*>> modelsOfPart(const Model& anyModelOfDeck,
                                                                               std::string_view kind, std::int64_t id);
};

} // namespace oor::model
