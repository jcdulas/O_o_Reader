#pragma once

#include "oor/analysis/validation/engine/CheckRegistry.hpp"
#include "oor/analysis/validation/engine/ValidationContext.hpp"
#include "oor/analysis/validation/engine/ValidationFinding.hpp"
#include "oor/base/threading/ProgressCounter.hpp"
#include "oor/base/threading/ThreadPool.hpp"
#include "oor/model/core/Model.hpp"

#include <vector>

namespace oor::analysis
{

// Runs the checks chosen by the options on a model and on its parts.
//
// The work is cut into one task per (model, family): the families are independent, so the tasks run at the same time on
// the pool, and the checks of a family one after the other in the order they were registered. The findings are put
// together after the tasks end and sorted (errors first, then residual structure before parts, family order, card key,
// check order, the order the check reported them in), so the report is the same for any number of threads.
//
// The model is read, never changed. A check that throws a library error (a coordinate system that cannot be resolved)
// does not stop the run: the failure is reported as a finding of that check's code, as an error, "the check could not
// run: ...". OperationCancelled and any other exception end the run.
class ValidationEngine
{
public:
    // The checks the options choose, in running order. Throws UsageError for a name in `select` or `ignore` that is
    // neither a check, a family nor a prefix.
    [[nodiscard]] static std::vector<const ValidationCheck*> select(const ValidationOptions& options);

    // The severity floor to use when the caller gives none: warnings and errors, lowered to the lowest severity of the
    // checks `select` names by their exact code (someone who asks for an information check wants to see it).
    [[nodiscard]] static base::Severity defaultMinimum(const std::vector<std::string>& select);

    // `pool` (null: the calling thread) runs the tasks; `progress` (optional) counts them.
    [[nodiscard]] static ValidationReport run(const model::Model& model, const ValidationOptions& options,
                                              base::ThreadPool* pool = nullptr,
                                              base::ProgressCounter* progress = nullptr);

    // Throws ValidationError when the report has an error.
    static void raiseIfErrors(const ValidationReport& report);
};

} // namespace oor::analysis
