#pragma once

namespace oor::model
{

// Opens an opaque group of records for an external explicit solver. The model retains the records without interpreting their fields.
struct ExplicitTextStartCard
{
    [[nodiscard]] bool operator==(const ExplicitTextStartCard&) const noexcept = default;
};

} // namespace oor::model
