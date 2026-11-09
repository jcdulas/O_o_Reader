#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// One term of a CLOAD card: the scale factor Si and the number IDVi of the load vector it applies to.
struct SuperelementLoadCombinationTerm
{
    double scale = 0.0;        // Si
    base::EntityId loadId = 0; // IDVi: the EXCITEID of an LSEQ card

    [[nodiscard]] bool operator==(const SuperelementLoadCombinationTerm&) const noexcept = default;
};

// One CLOAD card: a static load that combines the load vectors the LSEQ cards define for a superelement (non-linear
// static analysis, SOL 106 and 153). The Case Control CLOAD = CID selects it in the residual subcases.
//
// Card layout: CLOAD  CID  S  S1  IDV1  S2  IDV2  S3  IDV3
//                    S4   IDV4  -etc.-
// The load is S times the sum of Si times the vector IDVi. The pairs follow each other from field 3, over as many
// continuation lines as needed; at least one is given. An IDVi is the EXCITEID of an LSEQ card selected by LOADSET,
// the way a dynamic load names its excitation. A CLOAD may name no ID that another CLOAD
// defines: a rule between cards, not checked here. The card is filed like LOAD, in the category Load and
// among the sets of loads by its CID (IdSpace::Group); the format reference shows it among the aeroelastic cards only
// because of where it sits in the book.
struct SuperelementLoadCombinationCard
{
    base::EntityId id = 0;
    double scaleFactor = 0.0; // the overall scale factor
    std::vector<SuperelementLoadCombinationTerm> terms;
};

} // namespace oor::model
