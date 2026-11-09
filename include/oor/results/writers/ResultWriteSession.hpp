#pragma once

#include "oor/results/core/NamedResult.hpp"
#include "oor/results/core/ResultTable.hpp"
#include "oor/results/core/ResultTables.hpp"
#include "oor/results/writers/ResultWriteReport.hpp"

#include <memory>

namespace oor::results
{

// Writing a result file by batches: `begin` (the session is made, the header and the geometry
// written), `push` a batch of load cases or of steps as many times as needed, `finish` (the file is complete and in
// place; the report says what was written and what was left out). A session holds none of the values pushed: the
// tables are encoded at once and kept on disk until `finish`. It keeps a small record of each table written (its line
// of the report and its steps, for the checks below; for HDF5 its domain): about a kilobyte a load case.
//
// A push carries whole steps: the steps of a (kind, subcase, analysis) pushed twice, or rows that change from one push
// to the next for the same table, raise UsageError. Nothing is in place at the target until `finish`; `abort` (or the
// destruction of an unfinished session) removes everything written.
class ResultWriteSession
{
public:
    virtual ~ResultWriteSession() = default;

    // Every table and named result of a set.
    virtual void push(const ResultTables& tables) = 0;
    virtual void push(const std::shared_ptr<ResultTable>& table) = 0;
    virtual void pushNamed(const std::shared_ptr<NamedResult>& result) = 0;
    // Writes the file and returns what was done (the report of the session, not a copy). A second call raises
    // UsageError.
    virtual const ResultWriteReport& finish() = 0;
    virtual void abort() noexcept = 0;
    [[nodiscard]] virtual bool finished() const noexcept = 0;
    // What was done so far (the tables pushed).
    [[nodiscard]] virtual const ResultWriteReport& report() const noexcept = 0;
};

} // namespace oor::results
