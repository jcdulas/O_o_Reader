#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/Model.hpp"

#include <unordered_set>
#include <vector>

namespace oor::model
{

// The superelement numbers a deck defines, found once and looked up many times (the reference checker asks for every
// SEID of every card; a renumbering of the superelements needs the whole list).
//
// A number is defined when the deck has the part BEGIN SUPER=n (a part that carries a MASSID defines nothing: it is a
// mass increment of its superelement), when a card of the family that defines superelements names it (SESET, SEELT,
// SEBULK, CSUPEXT, CSUPER: the cards of the category Superelement that have an ID space of their own), or when a grid
// point of the residual structure puts itself in it with the SEID of its GRID card (or of the GRDSET that gave it
// one). Number 0, the residual structure, is always defined and is not in the list.
//
// The list is taken from the residual structure of the deck the model belongs to, so asking from a part gives the same
// answer as asking from the residual structure.
class SuperelementIds
{
public:
    explicit SuperelementIds(const Model& model);

    // True for 0 and for every number of the list.
    [[nodiscard]] bool contains(base::EntityId id) const noexcept { return id == 0 || m_set.contains(id); }
    // The numbers, in ascending order (0 is not one of them).
    [[nodiscard]] const std::vector<base::EntityId>& sorted() const noexcept { return m_sorted; }

private:
    std::unordered_set<base::EntityId> m_set;
    std::vector<base::EntityId> m_sorted;
};

// The mass increments the deck defines (the MASSID of BEGIN MASSID=n and of BEGIN SUPER=m MASSID=n). 0 is the base mass
// and is not in the set.
[[nodiscard]] std::unordered_set<base::EntityId> massIncrementsOf(const Model& model);

} // namespace oor::model
