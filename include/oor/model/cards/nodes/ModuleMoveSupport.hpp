#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/core/CardProblem.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <string>

// What the cards that reposition a module (MDTRAN, MDROT1, MDROT2, MDMIR1, MDMIR2, MDMOVE, MDMPLN, MDLOC) have in
// common: the points they name to define a vector or a plane. One kind of value; every card still lays out, reads,
// writes and checks its own fields by hand.
//
// A module is a BEGIN MODULE section, a part of the deck that no card defines, so a module number (MVID, MODID) is a
// plain integer and never a reference. The GRID or POINT IDs of these cards name points of the main Bulk Data (module
// 0), except the PAi of an MDLOC, which are points of the module that is moved.
namespace oor::model
{

// The problem with the points that define a vector (two points) or a plane (three), if there is one: each is a
// positive ID, and no ID is used twice, because a point repeated cannot give a direction nor a plane (the format
// reference asks for three non-collinear points). The collinearity itself needs the coordinates, so it is not a rule of
// the card.
template <std::size_t COUNT>
[[nodiscard]] std::optional<CardProblem> checkDistinctModulePoints(const std::array<base::EntityId, COUNT>& points,
                                                                   const std::array<const char*, COUNT>& names)
{
    for (std::size_t index = 0; index < COUNT; ++index)
    {
        if (const auto problem = checkRequiredId(points[index], names[index]))
            return problem;
        for (std::size_t earlier = 0; earlier < index; ++earlier)
        {
            if (points[earlier] == points[index])
                return CardProblem{"card.bad-value",
                                   std::string(CardProblem::FIELD) + " must not be the same point as " +
                                       CardProblem::fieldNamed(names[earlier]),
                                   names[index]};
        }
    }
    return std::nullopt;
}

} // namespace oor::model
