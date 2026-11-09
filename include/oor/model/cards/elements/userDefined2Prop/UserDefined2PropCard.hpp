#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/elements/DummyFieldList.hpp"

namespace oor::model
{

// One PDUM2 card: the property of the user dummy elements of number 2 (the CDUM2 cards name it in their PID). The
// element routines that use it are written by the user; the ADUM2 card says how many additional fields it has.
//
// Card layout: PDUM2  PID  MID  A1  A2  A3  A4  A5  A6          fields 1 to 8, then the continuation lines
//                    A7   -etc.-
//
//   PID  the property ID: a positive integer, in the space of all the properties
//   MID  the material ID: a positive integer, a material of the model
//   Aj   the additional fields, up to 24 (the NP of the ADUM2 card, which is at most 24): integers or reals, which the
//        user's routines define and check. The list keeps them in the positions of the card, a blank included (a blank
//        Aj keeps its place, so the fields after it keep their numbers).
//
// The format reference of PDUMi says the number i runs from 3 to 7; the library reads 1 to 9 (see
// UserDefined2AttributesCard).
struct UserDefined2PropCard
{
    static constexpr std::size_t MOST_ADDITIONAL_FIELDS = 24;

    base::EntityId id = 0;
    base::EntityId matId = 0;
    DummyFieldList fields; // A1, A2, ...: integer, real or blank; never ends with a blank
};

} // namespace oor::model
