#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <utility>
#include <vector>

namespace oor::model
{

// One MDBCPAR card: the modules that are candidates for the automatic detection of contact across modules (Case Control
// BCONTACT=AUTO in a deck that has modules). The format reference files it among the node entries; it is a contact
// card. The distance within which two bodies of different modules touch is the DISTANCE of the MDBCATP.
//
// Card layout: MDBCPAR  (blank)  SET3ID                                         field 2
//             (line 2)  MODID11  MODID21  (blank)  (blank)  MODID12  MODID22     fields 9, 10, 13 and 14
//             (line 3)  MODID13  MODID23  (blank)  (blank)  MODID1i  MODID2i     fields 17, 18, 21 and 22, and so on
// SET3ID is in the second field (the first is a blank cell of the layout) and defaults to
// -1: every module takes part. With 0 and no pair no contact is detected across modules, and with a positive number the
// modules are those of the SET3 of that number whose DES is MODULE. Each pair (MODID1i, MODID2i) names two
// modules whose bodies are tested against each other. The pairs are optional and may follow any SET3ID. The module
// numbers are the numbers of the BEGIN MODULE sections of the deck (parts of the deck, which no card defines): they are
// not references; the SET3 is one. The fields of a pair are read in order whatever line they are on, so a deck that
// packs the pairs from field 3 on reads too; the card writes them as the format draws them. Only one MDBCPAR is
// allowed, in module 0.
struct ModuleContactCandidatesCard
{
    std::int32_t moduleSetId = -1;                                // -1: all the modules, 0: none, above: a SET3
    std::vector<std::pair<base::EntityId, base::EntityId>> pairs; // (MODID1i, MODID2i)
};

} // namespace oor::model
