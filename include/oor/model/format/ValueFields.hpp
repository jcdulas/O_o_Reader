#pragma once

#include <array>
#include <map>
#include <memory>
#include <string>
#include <string_view>

namespace oor::model
{

// The semantic role of a neutral text field. Most fields hold a vocabulary word; names, labels and text data
// stay literal even when they happen to spell a known keyword. A card-type field uses the cards section instead.
enum class ValueFieldKind
{
    Word,
    Literal,
    CardType,
    Routine,
    MatrixName,
    AeroLabel,
    KeywordValue
};

// The reserved labels of rigid trim motions and the control-vector intercept. Other labels remain user text.
[[nodiscard]] inline constexpr bool isBuiltinAeroLabel(std::string_view name) noexcept
{
    constexpr std::array<std::string_view, 12> LABELS{
        "AngleOfAttack",      "SideSlip",    "RollRate",    "PitchRate",          "YawRate",
        "RigidAccelX",        "RigidAccelY", "RigidAccelZ", "RigidAngularAccelX", "RigidAngularAccelY",
        "RigidAngularAccelZ", "Intercept"};
    for (const std::string_view label : LABELS)
        if (name == label)
            return true;
    return false;
}

// The field semantics that the layout's scalar kind (text, any, ...) alone cannot express. The source table is
// formats/value-fields.json; numbered fields also match their base name (parameterName2 matches parameterName).
class ValueFields
{
public:
    [[nodiscard]] static const std::shared_ptr<const ValueFields>& standard();
    [[nodiscard]] static std::shared_ptr<const ValueFields> fromJson(std::string_view text);
    [[nodiscard]] ValueFieldKind kind(std::string_view card, std::string_view field) const;

private:
    std::map<std::string, std::map<std::string, ValueFieldKind, std::less<>>, std::less<>> m_cards;
};

[[nodiscard]] std::string_view valueFieldsText() noexcept;

} // namespace oor::model
