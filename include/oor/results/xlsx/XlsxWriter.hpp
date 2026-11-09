#pragma once

#include "oor/results/core/ResultTables.hpp"
#include "oor/results/writers/ResultWriteReport.hpp"

#include <cstddef>
#include <filesystem>
#include <string>

namespace oor::results
{

// How an Excel workbook of results is written.
struct XlsxWriteOptions
{
    // The texts at the top of the index sheet.
    std::string title;
    std::string subtitle;
    std::string label;
    // The deflate level of the ZIP container, 0 (stored) to 9.
    int compressionLevel = 6;
    // The rows of a sheet, header included, before the table goes on in another sheet: Excel's limit by default. A
    // smaller number splits tables sooner.
    std::size_t maxRowsPerSheet = 1'048'576;
};

// Writes result tables as an Excel workbook (.xlsx, Office Open XML), an output for people (the workbook is not read
// back: the HDF5 archive is the format to exchange results).
//
// - The sheet "Index" lists every table, with a link to its sheet.
// - Each table has a sheet: a few lines that say what it is, then one line per step and row, with the labels of the row
//   (step, ID, grid point, layer, ...) and one column per component (two for a complex value: real and imaginary
//   parts). The header line is frozen and has filters. A table longer than a sheet goes on in the next ones.
// - Each named result (a matrix, a grid point weight, ...) has a sheet with its arrays.
// Empty tables are reported in `skipped`, never dropped silently.
[[nodiscard]] std::string formatXlsx(const ResultTables& tables, const XlsxWriteOptions& options = {},
                                     ResultWriteReport* report = nullptr);

// The same, written to a file (through a temporary file next to it, so a failure never leaves half a file).
ResultWriteReport writeXlsx(const std::filesystem::path& path, const ResultTables& tables,
                            const XlsxWriteOptions& options = {});

} // namespace oor::results
