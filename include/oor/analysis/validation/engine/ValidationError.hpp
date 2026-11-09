#pragma once

#include "oor/analysis/validation/engine/ValidationFinding.hpp"
#include "oor/base/errors/OorError.hpp"

#include <string>
#include <utility>

namespace oor::analysis
{

// What `validate(raise_on_error=True)` throws when the validation found an error: carries every finding of the run (the
// ones below the severity floor excluded), not only the errors.
class ValidationError : public base::OorError
{
public:
    explicit ValidationError(ValidationReport report) : OorError(describe(report)), m_report(std::move(report)) {}

    [[nodiscard]] const ValidationReport& report() const noexcept { return m_report; }

private:
    // "validation found 3 errors; the first: error[ref.unresolved] CQUAD4 10, field pid: ..."
    [[nodiscard]] static std::string describe(const ValidationReport& report);

    ValidationReport m_report;
};

} // namespace oor::analysis
