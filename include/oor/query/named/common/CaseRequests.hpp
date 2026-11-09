#pragma once

#include "oor/model/control/case_control/CaseControl.hpp"

#include <span>
#include <string>
#include <vector>

namespace oor::query
{

// One output request that applies to a subcase.
struct OutputRequest
{
    const model::CaseEntry* entry = nullptr;
    bool isInherited = false; // given by a section the subcase inherits from (the global section), not by itself
};

// The output requests that apply to a subcase: for each request name, the requests of the nearest section that has
// requests of that name (a subcase that has any DISPLACEMENT request hides every DISPLACEMENT request of the global
// section, CaseControl). The names come in the order they are first met from the subcase up to the global section; the
// requests of one name in the order of their section.
[[nodiscard]] std::vector<OutputRequest> effectiveRequests(const model::CaseControl& caseControl,
                                                           const model::Subcase& subcase);
// The same along the lineage of the subcase (model::CaseLineages::of, for a loop over the subcases).
[[nodiscard]] std::vector<OutputRequest> effectiveRequests(std::span<const model::Subcase* const> lineage);

// A request as text with the names and values the model holds: DisplacementOutput(SortByPoint,RealPart) = All.
[[nodiscard]] std::string requestText(const model::CaseEntry& entry);

// The describers of a request by their neutral names, in the order of the command: an option the dictionary names
// (SORT1: SortByPoint), or written as a shortened form of one of the describers of the output requests (at least four
// letters that begin one describer and no other: VONMI for VONMISES), is given by the neutral name of the describer;
// any other option (GRID=12 of a MEFFMASS, a THRESH value) is kept as written.
[[nodiscard]] std::vector<std::string> fullDescribers(const model::CaseEntry& entry);

} // namespace oor::query
