#pragma once

#include "oor/base/errors/OorError.hpp"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace oor::query
{

// A query that cannot run as written: an unknown column, an expression that does not parse, a join that has no
// reference to follow, a key that is not unique. Python sees `QueryError`, which is also a `ValueError`.
class QueryError : public base::OorError
{
public:
    using OorError::OorError;
};

// A quantity that no way of obtaining it could give: the message lists every
// way that was tried and why it failed. Python sees `QuantityUnavailableError`, a `QueryError`.
class QuantityUnavailableError : public QueryError
{
public:
    using QueryError::QueryError;
};

// A column used with a type it does not have: text added to a number, a sum of text, a list in a comparison. Python
// sees `QueryTypeError`, which is also a `TypeError`.
class QueryTypeError : public base::OorError
{
public:
    using OorError::OorError;
};

// An edit of a column that cannot be edited: a computed quantity, a joined or derived column, the key ID of a card, a
// field with no setter. Raised when the edit is made, never at `apply`. Python sees `ReadOnlyColumnError`, which is
// both a `TypeError` (a column does not take item assignment) and an `AttributeError` (a record attribute that is read
// only).
class ReadOnlyColumnError : public base::OorError
{
public:
    using OorError::OorError;
};

// One reason why an edit could not be written to the model: the card (its type, ID and card key), the field, and what
// went wrong.
struct ApplyProblem
{
    std::string card;      // card type name, "ShellQuad4"
    std::int64_t id = 0;   // the card ID, 0 when the card is gone
    std::uint32_t key = 0; // the card key
    std::string field;     // the column that was being written, empty for a problem of the card as a whole
    std::string message;
};

// `apply` refused to write: nothing was written. `problems` lists every reason found (the cards deleted since the
// query, the cells a card refused). `isStale` says that at least one card of the table no longer exists; Python then
// raises `StaleApplyError`, which is also a `StaleHandleError`.
class ApplyError : public base::OorError
{
public:
    ApplyError(const std::string& message, std::vector<ApplyProblem> problems, bool stale)
        : OorError(message), m_problems(std::move(problems)), m_stale(stale)
    {
    }
    [[nodiscard]] const std::vector<ApplyProblem>& problems() const noexcept { return m_problems; }
    [[nodiscard]] bool isStale() const noexcept { return m_stale; }

private:
    std::vector<ApplyProblem> m_problems;
    bool m_stale = false;
};

} // namespace oor::query
