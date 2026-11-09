#pragma once

#include "oor/analysis/validation/engine/ValidationFinding.hpp"
#include "oor/model/core/Model.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace oor::analysis
{

class ValidationContext;

// Where a check applies: to every model of the deck (the residual structure and each part, each with its own tables),
// or to the deck as a whole (the control sections, which only the residual structure has, and what compares the parts).
enum class CheckScope : std::uint8_t
{
    Model,
    Deck
};

// What the catalogue says of a check. One check, one code.
struct CheckInfo
{
    std::string_view code; // "ref.unresolved": the family's prefix, a dot, a short name
    base::Severity severity = base::Severity::Error;
    std::string_view family;      // "references"
    std::string_view description; // one sentence: what the check finds
    bool optIn = false;           // runs only when asked for by its code (or by "all")
    CheckScope scope = CheckScope::Model;
};

// What a check says about a finding; the rest (code, severity, family, part) is the check's own and added by the sink.
struct FindingDraft
{
    std::string message;
    std::string cardName;
    base::EntityId cardId = 0;
    std::uint32_t cardKey = base::CardKey::INVALID_VALUE;
    std::string field;
    std::string suggestion;
};

// Where a check puts its findings. It stamps each with the check's code, severity and family and with the part being
// validated, so that a check cannot report under another name. One sink per check run; not shared between threads.
class FindingSink
{
public:
    FindingSink(const CheckInfo& info, const model::Model& model, std::string part, std::vector<ValidationFinding>& out)
        : m_info(info), m_model(model), m_part(std::move(part)), m_out(out)
    {
    }

    void report(FindingDraft draft);
    // A finding about a card of the model being validated: its name, ID and key come from its table. `field` is the
    // field at fault (empty for the card as a whole).
    void reportCard(model::CardLocation where, std::string field, std::string message, std::string suggestion = {});
    // A finding about a statement of a control section or about the deck: `statement` names it (SUBCASE, SOL, PARAM),
    // `number` is its number when it has one.
    void reportStatement(std::string statement, std::int64_t number, std::string message, std::string suggestion = {});

    [[nodiscard]] std::size_t count() const noexcept { return m_count; }

private:
    const CheckInfo& m_info;
    const model::Model& m_model;
    std::string m_part;
    std::vector<ValidationFinding>& m_out;
    std::size_t m_count = 0;
};

// One check: a class of its own, found by its code. `run` looks at the model of the context (read-only: it may run at
// the same time as the checks of other families) and reports into the sink. It does not throw on a model it finds
// wrong; a check that needs something the model lacks (a coordinate system that cannot be resolved) leaves what it
// cannot judge to the check that reports the cause.
class ValidationCheck
{
public:
    virtual ~ValidationCheck() = default;
    [[nodiscard]] virtual const CheckInfo& info() const noexcept = 0;
    virtual void run(const ValidationContext& context, FindingSink& sink) const = 0;
};

} // namespace oor::analysis
