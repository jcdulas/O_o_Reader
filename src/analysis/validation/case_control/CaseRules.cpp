#include "CaseRules.hpp"

#include "oor/model/control/case_control/CommandCatalogue.hpp"

namespace oor::analysis
{

std::vector<const model::CaseEntry*> commandsOutsideBlocks(const model::Subcase& section)
{
    std::vector<const model::CaseEntry*> commands;
    for (std::size_t index = 0; index < section.entries.size(); ++index)
    {
        const model::CaseEntry& entry = section.entries[index];
        if (entry.kind == model::EntryKind::OutputBlock)
        {
            const model::CaseControl::Extent block = model::CaseControl::blockExtent(section, index);
            if (block.last > index + 1)
                index = block.last - 1; // the loop goes on with the entry after the block
            continue;
        }
        if (entry.kind != model::EntryKind::Command && entry.kind != model::EntryKind::Output)
            continue;
        // The plot commands (PLOT, PTITLE, SEPLOT, ...) repeat on purpose, one per plot.
        const model::CommandDefinition* definition = model::CommandCatalogue::instance().findByName(entry.name);
        if (definition != nullptr && definition->isBlockCommand)
            continue;
        commands.push_back(&entry);
    }
    return commands;
}

std::string commandIdentity(const model::CaseEntry& entry)
{
    std::string identity = entry.name + "/" + entry.variant;
    for (const std::string& option : entry.options)
        identity += "," + option;
    return identity;
}

std::string commandLabel(std::string_view name, std::string_view variant)
{
    std::string label = std::string(name);
    if (!variant.empty())
        label += "(" + std::string(variant) + ")";
    return label;
}

bool selectsSet(const std::string& name)
{
    return targetOfRequest(name) != RequestTarget::None ||
           isOneOf(name,
                   {"SolutionSetDisplacementOutput", "SolutionSetVelocityOutput", "SolutionSetAccelOutput",
                    "TotalDisplacementOutput", "TotalVelocityOutput", "TotalAccelOutput", "NonlinearStressOutput",
                    "NodeStressOutput", "NodeStrainOutput", "NodeKineticEnergyOutput", "ThermalOutput",
                    "HeatFluxOutput", "NonlinearLoadOutput", "ElemSummaryOutput", "SolutionSetEigenvectorOutput"});
}

RequestTarget targetOfRequest(const std::string& name)
{
    if (isOneOf(name, {"DisplacementOutput", "VelocityOutput", "AccelOutput", "SinglePointConstrForceOutput",
                       "MultiPointConstrForceOutput", "AppliedLoadOutput", "NodeForceOutput"}))
        return RequestTarget::Nodes;
    if (isOneOf(name, {"StressOutput", "StrainOutput", "ElemForceOutput", "ElemStrainEnergyOutput",
                       "ElemKineticEnergyOutput", "ElemDampingEnergyOutput"}))
        return RequestTarget::Elements;
    return RequestTarget::None;
}

} // namespace oor::analysis
