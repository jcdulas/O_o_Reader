#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One MDLOC card: the position of a module in the main model, given by three points of the module and the three points
// of module 0 that they must be moved onto. The module is rotated and translated until each PAi lies where PBi is.
//
// Card layout: MDLOC  MODID  PA1  PA2  PA3  PB1  PB2  PB3            fields 1 to 7
// MODID is the number of a primary, repeated or mirrored module, a plain integer (a BEGIN MODULE section is not a card
// of the model). PA1 to PA3 are GRID or POINT IDs of three non-collinear points in the Bulk Data of the module (the
// section of MODID, or the one that the MDBULK of a copied module names as RMODID): they are points of another
// section, so the card keeps them as plain IDs and no reference is looked up for them. PB1 to PB3 are GRID or POINT IDs
// of module 0, with the same relative positions as the PAi: references to the points of the main Bulk Data. Three
// points are required even when the module touches the model at one or two points.
//
// The card can see that each triple is made of three different IDs; the collinearity and the equal shape of the two
// triples need the coordinates, so they are checks of the model. The format reference puts the card in the main Bulk
// Data only, a rule of the deck.
//
// One MDLOC per module: the MODID is unique per card type (IdSpace::PerCardType, category Other).
struct ModuleLocationCard
{
    base::EntityId id = 0;      // the module that is placed
    base::EntityId pointA1 = 0; // the three points in the module (plain IDs)
    base::EntityId pointA2 = 0;
    base::EntityId pointA3 = 0;
    base::EntityId pointB1 = 0; // the three points of module 0 that they are moved onto
    base::EntityId pointB2 = 0;
    base::EntityId pointB3 = 0;
};

} // namespace oor::model
