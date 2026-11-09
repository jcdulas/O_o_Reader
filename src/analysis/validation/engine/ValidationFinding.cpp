#include "oor/analysis/validation/engine/ValidationFinding.hpp"

#include "oor/analysis/validation/engine/ValidationCheck.hpp"
#include "oor/analysis/validation/engine/ValidationError.hpp"
#include "oor/model/control/case_control/Subcase.hpp"
#include "oor/model/core/CardRegistry.hpp"

#include <algorithm>

namespace oor::analysis
{

namespace
{

// The keyword of a statement a finding is filed under, as the deck writes it: a kind of Case Control section by the
// command that opens it ("subcase": SUBCASE, "global": GLOBAL), an Executive Control or File Management statement
// ("SolutionSequence": SOL), a word of the deck structure ("SectionBegin": BEGIN); the name as it is otherwise.

} // namespace

std::string ValidationFinding::toString() const
{
    std::string text(base::severityName(severity));
    text += "[" + code + "] ";
    if (!part.empty())
        text += part + ": ";
    if (!cardName.empty())
    {
        text += cardName;
        if (cardId != 0)
            text += " " + std::to_string(cardId);
        if (!field.empty())
            text += ", field " + field;
        text += ": ";
    }
    return text + message;
}

std::size_t ValidationReport::count(base::Severity severity) const noexcept
{
    return static_cast<std::size_t>(std::ranges::count_if(
        findings, [severity](const ValidationFinding& finding) { return finding.severity == severity; }));
}

std::string ValidationError::describe(const ValidationReport& report)
{
    const std::size_t errors = report.errorCount();
    std::string text = "validation found " + std::to_string(errors) + (errors == 1 ? " error" : " errors");
    const auto first = std::ranges::find_if(
        report.findings, [](const ValidationFinding& finding) { return finding.severity == base::Severity::Error; });
    if (first != report.findings.end())
        text += "; the first: " + first->toString();
    return text;
}

void FindingSink::report(FindingDraft draft)
{
    ValidationFinding finding;
    finding.severity = m_info.severity;
    finding.code = std::string(m_info.code);
    finding.family = std::string(m_info.family);
    finding.message = std::move(draft.message);
    finding.cardName = std::move(draft.cardName);
    finding.cardId = draft.cardId;
    finding.cardKey = draft.cardKey;
    finding.field = std::move(draft.field);
    finding.suggestion = std::move(draft.suggestion);
    finding.part = m_part;
    m_out.push_back(std::move(finding));
    ++m_count;
}

void FindingSink::reportCard(model::CardLocation where, std::string field, std::string message, std::string suggestion)
{
    FindingDraft draft;
    draft.message = std::move(message);
    draft.field = std::move(field);
    draft.suggestion = std::move(suggestion);
    if (const model::CardTable* table = m_model.findTable(where.table); table != nullptr)
    {
        draft.cardName = std::string(table->info().name);
        draft.cardId = table->idAt(where.row);
        draft.cardKey = table->rows().keyAt(where.row).value;
    }
    report(std::move(draft));
}

void FindingSink::reportStatement(std::string statement, std::int64_t number, std::string message,
                                  std::string suggestion)
{
    FindingDraft draft;
    draft.message = std::move(message);
    draft.suggestion = std::move(suggestion);
    draft.cardName = std::move(statement);
    draft.cardId = static_cast<base::EntityId>(number);
    report(std::move(draft));
}

} // namespace oor::analysis
