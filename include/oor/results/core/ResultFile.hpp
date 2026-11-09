#pragma once

#include "oor/base/diagnostics/Diagnostic.hpp"
#include "oor/results/core/ResultFileMetadata.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace oor::results
{

class ResultTables;
struct ResultFilter;

// A file of results once opened: what every format (the HDF5 results archive, a result set made in memory) offers.
// Opening a file reads what it holds, not the values: the values of a table are read when the
// table is first asked for them (see ResultTable).
//
// To support a new format, derive from this class and implement `tables`; nothing else in the library needs to know
// the format.
class ResultFile
{
public:
    virtual ~ResultFile() = default;

    // "archive" or "memory".
    [[nodiscard]] virtual std::string_view format() const noexcept = 0;
    [[nodiscard]] virtual const ResultFileMetadata& metadata() const noexcept = 0;
    // What went wrong while the file was opened (a table that could not be understood, a damaged part), in order.
    [[nodiscard]] virtual const std::vector<base::Diagnostic>& diagnostics() const noexcept = 0;
    // The tables of the file that the filter keeps (see ResultTables::build). Builds the containers only: no value is
    // read until a table is asked for its data.
    [[nodiscard]] virtual ResultTables tables(const ResultFilter& filter) const = 0;

    [[nodiscard]] const std::string& path() const noexcept { return metadata().path; }
};

} // namespace oor::results
