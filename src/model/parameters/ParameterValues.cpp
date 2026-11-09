#include "oor/model/parameters/ParameterValues.hpp"

#include "oor/model/cards/parameters/parameter/ParameterTable.hpp"
#include "oor/model/cards/solution_control/modelParams/ModelParamsTable.hpp"
#include "oor/model/parameters/ParameterCatalogue.hpp"

namespace oor::model
{

namespace
{

// The last PARAM card of the name for which `accept` is true.
template <typename Accept>
std::optional<ParameterCard> lastCard(const Model& model, std::string_view name, Accept accept)
{
    std::optional<ParameterCard> found;
    const ParameterTable* table = model.findTable<ParameterTable>();
    if (table == nullptr)
        return found;
    for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
    {
        if (!table->rows().isAlive(row))
            continue;
        ParameterCard card = table->card(row);
        if (card.name == name && accept(card))
            found = std::move(card);
    }
    return found;
}

std::optional<ParameterCard> defaultOf(std::string_view name)
{
    const ParamInfo* info = ParameterCatalogue::instance().find(name);
    if (info == nullptr || !info->defaultValue.exists())
        return std::nullopt;
    return ParameterCatalogue::defaultCard(*info);
}

bool isNumber(const ParameterCard& card) noexcept
{
    return card.kind == ParameterKind::Integer || card.kind == ParameterKind::Real;
}

double numberOf(const ParameterCard& card) noexcept
{
    return card.kind == ParameterKind::Integer ? static_cast<double>(card.integerValue) : card.realValue;
}

} // namespace

std::optional<ParameterValue> parameterValue(const Model& model, std::string_view name)
{
    if (std::optional<ParameterCard> card = lastCard(model, name, [](const ParameterCard&) { return true; }))
        return ParameterValue{std::move(*card), true};
    if (std::optional<ParameterCard> card = defaultOf(name))
        return ParameterValue{std::move(*card), false};
    return std::nullopt;
}

std::optional<double> realParameter(const Model& model, std::string_view name)
{
    if (const std::optional<ParameterCard> card = lastCard(model, name, isNumber))
        return numberOf(*card);
    if (const std::optional<ParameterCard> card = defaultOf(name); card && isNumber(*card))
        return numberOf(*card);
    return std::nullopt;
}

std::optional<std::int32_t> integerParameter(const Model& model, std::string_view name)
{
    const auto isInteger = [](const ParameterCard& card) { return card.kind == ParameterKind::Integer; };
    if (const std::optional<ParameterCard> card = lastCard(model, name, isInteger))
        return card->integerValue;
    if (const std::optional<ParameterCard> card = defaultOf(name); card && isInteger(*card))
        return card->integerValue;
    return std::nullopt;
}

std::optional<ModelParameter> modelParameter(const Model& model, std::string_view name)
{
    std::optional<ModelParameter> found;
    const ModelParamsTable* table = model.findTable<ModelParamsTable>();
    if (table == nullptr)
        return found;
    for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
    {
        if (!table->rows().isAlive(row))
            continue;
        ModelParamsCard card = table->card(row);
        for (ModelParameter& parameter : card.parameters)
        {
            if (parameter.name == name)
                found = std::move(parameter);
        }
    }
    return found;
}

} // namespace oor::model
