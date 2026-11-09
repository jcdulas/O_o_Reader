#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/core/CardProblem.hpp"
#include "oor/model/references/ReferenceVisitor.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

// What the cards that associate name and value pairs with a group of meta data (ABSNMVB, ABSNMVI, ABSNMVR, ABSNMVS)
// share, one kind of value each: the NAME of a pair, and the reference to the group (ABSTRCT) the pairs belong to. Each
// card still lays out, reads, writes and checks its own fields by hand.
namespace oor::model
{

[[nodiscard]] inline std::optional<CardProblem> metadataNameProblem(std::string_view name)
{
    return checkNameWord(name, "entryName");
}

// The ABSTID of a pair card is the ID of a meta data group: the ABSTID of the ABSTRCT cards, which make the group of
// the entities of several cards that share it. The card is filed under that ID too (a set ID: IdSpace::Group, category
// Other: the group is the category MetadataGroup of the ABSTRCT).
inline void visitMetadataGroup(ReferenceVisitor& visitor, base::EntityId& metadataGroupId)
{
    visitId(visitor, "metadataGroupId", ReferenceTarget::MetadataGroup, metadataGroupId);
}

} // namespace oor::model
