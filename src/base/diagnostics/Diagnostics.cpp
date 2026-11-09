#include "oor/base/diagnostics/Diagnostic.hpp"
#include "oor/base/diagnostics/DiagnosticSink.hpp"
#include "oor/base/diagnostics/Severity.hpp"
#include "oor/base/diagnostics/SourceLocation.hpp"

#include <iterator>
#include <utility>

namespace oor::base
{

std::string_view severityName(Severity severity) noexcept
{
    switch (severity)
    {
    case Severity::Info:
        return "info";
    case Severity::Warning:
        return "warning";
    case Severity::Error:
        return "error";
    }
    return "error";
}

std::string SourceLocation::toString() const
{
    std::string text = file;
    if (line != 0)
        text += (text.empty() ? "line " : ":") + std::to_string(line);
    std::string detail = cardName;
    if (!fieldName.empty())
        detail += (detail.empty() ? "" : ", ") + fieldName;
    if (!detail.empty())
        text += (text.empty() ? "" : " ") + std::string("(") + detail + ")";
    return text;
}

std::string Diagnostic::toString() const
{
    std::string text(severityName(severity));
    if (!code.empty())
        text += "[" + code + "]";
    const std::string where = location.toString();
    if (!where.empty())
        text += " " + where;
    return text + ": " + message;
}

void DiagnosticSink::report(Diagnostic diagnostic)
{
    if (diagnostic.severity == Severity::Error)
        ++m_errorCount;
    m_diagnostics.push_back(std::move(diagnostic));
}

void DiagnosticSink::error(std::string code, std::string message, SourceLocation location)
{
    report(Diagnostic{Severity::Error, std::move(code), std::move(message), std::move(location)});
}

void DiagnosticSink::warning(std::string code, std::string message, SourceLocation location)
{
    report(Diagnostic{Severity::Warning, std::move(code), std::move(message), std::move(location)});
}

void DiagnosticSink::info(std::string code, std::string message, SourceLocation location)
{
    report(Diagnostic{Severity::Info, std::move(code), std::move(message), std::move(location)});
}

void DiagnosticSink::append(DiagnosticSink&& other)
{
    m_errorCount += other.m_errorCount;
    m_diagnostics.insert(m_diagnostics.end(), std::make_move_iterator(other.m_diagnostics.begin()),
                         std::make_move_iterator(other.m_diagnostics.end()));
    other.m_diagnostics.clear();
    other.m_errorCount = 0;
}

std::optional<Diagnostic> DiagnosticSink::firstError() const
{
    for (const Diagnostic& diagnostic : m_diagnostics)
    {
        if (diagnostic.severity == Severity::Error)
            return diagnostic;
    }
    return std::nullopt;
}

} // namespace oor::base
