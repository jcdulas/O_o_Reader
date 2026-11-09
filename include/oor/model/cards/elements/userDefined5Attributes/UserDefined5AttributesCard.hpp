#pragma once

#include <cstdint>
#include <string>

namespace oor::model
{

// One ADUM5 card: the attributes of the user dummy element number 5, whose connection is a CDUM5 card and whose
// property is a PDUM5 card. The element routines that give the dummy element its stiffness and its output are written
// by the user; the ADUM5 card tells the solver how many fields the two cards have.
//
// Card layout: ADUM5  NG  NC  NP  ND  ELNM            fields 1 to 5
//
//   NG    the most grid points the element connects: a positive integer
//   NC    how many additional fields Ai the CDUM5 card has: zero or more
//   NP    how many additional fields Aj the PDUM5 card has: 0 to 24
//   ND    the displacement components per grid point of the differential stiffness matrix: 3 or 6, and 0 for an element
//         with no differential stiffness
//   ELNM  the name of the connection and of the property card; a common deck names the connection CRAC2D and says the
//         property is PRAC2D (the same name with a P in place of the C). A name is one to eight characters, or
//         blank (the cards keep the names CDUM5 and PDUM5).
//
// The format reference of the ADUMi entry says the number i runs from 3 to 9 and those of CDUMi and PDUMi from 3
// to 7; the numbered dummy elements of the format are DUM1 to DUM9, so the library reads the nine numbers 1 to 9 for
// the three entries. The format adds that ADUM8 and ADUM9 are used only by the CRAC2D and CRAC3D elements.
//
// The format gives no default for NC, NP and ND: a blank one is read as 0 (a value every one of them accepts) and the
// three are always written. ELNM is a card name, so it is kept in upper case. The card has no ID (IdSpace::None) and
// names no other card: ELNM is a word, not a reference. The format reference does not say a deck may have only one
// ADUM5 card, so the library keeps every one it reads and leaves the question of two of one number to the checks
// between cards.
struct UserDefined5AttributesCard
{
    std::int32_t nodeCount = 0;                  // NG
    std::int32_t connectionFieldCount = 0;       // NC
    std::int32_t propertyFieldCount = 0;         // NP
    std::int32_t displacementComponentCount = 0; // ND
    std::string elementName;                     // ELNM: the neutral name of the element card
};

} // namespace oor::model
