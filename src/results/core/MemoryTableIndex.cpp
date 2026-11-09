#include "oor/results/core/MemoryTableIndex.hpp"

namespace oor::results
{

MemoryTableKey MemoryTableIndex::keyOf(const ResultTableMeta& meta)
{
    MemoryTableKey key;
    key.kind = meta.kind;
    key.variant = meta.variant;
    key.container = meta.elementName.empty() ? meta.subName : meta.elementName;
    key.layout = meta.layout;
    key.subcase = meta.subcase;
    key.analysisCode = meta.analysisCode;
    key.designCycle = meta.designCycle;
    key.superelement = meta.superelement;
    key.loadSet = meta.loadSet;
    key.tableName = meta.tableName;
    return key;
}

std::string MemoryTableIndex::describe(const MemoryTableKey& key)
{
    std::string text(resultKindName(key.kind));
    if (key.variant != ResultVariant::Plain)
        text = std::string(resultVariantName(key.variant)) + " " + text;
    if (!key.container.empty())
        text += " of " + key.container;
    if (!key.layout.empty())
        text += " (" + key.layout + " layout)";
    text += ", subcase " + std::to_string(key.subcase) + ", " + std::string(analysisTypeName(key.analysisCode));
    if (key.loadSet != 0)
        text += ", load set " + std::to_string(key.loadSet);
    if (key.designCycle >= 0)
        text += ", design cycle " + std::to_string(key.designCycle);
    if (key.superelement >= 0)
        text += ", superelement " + std::to_string(key.superelement);
    if (!key.tableName.empty())
        text += ", table " + key.tableName;
    return text;
}

} // namespace oor::results
