#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/references/ReferenceChecks.hpp"

#include <charconv>

namespace oor::analysis
{

namespace
{

// An ID defined by two cards of one ID space. The reader noticed it (a tolerant read keeps the last card and records
// the diagnostic "deck.duplicate-id"); this check puts it in the report, so that a script that validates sees it too.
class DuplicateIdCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"ref.duplicate-id", base::Severity::Error, "references",
                                    "Two cards of one ID space have the same ID (found when the deck was read)."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        for (const base::Diagnostic& diagnostic : context.model().diagnostics().diagnostics())
        {
            if (diagnostic.code != "deck.duplicate-id")
                continue;
            FindingDraft draft;
            draft.message = diagnostic.message;
            draft.cardName = diagnostic.location.cardName;
            draft.cardId = idOf(diagnostic.message);
            draft.suggestion = "give one of the cards another ID (renumber the model)";
            sink.report(std::move(draft));
        }
    }

private:
    // The message of the reader starts "ID 25 is used by two cards ...".
    static base::EntityId idOf(const std::string& message)
    {
        base::EntityId id = 0;
        if (message.starts_with("ID "))
            std::from_chars(message.data() + 3, message.data() + message.size(), id);
        return id;
    }
};

} // namespace

void addDuplicateIdCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<DuplicateIdCheck>());
}

} // namespace oor::analysis
