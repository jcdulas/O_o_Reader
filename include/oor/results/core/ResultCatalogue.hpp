#pragma once

#include "oor/results/core/ResultKind.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace oor::results
{

// The catalogue of result layouts: for every kind of result (and, for the element kinds, every element type), the
// components a table holds and in which order. It is the single place that says "a CQUAD4 stress table has the columns
// fiberDistance, oxx, oyy, ...". Every reader, every writer (archive, workbook) and the tables made in memory use it,
// so a table has the same columns whatever file it came from.
//
// The data live in ResultCatalogueData.cpp: one line a layout, easy to read and to extend by hand.

// One layout: a kind, an element type (or variant), real or complex values, and the components.
struct TableLayout
{
    ResultKind kind = ResultKind::Displacements;
    // The element type ("ShellQuad4") or the variant of a grid point stress table ("surface"); empty for the other
    // kinds.
    std::string elementType;
    bool complex = false;
    // The default components, in order.
    std::vector<std::string> components;
    // Other component lists a table of this layout may have (a shell stress with maxShear in place of vonMises, a
    // shell strain with fiberDistance in place of fiberCurvature, ...).
    std::vector<std::vector<std::string>> alternatives;
    // The text labels of a row: "elementName" for a grid point force, "fiber" for a surface grid point stress, ...
    std::vector<std::string> rowTexts;
};

// Every layout, in the order of the kinds, then of the element types, real before complex.
[[nodiscard]] const std::vector<TableLayout>& allTableLayouts();

// The element types (or variants) a family kind has, in alphabetical order; empty for a kind that is not a family.
[[nodiscard]] const std::vector<std::string>& catalogueElementNames(ResultKind kind);

// The default components of a table of a kind and element type, real or complex. When the catalogue has no complex
// layout for it, the real one is given (and the other way round). Empty for an element type the kind does not have.
[[nodiscard]] const std::vector<std::string>& catalogueComponents(ResultKind kind, std::string_view elementName,
                                                                  bool complex);

// Every component list a table of the kind and element type may have: the default list first, then the alternatives.
[[nodiscard]] const std::vector<std::vector<std::string>>& catalogueComponentLists(ResultKind kind,
                                                                                   std::string_view elementName,
                                                                                   bool complex);

// The card type of an element type of the results: "ShellQuad4Composite" -> "ShellQuad4", "Bar2LoadStations" ->
// "Bar2". Empty when unknown.
[[nodiscard]] std::string_view elementCardOf(std::string_view elementName) noexcept;

// The element types whose tables come either at the centre only or at the centre and the corners.
[[nodiscard]] bool hasCentreAndCornerLayouts(std::string_view elementName) noexcept;

} // namespace oor::results
