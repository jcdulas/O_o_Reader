#pragma once

#include "oor/api/Deck.hpp"
#include "oor/query/results/ResultSets.hpp"
#include "oor/results/archive/ArchiveWriter.hpp"
#include "oor/results/core/MemoryResults.hpp"
#include "oor/results/core/ResultFile.hpp"
#include "oor/results/core/ResultTables.hpp"
#include "oor/results/xlsx/XlsxWriter.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace oor::api
{

// What a result table is, without its values.
struct TableDescription
{
    std::size_t index = 0;   // the position of the table in `Results::tables()`: the handle to read it with
    std::string kind;        // "displacements", "stress", ...
    std::string variant;     // "" for the results of an analysis, "psd", "rms", ...
    std::string elementType; // "ShellQuad4" for an element table, the variant of a grid point stress table; else empty
    std::string layout;      // "centre" or "corner" for the element types that have both; else empty
    std::int32_t subcase = 0;
    std::string analysis; // "static", "modes", "frequency", "transient", ...
    std::string stepKind; // "none", "loadSet", "mode", "frequency", "time", "loadFactor"
    std::vector<double> steps;
    std::vector<double> eigenvalues;
    std::vector<double> cycles;
    std::size_t rowCount = 0;
    std::vector<std::string> components;
    std::string valueType; // "float32", "float64", "complex64", "complex128"
    std::string tableName;
    std::string title;
    std::string subtitle;
    std::string label;
};

// What to read of a table; an empty member keeps everything.
struct TableSelection
{
    std::vector<std::int64_t> stepIndices; // positions of the steps, from 0
    std::vector<std::string> components;   // in the order wanted
    std::vector<std::int32_t> ids;         // grid points of a nodal table
    std::vector<std::int32_t> elementIds;  // elements of an element table
};

// The values of a table, read: shape (steps, rows, components), with a last dimension of 2 for complex values.
struct TableValues
{
    std::vector<std::size_t> shape;
    std::string valueType;
    std::vector<std::string> components;
    std::vector<double> steps;
    std::vector<std::int32_t> ids;
    std::vector<std::int32_t> nodeIds;    // element tables: the grid point of the row, 0 at the centre
    std::vector<std::int32_t> layers;     // element tables: the fibre, ply or station of the row, from 1
    std::vector<std::int32_t> elementIds; // grid point forces: the element of the row
    // Text labels of the rows, by name ("elementName"), one text a row.
    std::vector<std::pair<std::string, std::vector<std::string>>> texts;
    // The values (it keeps them alive): read them with `values<float>()` or `values<double>()`.
    std::shared_ptr<const results::ResultData> data;
};

// A result set: an HDF5 results archive opened, or tables made in memory by a program.
class Results
{
public:
    // Opens an archive by content. Model and result values are loaded only when requested.
    [[nodiscard]] static std::shared_ptr<Results> open(const std::string& path);
    // A result set of tables made in memory (a solver hands over its results): each input is checked against the result
    // catalogue (makeMemoryTable). `label` names the set. Throws UsageError with every problem of an input.
    [[nodiscard]] static std::shared_ptr<Results> fromTables(const std::string& label,
                                                             std::vector<results::MemoryTableInput> inputs);

    [[nodiscard]] std::string format() const;
    [[nodiscard]] const results::ResultFileMetadata& metadata() const;
    [[nodiscard]] std::vector<DiagnosticText> diagnostics() const;

    [[nodiscard]] std::vector<TableDescription> tables() const;
    // Reads a table (by its index in `tables()`). Throws UsageError for an index out of range, EmptySelectionError when
    // the selection keeps nothing.
    [[nodiscard]] TableValues read(std::size_t index, const TableSelection& selection = {}) const;
    [[nodiscard]] bool hasModel() const;
    // The attached or embedded model, or nullptr when absent. Loading also attaches it to the results for queries.
    [[nodiscard]] std::shared_ptr<Deck> model();

    // Links the results to the cards of a deck: what the queries that combine both need. Throws DeckError when the
    // results name cards the deck does not have and `strict` is set.
    void attach(const std::shared_ptr<Deck>& deck, bool strict = false);

    results::ResultWriteReport writeArchive(const std::string& path,
                                            const results::ArchiveWriteOptions& options = {}) const;
    // An Excel workbook of the tables, for people (results/xlsx/XlsxWriter.hpp).
    results::ResultWriteReport writeXlsx(const std::string& path, const results::XlsxWriteOptions& options = {}) const;

    // The result set the query engine reads.
    [[nodiscard]] query::ResultSet resultSet() const;

private:
    Results() = default;

    std::shared_ptr<results::ResultFile> m_file;
    std::shared_ptr<results::ResultTables> m_tables;
    std::vector<std::shared_ptr<results::ResultTable>> m_list; // the tables in a stable order: the handles
    std::shared_ptr<results::ResultAttachment> m_attachment;
    std::shared_ptr<Deck> m_deck;
};

} // namespace oor::api
