#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/analysis/validation/fields/FieldChecks.hpp"
#include "oor/model/cards/parameters/parameter/ParameterTable.hpp"
#include "oor/model/parameters/ParameterCatalogue.hpp"

#include <map>
#include <string>

namespace oor::analysis
{

namespace
{

bool sameValue(const model::ParameterCard& left, const model::ParameterCard& right)
{
    return left.kind == right.kind && left.integerValue == right.integerValue && left.realValue == right.realValue &&
           left.imaginaryValue == right.imaginaryValue && left.textValue == right.textValue;
}

std::string describe(const model::ParameterCard& card)
{
    switch (card.kind)
    {
    case model::ParameterKind::Blank:
        return "no value";
    case model::ParameterKind::Integer:
        return std::to_string(card.integerValue);
    case model::ParameterKind::Real:
        return std::to_string(card.realValue);
    case model::ParameterKind::Text:
        return card.textValue;
    case model::ParameterKind::Complex:
        return std::to_string(card.realValue) + ", " + std::to_string(card.imaginaryValue);
    }
    return "";
}

// A parameter of the format reference given twice with different values: the solver takes the last card, so the first
// is dead text. Only the parameters the catalogue knows are judged (the names of a pre-processor may be repeated on
// purpose), and each model on its own (a part superelement has parameters of its own).
class ParameterRepeatedCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"field.parameter-repeated", base::Severity::Warning, "fields",
                                    "A parameter of the format reference is given twice with different values."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::ParameterCatalogue& catalogue = model::ParameterCatalogue::instance();
        std::map<std::string, model::ParameterCard> before;
        forEachCard<model::ParameterTable>(
            context.model(),
            [&](const model::ParameterCard& card, model::CardLocation where)
            {
                if (catalogue.find(card.name) == nullptr)
                    return;
                const auto previous = before.find(card.name);
                if (previous != before.end() && !sameValue(previous->second, card))
                    sink.reportCard(where, "value1",
                                    std::string(card.name) + " is " + describe(previous->second) + " and then " +
                                        describe(card) + ": the last card is the one that counts",
                                    "keep one card, or the value that is meant");
                before[card.name] = card;
            });
    }
};

} // namespace

void addParameterRepeatedCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<ParameterRepeatedCheck>());
}

} // namespace oor::analysis
