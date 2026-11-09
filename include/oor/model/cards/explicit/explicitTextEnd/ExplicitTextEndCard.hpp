#pragma once

namespace oor::model
{

// Closes the opaque group opened by ExplicitTextStart.
struct ExplicitTextEndCard
{
    [[nodiscard]] bool operator==(const ExplicitTextEndCard&) const noexcept = default;
};

} // namespace oor::model
