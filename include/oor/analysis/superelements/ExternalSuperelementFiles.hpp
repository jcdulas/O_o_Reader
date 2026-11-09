#pragma once

#include "oor/analysis/superelements/SuperelementCatalogue.hpp"

#include <optional>
#include <string>

namespace oor::analysis
{

// The file an external superelement's matrices are read from (the `assigned_file` column of `parts_table`): for the
// SEBULK TYPE of the external binary results, of the external matrix file and EXTROM the file of the File Management
// ASSIGN statement whose UNIT is the SEBULK UNITNO (ASSIGN INPUTT4='se.mat' UNIT=31 for UNITNO 31), as the statement
// writes it. EXTERNAL reads a database that DBLOCATE
// or the ASSIGN of a DBset names, and EXTPCH the DMIG entries of its own part: neither has a unit, so neither has a
// file here.
class ExternalSuperelementFiles
{
public:
    // The file, or nullopt (not an external superelement of a unit, or no ASSIGN of that unit).
    [[nodiscard]] static std::optional<std::string> assignedFile(const model::Model& model,
                                                                 const SuperelementEntry& entry);
};

} // namespace oor::analysis
