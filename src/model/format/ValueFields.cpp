#include "oor/model/format/ValueFields.hpp"

#include "oor/base/errors/Errors.hpp"
#include "oor/base/text/Json.hpp"

#include <array>
#include <utility>

namespace oor::model
{

namespace
{

constexpr std::array<std::pair<std::string_view, ValueFieldKind>, 7> KINDS{{
    {"Word", ValueFieldKind::Word},
    {"Literal", ValueFieldKind::Literal},
    {"CardType", ValueFieldKind::CardType},
    {"Routine", ValueFieldKind::Routine},
    {"MatrixName", ValueFieldKind::MatrixName},
    {"AeroLabel", ValueFieldKind::AeroLabel},
    {"KeywordValue", ValueFieldKind::KeywordValue},
}};

ValueFieldKind kindOf(std::string_view name)
{
    for (const auto& [word, kind] : KINDS)
        if (word == name)
            return kind;
    throw base::UsageError("value fields: unknown kind " + std::string(name));
}

} // namespace

std::shared_ptr<const ValueFields> ValueFields::fromJson(std::string_view text)
{
    const base::JsonValue root = base::JsonValue::parse(text);
    const base::JsonValue* version = root.member("formatVersion");
    if (version == nullptr || !version->isNumber() || version->asNumber("formatVersion") != 1)
        throw base::UsageError("value fields: formatVersion is not 1");
    const base::JsonValue* cards = root.member("cards");
    if (cards == nullptr || !cards->isObject())
        throw base::UsageError("value fields: missing cards object");
    auto result = std::make_shared<ValueFields>();
    for (const auto& [card, fields] : cards->asObject("cards"))
        for (const auto& [field, kind] : fields.asObject(card))
            result->m_cards[card][field] = kindOf(kind.asText(card + "." + field));
    return result;
}

const std::shared_ptr<const ValueFields>& ValueFields::standard()
{
    static const std::shared_ptr<const ValueFields> STANDARD = fromJson(valueFieldsText());
    return STANDARD;
}

ValueFieldKind ValueFields::kind(std::string_view card, std::string_view field) const
{
    const auto owner = m_cards.find(card);
    if (owner == m_cards.end())
        return ValueFieldKind::Word;
    const auto exact = owner->second.find(field);
    if (exact != owner->second.end())
        return exact->second;
    const std::size_t lastLetter = field.find_last_not_of("0123456789");
    if (lastLetter != std::string_view::npos && lastLetter + 1 < field.size())
    {
        const auto baseName = owner->second.find(field.substr(0, lastLetter + 1));
        if (baseName != owner->second.end())
            return baseName->second;
    }
    return ValueFieldKind::Word;
}

} // namespace oor::model
