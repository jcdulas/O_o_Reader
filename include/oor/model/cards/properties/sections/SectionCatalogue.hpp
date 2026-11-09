#pragma once

#include <cstddef>
#include <span>
#include <string_view>

namespace oor::model::sections
{

// One cross-section type of the standard beam library (STANDARD_SECTION_LIBRARY), as PBARL and PBEAML list them.
struct SectionType
{
    std::string_view name;
    std::size_t dimensions; // DIM1 .. DIMn of the section at one station
};

// The standard library, in the order of the format reference list (the format reference names "L" for PBEAML only;
// both cards accept it). The number of dimensions is the one of the section at a station: a PBARL has one set of them,
// a PBEAML one per station.
inline constexpr SectionType STANDARD_SECTIONS[] = {
    {"Rod", 1},       {"Tube", 2},
    {"Tube2", 2},     {"LSection", 4},
    {"ISection", 6},  {"Channel", 4},
    {"TSection", 4},  {"Box", 4},
    {"Bar", 2},       {"Cross", 4},
    {"HSection", 4},  {"TSection1", 4},
    {"ISection1", 4}, {"Channel1", 4},
    {"ZSection", 4},  {"Channel2", 4},
    {"TSection2", 4}, {"Box1", 6},
    {"Hexagon", 3},   {"Hat", 4},
    {"Hat1", 5},      {"DoubleBoxSection", 10},
};

// The name of the standard library, the only library PBARL and PBEAML accept (a blank GROUP is this one); messages
// and comments say "the standard section library".
inline constexpr std::string_view STANDARD_SECTION_LIBRARY = "StandardSectionLibrary";

[[nodiscard]] constexpr std::span<const SectionType> standardSections() noexcept
{
    return std::span<const SectionType>(STANDARD_SECTIONS);
}

// The section of the library with this name (a neutral name: Rod, ISection, ...), or nullptr.
[[nodiscard]] constexpr const SectionType* findSection(std::string_view type) noexcept
{
    for (const SectionType& section : STANDARD_SECTIONS)
    {
        if (section.name == type)
            return &section;
    }
    return nullptr;
}

// The number of dimensions of a section of the library; 0 for a type that is not in it.
[[nodiscard]] constexpr std::size_t dimensionCount(std::string_view type) noexcept
{
    const SectionType* section = findSection(type);
    return section == nullptr ? 0 : section->dimensions;
}

// The geometry of the sections (area, moments of inertia, torsional constant, shear factors, stress recovery points of
// each type) is in the analysis module: analysis::SectionFormulas (include/oor/analysis/mass/SectionFormulas.hpp),
// one function per type of this list.

} // namespace oor::model::sections
