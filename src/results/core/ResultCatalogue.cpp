#include "oor/results/core/ResultCatalogue.hpp"

#include "ResultCatalogueData.hpp"

#include <algorithm>
#include <map>
#include <sstream>
#include <tuple>
#include <utility>

namespace oor::results
{

namespace
{

std::vector<std::string> splitWords(const char* text)
{
    std::vector<std::string> words;
    std::istringstream stream(text);
    std::string word;
    while (stream >> word)
        words.push_back(word);
    return words;
}

std::vector<std::vector<std::string>> splitLists(const char* text)
{
    std::vector<std::vector<std::string>> lists;
    std::string remaining(text);
    std::size_t start = 0;
    while (start < remaining.size())
    {
        std::size_t end = remaining.find(';', start);
        if (end == std::string::npos)
            end = remaining.size();
        std::vector<std::string> list = splitWords(remaining.substr(start, end - start).c_str());
        if (!list.empty())
            lists.push_back(std::move(list));
        start = end + 1;
    }
    return lists;
}

// Everything the catalogue answers, computed once from the raw rows.
struct Catalogue
{
    std::vector<TableLayout> layouts;
    std::map<ResultKind, std::vector<std::string>> elementNames;
    // (kind, element type, complex) -> index in `layouts`
    std::map<std::tuple<ResultKind, std::string, bool>, std::size_t> byKey;
    // (kind, element type, complex) -> default list followed by the alternatives
    std::map<std::tuple<ResultKind, std::string, bool>, std::vector<std::vector<std::string>>> lists;
};

const Catalogue& theCatalogue()
{
    static const Catalogue CATALOGUE = []
    {
        Catalogue built;
        for (const catalogue::LayoutRow& row : catalogue::layoutRows())
        {
            TableLayout layout;
            layout.kind = row.kind;
            layout.elementType = row.elementType;
            layout.complex = row.complex;
            layout.components = splitWords(row.components);
            layout.alternatives = splitLists(row.alternatives);
            layout.rowTexts = splitWords(row.rowTexts);
            built.layouts.push_back(std::move(layout));
        }
        for (std::size_t index = 0; index < built.layouts.size(); ++index)
        {
            const TableLayout& layout = built.layouts[index];
            built.byKey[{layout.kind, layout.elementType, layout.complex}] = index;
            if (!layout.elementType.empty())
            {
                std::vector<std::string>& names = built.elementNames[layout.kind];
                if (std::find(names.begin(), names.end(), layout.elementType) == names.end())
                    names.push_back(layout.elementType);
            }
            std::vector<std::vector<std::string>> lists = {layout.components};
            lists.insert(lists.end(), layout.alternatives.begin(), layout.alternatives.end());
            built.lists[{layout.kind, layout.elementType, layout.complex}] = std::move(lists);
        }
        for (auto& [kind, names] : built.elementNames)
            std::sort(names.begin(), names.end());
        return built;
    }();
    return CATALOGUE;
}

// The layout of a kind and element type in the form asked for, else in the other form; null when there is none.
const TableLayout* findLayout(ResultKind kind, std::string_view elementName, bool complex)
{
    const Catalogue& all = theCatalogue();
    for (const bool form : {complex, !complex})
    {
        const auto found = all.byKey.find({kind, std::string(elementName), form});
        if (found != all.byKey.end())
            return &all.layouts[found->second];
    }
    return nullptr;
}

} // namespace

const std::vector<TableLayout>& allTableLayouts()
{
    return theCatalogue().layouts;
}

const std::vector<std::string>& catalogueElementNames(ResultKind kind)
{
    static const std::vector<std::string> NONE;
    const auto& names = theCatalogue().elementNames;
    const auto found = names.find(kind);
    return found == names.end() ? NONE : found->second;
}

const std::vector<std::string>& catalogueComponents(ResultKind kind, std::string_view elementName, bool complex)
{
    static const std::vector<std::string> NONE;
    const TableLayout* layout = findLayout(kind, elementName, complex);
    return layout == nullptr ? NONE : layout->components;
}

const std::vector<std::vector<std::string>>& catalogueComponentLists(ResultKind kind, std::string_view elementName,
                                                                     bool complex)
{
    static const std::vector<std::vector<std::string>> NONE;
    const TableLayout* layout = findLayout(kind, elementName, complex);
    if (layout == nullptr)
        return NONE;
    return theCatalogue().lists.at({layout->kind, layout->elementType, layout->complex});
}

std::string_view elementCardOf(std::string_view elementName) noexcept
{
    for (const catalogue::CardRow& row : catalogue::cardRows())
        if (elementName == row.elementType)
            return row.card;
    return {};
}

bool hasCentreAndCornerLayouts(std::string_view elementName) noexcept
{
    return elementName == "ShellQuad4" || elementName == "ShellQuad4Drilling" || elementName == "ShellTria3Drilling";
}

} // namespace oor::results
