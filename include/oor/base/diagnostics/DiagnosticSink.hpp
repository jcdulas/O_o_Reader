#pragma once

#include "oor/base/diagnostics/Diagnostic.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace oor::base
{

// Strict: the first error ends the read. Tolerant: errors are collected and reading goes on.
enum class ReadMode
{
    Strict,
    Tolerant
};

// Collects the diagnostics of a read. Parsers write here and never throw on file content; the entry points turn the
// first error into a DeckError in strict mode (see errors/Errors.hpp).
//
// Not thread-safe. Parallel code gives each chunk its own sink and merges them in chunk order with append(), which
// keeps the diagnostics in file order whatever the thread count.
class DiagnosticSink
{
public:
    explicit DiagnosticSink(ReadMode mode = ReadMode::Tolerant) : m_mode(mode) {}

    void report(Diagnostic diagnostic);
    void error(std::string code, std::string message, SourceLocation location = {});
    void warning(std::string code, std::string message, SourceLocation location = {});
    void info(std::string code, std::string message, SourceLocation location = {});

    // Moves the diagnostics of `other` after the ones already here.
    void append(DiagnosticSink&& other);

    [[nodiscard]] ReadMode mode() const noexcept { return m_mode; }
    [[nodiscard]] bool hasErrors() const noexcept { return m_errorCount > 0; }
    [[nodiscard]] std::size_t errorCount() const noexcept { return m_errorCount; }
    // True when the caller should stop: strict mode and at least one error.
    [[nodiscard]] bool shouldStop() const noexcept { return m_mode == ReadMode::Strict && hasErrors(); }
    [[nodiscard]] const std::vector<Diagnostic>& diagnostics() const noexcept { return m_diagnostics; }
    [[nodiscard]] std::optional<Diagnostic> firstError() const;

private:
    ReadMode m_mode;
    std::size_t m_errorCount = 0;
    std::vector<Diagnostic> m_diagnostics;
};

} // namespace oor::base
