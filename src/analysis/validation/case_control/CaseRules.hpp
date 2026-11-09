#pragma once

#include "oor/analysis/validation/engine/ValidationSupport.hpp"

#include <string>
#include <string_view>

namespace oor::analysis
{

// Calls fn(section) for the global section, every subcase in file order, and the steps inside them.
template <typename Fn>
void forEachSection(const model::Subcase& section, Fn&& fn)
{
    fn(section);
    for (const model::Subcase& child : section.children)
        forEachSection(child, fn);
}

template <typename Fn>
void forEachSection(const model::CaseControl& caseControl, Fn&& fn)
{
    for (const model::Subcase& segment : caseControl.segments())
        forEachSection(segment, fn);
}

// The statement and number findings about a section are filed under: the name of its kind ("subcase", "global";
// FindingSink::reportStatement gives the finding the keyword of the dictionary of the model, SUBCASE) and its number.
[[nodiscard]] inline std::string statementOf(const model::Subcase& section)
{
    return std::string(model::subcaseKindName(section.kind));
}

// A command in a message: its keyword, with the keyword of its variant in parentheses ("TEMPERATURE(MATERIAL)").
[[nodiscard]] std::string commandLabel(std::string_view name, std::string_view variant = {});

// The entries of a section that are commands outside the plot blocks (the commands of an OUTPUT(PLOT) block repeat on
// purpose), in file order.
[[nodiscard]] std::vector<const model::CaseEntry*> commandsOutsideBlocks(const model::Subcase& section);

// What makes two commands the same command in a section: the name and variant, and for an output request its options
// (DISPLACEMENT(PLOT) and DISPLACEMENT(PRINT) are two requests).
[[nodiscard]] std::string commandIdentity(const model::CaseEntry& entry);

// The Case Control output requests (neutral names) that name a set of nodes or of elements: DisplacementOutput
// (nodes), StressOutput (elements).
enum class RequestTarget
{
    None,
    Nodes,
    Elements
};
[[nodiscard]] RequestTarget targetOfRequest(const std::string& name);

// True for the output requests whose number selects a SET of the Case Control. The number of the others is the ID of a
// Bulk Data card (RCROSS = 20 selects an RCROSS card), or of something else.
[[nodiscard]] bool selectsSet(const std::string& name);

} // namespace oor::analysis
