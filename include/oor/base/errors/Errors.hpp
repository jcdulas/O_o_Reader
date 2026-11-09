#pragma once

#include "oor/base/diagnostics/Diagnostic.hpp"
#include "oor/base/errors/OorError.hpp"

#include <string>
#include <utility>

namespace oor::base
{

// The exceptions of the public API: small types that only make sense together, hence one header. Each maps to a
// Python exception of the same name (python/bindings/base/ErrorsPython.cpp).

// A file content error that strict mode turns into an exception; carries the diagnostic.
class DeckError : public OorError
{
public:
    explicit DeckError(Diagnostic diagnostic) : OorError(diagnostic.toString()), m_diagnostic(std::move(diagnostic)) {}
    [[nodiscard]] const Diagnostic& diagnostic() const noexcept { return m_diagnostic; }

private:
    Diagnostic m_diagnostic;
};

// A file could not be opened, read or written.
class IoError : public OorError
{
public:
    using OorError::OorError;
};

// A handle whose card was deleted.
class StaleHandleError : public OorError
{
public:
    using OorError::OorError;
};

// A reference to a card that does not exist.
class UnresolvedReferenceError : public OorError
{
public:
    using OorError::OorError;
};

class DuplicateIdError : public OorError
{
public:
    using OorError::OorError;
};

// A frame change that is not validated (see the frame convention table).
class UnsupportedFrameError : public OorError
{
public:
    using OorError::OorError;
};

// A result set attached to a model whose structure changed since (elements or nodes added, removed, renumbered, a
// compaction): its links to the model are no longer known to be right, and operations that need the model (frame
// changes) refuse until `refresh()` or a new attach. Also an attachment whose model no longer exists.
class StaleAttachmentError : public OorError
{
public:
    using OorError::OorError;
};

// A result filter that selects nothing.
class EmptySelectionError : public OorError
{
public:
    using OorError::OorError;
};

// The operation was cancelled (Ctrl-C or a progress callback returning false). Nothing is left half done.
class OperationCancelled : public OorError
{
public:
    OperationCancelled() : OorError("operation cancelled") {}
};

// An input exceeds a resource bound (size, count, nesting) set to keep hostile files harmless.
class LimitExceededError : public OorError
{
public:
    using OorError::OorError;
};

// A programming error: an argument the API cannot accept.
class UsageError : public OorError
{
public:
    using OorError::OorError;
};

} // namespace oor::base
