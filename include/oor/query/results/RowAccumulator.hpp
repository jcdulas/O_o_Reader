#pragma once

#include "oor/query/named/QueryInfo.hpp"
#include "oor/query/results/ResultSets.hpp"
#include "oor/query/table/QueryTable.hpp"
#include "oor/results/core/ResultLayout.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace oor::query
{

// The precision of the values of a result table: single for a 32-bit file, double for a 64-bit one.
enum class Precision : std::uint8_t
{
    Single,
    Double
};

// Rows made from one table, or from one computation, column by column: the part of a result table that a query adds to
// the accumulator. A column is named as in the catalogue of the query; a column the block does not have is null for its
// rows.
class RowBlock
{
public:
    explicit RowBlock(std::size_t rows) : m_rows(rows) {}

    [[nodiscard]] std::size_t rows() const noexcept { return m_rows; }

    // Integer columns (Int32 or Int64 in the catalogue). `valid` has one byte a row (0: null); empty: all there.
    void ints(const std::string& name, std::vector<std::int32_t> values, std::vector<std::uint8_t> valid = {});
    // Real columns (Float64, or a value of a result table): a NaN is a null.
    void reals(const std::string& name, std::vector<double> values);
    // A value that is complex in a complex table: the real and the imaginary part (a null is NaN in both).
    void complex(const std::string& name, std::vector<double> real, std::vector<double> imaginary);
    void texts(const std::string& name, std::vector<std::string> values, std::vector<std::uint8_t> valid = {});
    // A text column by codes into `dictionary` (distinct texts; a negative code is a null): the same column as the
    // texts, without a text a row (COMPOSITE-API-AUDIT P2).
    void codedTexts(const std::string& name, std::vector<std::string> dictionary, std::vector<std::int32_t> codes);
    void flags(const std::string& name, std::vector<std::uint8_t> values, std::vector<std::uint8_t> valid = {});
    // A list of IDs, or of texts, for each row (an empty list is a list, not a null).
    void intLists(const std::string& name, std::vector<std::vector<std::int32_t>> values);
    void textLists(const std::string& name, std::vector<std::vector<std::string>> values);

    // Gives a column another name (the column the table calls `heatFlow` is `power` in the query).
    void rename(const std::string& from, const std::string& to);

    // The step of each row: its kind and its value (the mode number, the time, the frequency or the load factor), with
    // the eigenvalue of each row for modes (NaN when there is none).
    void steps(results::StepKind kind, std::vector<double> values, std::vector<double> eigenvalues = {});

    // The result set the rows come from (its label is the `analysis` column) and their design cycle (-1: none).
    void origin(std::size_t set, std::string analysis, std::int32_t designCycle);
    // The values are in single precision (the default is double).
    void precision(Precision value) noexcept { m_precision = value; }
    // The table stored its complex values as magnitude and phase.
    void storedAsMagnitudePhase(bool value) noexcept { m_magnitudePhase = value; }
    // The provider (an index returned by `RowAccumulator::addProvider`) of the rows: one for all of them, or one each.
    void provider(std::int16_t index) { m_providers.assign(m_rows, index); }
    void providers(std::vector<std::int16_t> perRow) { m_providers = std::move(perRow); }
    // The status of the table the rows were read from ("validated", "notValidated", "experimental").
    void validation(std::string status) { m_validation = std::move(status); }
    // A text added to the `source` of each row (what else produced a column of the row: "vonMises computed").
    void sourceSuffix(std::vector<std::string> suffixes) { m_suffixes = std::move(suffixes); }

    struct Column
    {
        enum class Kind : std::uint8_t
        {
            Integer,
            Real,
            Complex,
            Text,
            CodedText,
            Flag,
            IntList,
            TextList
        };
        Kind kind = Kind::Real;
        std::vector<std::int32_t> integers;
        std::vector<double> reals;
        std::vector<double> imaginary;
        std::vector<std::string> texts;
        std::vector<std::int32_t> codes; // CodedText: the code of each row into `texts`, its dictionary
        std::vector<std::uint8_t> flags;
        std::vector<std::vector<std::int32_t>> intLists;
        std::vector<std::vector<std::string>> textLists;
        std::vector<std::uint8_t> valid; // empty: every row has a value
    };

    [[nodiscard]] const std::map<std::string, Column>& columns() const noexcept { return m_columns; }
    [[nodiscard]] const std::vector<std::int16_t>& providerRows() const noexcept { return m_providers; }
    [[nodiscard]] const std::vector<std::string>& suffixes() const noexcept { return m_suffixes; }
    [[nodiscard]] results::StepKind stepKind() const noexcept { return m_stepKind; }
    [[nodiscard]] const std::vector<double>& stepValues() const noexcept { return m_steps; }
    [[nodiscard]] const std::vector<double>& eigenvalues() const noexcept { return m_eigenvalues; }
    [[nodiscard]] std::size_t set() const noexcept { return m_set; }
    [[nodiscard]] const std::string& analysis() const noexcept { return m_analysis; }
    [[nodiscard]] std::int32_t designCycle() const noexcept { return m_designCycle; }
    [[nodiscard]] Precision precision() const noexcept { return m_precision; }
    [[nodiscard]] bool magnitudePhase() const noexcept { return m_magnitudePhase; }
    [[nodiscard]] const std::string& validationStatus() const noexcept { return m_validation; }

private:
    void check(const std::string& name, std::size_t size) const;

    std::size_t m_rows;
    std::map<std::string, Column> m_columns;
    std::vector<std::int16_t> m_providers;
    std::vector<std::string> m_suffixes;
    results::StepKind m_stepKind = results::StepKind::None;
    std::vector<double> m_steps;
    std::vector<double> m_eigenvalues;
    std::size_t m_set = 0;
    std::string m_analysis;
    std::int32_t m_designCycle = -1;
    Precision m_precision = Precision::Double;
    bool m_magnitudePhase = false;
    std::string m_validation;
};

// Gathers the blocks of a result query and makes its table with the columns, the types and the order the catalogue
// gives: `[step]` becomes the columns of the steps the blocks have, a value column that some block holds
// as complex becomes its two columns, `analysis` and `designCycle` come before `subcase` when there are several
// analyses or design cycles, and the `source` column is added on request. A column of the catalogue that no block has
// is null; a column of a block that the catalogue does not list is an error of the query.
class RowAccumulator
{
public:
    // The way a block was made: returns the index to give to `RowBlock::provider`.
    std::int16_t addProvider(std::string name, bool approximate);
    // Records that a provider filled `columns` of `rows` rows without being the provider of a block (a derived quantity
    // recomputed for the rows that did not store it).
    void note(std::int16_t provider, const std::vector<std::string>& columns, std::int64_t rows);

    void add(RowBlock&& block);
    [[nodiscard]] std::size_t rowCount() const noexcept { return m_rows; }
    [[nodiscard]] bool empty() const noexcept { return m_rows == 0; }

    // The table of the form `detail` of the query. `forceSource` adds the `source` column whatever the arguments say.
    [[nodiscard]] std::shared_ptr<const QueryTable> finish(const ResultQueryContext& context, const QueryInfo& info,
                                                           const std::string& detail = {}, bool forceSource = false);

private:
    struct Provider
    {
        std::string name;
        bool approximate = false;
    };
    struct Note
    {
        std::int16_t provider = 0;
        std::vector<std::string> columns;
        std::int64_t rows = 0;
    };

    std::vector<Provider> m_providers;
    std::vector<RowBlock> m_blocks;
    std::vector<Note> m_notes;
    std::size_t m_rows = 0;
};

// The text of a provider for the `source` column: its name, with " (approximate)" for an approximate way.
[[nodiscard]] std::string sourceText(const std::string& name, bool approximate);

} // namespace oor::query
