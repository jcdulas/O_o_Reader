#pragma once

#include <string>
#include <vector>

namespace oor::model
{

// One AECOMPL card: a component that is the union of other components.
//
// Card layout: AECOMPL  NAME  LABEL1  LABEL2  LABEL3  LABEL4  LABEL5  LABEL6  LABEL7
//                      LABEL8  -etc.-
// The labels run from field 2 to the last field used, with no blank between two of them; the first line holds seven,
// the rest are on the continuation lines and the format reference draws no limit. At least one label is given. Each
// label is the NAME of an AECOMP or of another AECOMPL. The names are not references by number: a check between cards
// compares them, without regard to the case, with the names of the components of the model, and keeps the structural
// and the aerodynamic components apart. The NAME of the card is its key, unique among all the AECOMP and
// AECOMPL cards (also a check between cards). Names and labels are kept as written.
struct AeroComponentUnionCard
{
    std::string name;                // up to eight characters
    std::vector<std::string> labels; // the names of the components that are joined

    [[nodiscard]] bool operator==(const AeroComponentUnionCard&) const noexcept = default;
};

} // namespace oor::model
