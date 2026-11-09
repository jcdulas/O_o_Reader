#include "oor/model/cards/optimisation/DesignTargets.hpp"

#include "oor/model/core/CardRegistry.hpp"

#include <string>

namespace oor::model
{

std::optional<std::uint16_t> typeOfDesignedCard(std::string_view cardName)
{
    return CardRegistry::instance().typeIndexOf(cardName);
}

std::optional<CardProblem> checkNeutralName(std::string_view name, const char* field)
{
    const auto isLetter = [](char character)
    { return (character >= 'A' && character <= 'Z') || (character >= 'a' && character <= 'z'); };
    const auto isDigit = [](char character) { return character >= '0' && character <= '9'; };
    bool valid = !name.empty() && isLetter(name.front());
    for (const char character : name)
        valid = valid && (isLetter(character) || isDigit(character));
    if (valid)
        return std::nullopt;
    return CardProblem{"card.bad-value",
                       std::string(CardProblem::FIELD) + " must be a name of letters and digits, the first a letter",
                       field};
}

std::optional<ReferenceTarget> targetOfDesignedCard(std::string_view cardName)
{
    const CardRegistry& registry = CardRegistry::instance();
    const std::optional<std::uint16_t> type = typeOfDesignedCard(cardName);
    if (!type)
        return std::nullopt;
    switch (registry.entry(*type).info.category)
    {
    case CardCategory::Property:
        return ReferenceTarget::Property;
    case CardCategory::Material:
        return ReferenceTarget::Material;
    case CardCategory::ThermalMaterial:
        return ReferenceTarget::ThermalMaterial;
    case CardCategory::Element:
        return ReferenceTarget::Element;
    default:
        break;
    }
    return std::nullopt;
}

} // namespace oor::model
