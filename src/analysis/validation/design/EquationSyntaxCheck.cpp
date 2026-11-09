#include "oor/analysis/validation/design/DesignChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/base/text/NumberParser.hpp"
#include "oor/model/core/UnknownCardTable.hpp"
#include "oor/model/expressions/EquationSet.hpp"

#include <string>

namespace oor::analysis
{

namespace
{

// The lexer gives every line of a DEQATN as eight fields: the ID (blank after the first line), then the text of the
// line, then six blanks (the same layout the card reads, `DesignEquationFormat::read`).
constexpr std::size_t FIELDS_PER_LINE = 8;
constexpr std::size_t FIRST_TEXT_INDEX = 1;
constexpr std::string_view EQUATION_TYPE = "DesignEquation";

// A DEQATN whose equations do not parse is not a card of the model: in tolerant mode the reader reports it, keeps the
// lines as text and writes them back as they were. What the reader said is not part of the model, so this check says it
// again, with the place the parser stopped; and it says why the DVPREL2 or DRESP2 that names the equation has none
// (`ref.unresolved`). An equation whose text is good but whose ID is not (the card is kept as text for that) is the
// reader's business.
class EquationSyntaxCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"design.equation-syntax", base::Severity::Error, "design",
                                    "A design equation card has equations that cannot be read, so the card is kept as "
                                    "text."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const auto* table =
            dynamic_cast<const model::UnknownCardTable*>(context.model().findTable(model::CardRegistry::UNKNOWN_TYPE));
        if (table == nullptr)
            return;
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            // an unknown card keeps the name the deck gives it: its keyword, turned into a card type here
            if (!table->rows().isAlive(row) || table->nameAt(row).view() != EQUATION_TYPE)
                continue;
            const std::vector<std::string> fields = table->fieldsAt(row);
            std::string text;
            for (std::size_t index = FIRST_TEXT_INDEX; index < fields.size(); index += FIELDS_PER_LINE)
                text += fields[index];
            const model::EquationParse parsed = model::parseEquations(text);
            if (parsed.ok())
                continue;
            FindingDraft draft;
            draft.cardName = std::string(EQUATION_TYPE);
            if (!fields.empty())
            {
                if (const auto id = base::NumberParser::parseInteger(fields.front()))
                    draft.cardId = static_cast<base::EntityId>(*id);
            }
            draft.cardKey = table->rows().keyAt(row).value;
            draft.field = "equation";
            draft.message = parsed.problem->message + " (character " + std::to_string(parsed.problem->position) +
                            " of the equation)";
            draft.suggestion = "correct the equation: the card is kept as text and no relation can use it";
            sink.report(std::move(draft));
        }
    }
};

} // namespace

void addEquationSyntaxCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<EquationSyntaxCheck>());
}

} // namespace oor::analysis
