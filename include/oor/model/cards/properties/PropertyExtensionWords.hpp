#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/FieldWriter.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <span>
#include <string>
#include <string_view>

// What the nonlinear property extensions (PBARN1, PBEMN1, PRODN1, PSHEARN, PSLDN1) and the layered solid and plane
// composites (PCOMPLS, PLCOMP) have in common: the words of the format reference tables (the analysis type, the section
// integration, the behaviour and integration codes of an element) and the keyword line "Ck BEHk INTk BEHkH INTkH". Each
// helper moves one kind of value. Nothing here describes the fields of a card: every card still lays out its own first
// line, says which keyword lines it has and which words each line allows (`BehaviorRules`, written next to the card).
namespace oor::model
{

// ---------------------------------------------------------------------------------------------------------------------
// The words
// ---------------------------------------------------------------------------------------------------------------------

// ANAL: which analysis the elements that name the property belong to. The enumerators follow PROPERTY_ANALYSIS_WORDS.
enum class PropertyAnalysis : std::uint8_t
{
    Structural,        // IS: implicit structural elements
    Heat,              // IH: implicit heat elements
    StructuralAndHeat, // ISH: both (the default of the cards that have a heat part)
};

inline constexpr std::array<std::string_view, 3> PROPERTY_ANALYSIS_WORDS{"ImplicitStructural", "ImplicitHeat",
                                                                         "ImplicitStructuralHeat"};

// SECT of a PBARN1 and a PBEMN1: how the section is integrated.
enum class SectionIntegration : std::uint8_t
{
    Smeared,   // S: the section properties of the PBAR or PBEAM (the default)
    Numerical, // N: the section is integrated numerically (PBARL, PBEAML)
};

inline constexpr std::array<std::string_view, 2> SECTION_INTEGRATION_WORDS{"Smeared", "Numerical"};

// The BEHi and BEHiH codes of the tables of the format reference (the structural and heat classification of elements).
// A card allows some of them on each keyword line (`BehaviorRules`).
enum class ElementBehavior : std::uint8_t
{
    Bar,                  // BAR
    Beam,                 // BEAM
    Rod,                  // ROD
    Shear,                // MB: the membrane behaviour of a shear panel
    Solid,                // SOLID
    SolidComposite,       // SLCOMP: layered solid (also the 3D solid gasket and the solid shell of a PSLDN1)
    IncompressibleSolid,  // ISOL
    PlaneStrainComposite, // COMPS
    AxisymmetricComposite // AXCOMP
};

inline constexpr std::array<std::string_view, 9> ELEMENT_BEHAVIOR_WORDS{"Bar",
                                                                        "Beam",
                                                                        "Rod",
                                                                        "Membrane",
                                                                        "Solid",
                                                                        "SolidComposite",
                                                                        "IncompressibleSolid",
                                                                        "PlaneStrainComposite",
                                                                        "AxisymComposite"};

// The INTi and INTiH codes of the tables of the format reference.
enum class IntegrationScheme : std::uint8_t
{
    Linear,                 // L
    LinearCubic,            // LC
    LinearCubicClosed,      // LCC
    LinearCubicOpen,        // LCO
    LinearShear,            // LS
    LinearReducedHourglass, // LRIH
    AssumedStrain,          // ASTN
    Quadratic,              // Q
    QuadraticReduced        // QRI
};

inline constexpr std::array<std::string_view, 9> INTEGRATION_SCHEME_WORDS{
    "Linear",        "LinearCubic", "LinearCubicClosed", "LinearCubicOpen", "LinearShear", "LinearReducedHourglass",
    "AssumedStrain", "Quadratic",   "QuadraticReduced"};

// ---------------------------------------------------------------------------------------------------------------------
// The keyword line
// ---------------------------------------------------------------------------------------------------------------------

// The words of one keyword line: the structural behaviour and integration, and (on the cards that have them) the heat
// ones. A card that has no heat words leaves the two heat members at their defaults and never writes them.
struct BehaviorGroup
{
    ElementBehavior behavior = ElementBehavior::Bar;
    IntegrationScheme integration = IntegrationScheme::Linear;
    ElementBehavior heatBehavior = ElementBehavior::Bar;
    IntegrationScheme heatIntegration = IntegrationScheme::Linear;

    [[nodiscard]] bool operator==(const BehaviorGroup&) const noexcept = default;
};

// What a card allows on the line of one keyword: the names of its fields (for the diagnostics) and the words each
// position takes, in the order of the format reference tables; the first word of each list is the default (the one the
// format reference underlines), which a line that is not in the card gets and a blank field gets. A card without heat
// words has empty heat lists. The lists are static constants written next to the card.
struct BehaviorRules
{
    const char* keyword = "";             // "C8"
    const char* behaviorName = "";        // "behavior8"
    const char* integrationName = "";     // "integration8"
    const char* heatBehaviorName = "";    // "heatBehavior8"
    const char* heatIntegrationName = ""; // "heatIntegration8"
    std::span<const ElementBehavior> behaviors{};
    std::span<const IntegrationScheme> integrations{};
    std::span<const ElementBehavior> heatBehaviors{};
    std::span<const IntegrationScheme> heatIntegrations{};

    [[nodiscard]] constexpr bool hasHeat() const noexcept { return !heatBehaviors.empty(); }
};

// The group a keyword gets when the card does not give its line: the first word of each list.
[[nodiscard]] constexpr BehaviorGroup defaultGroup(const BehaviorRules& rules) noexcept
{
    BehaviorGroup group;
    group.behavior = rules.behaviors.front();
    group.integration = rules.integrations.front();
    if (rules.hasHeat())
    {
        group.heatBehavior = rules.heatBehaviors.front();
        group.heatIntegration = rules.heatIntegrations.front();
    }
    return group;
}

template <std::size_t COUNT>
[[nodiscard]] constexpr std::array<BehaviorGroup, COUNT> defaultGroups(const std::array<BehaviorRules, COUNT>& rules)
{
    std::array<BehaviorGroup, COUNT> groups{};
    for (std::size_t index = 0; index < COUNT; ++index)
        groups[index] = defaultGroup(rules[index]);
    return groups;
}

// A line of fields of a continuation line of the Card layout.
inline constexpr int EXTENSION_LINE_FIELDS = 8;

// The position of the keyword `text` (without regard to the case) among the rules, if there is one.
[[nodiscard]] std::optional<std::size_t> findBehaviorRules(std::span<const BehaviorRules> rules,
                                                           std::string_view text) noexcept;

// "C4, C8 or C20": the keywords of the rules, for a message.
[[nodiscard]] std::string behaviorKeywordList(std::span<const BehaviorRules> rules);

// The words of the allowed list as a phrase for a message: "L, LRIH or ASTN".
template <typename Word, std::size_t COUNT>
[[nodiscard]] std::string allowedWordList(const std::array<std::string_view, COUNT>& words,
                                          std::span<const Word> allowed)
{
    std::string phrase;
    for (std::size_t index = 0; index < allowed.size(); ++index)
    {
        if (index > 0)
            phrase += index + 1 == allowed.size() ? " or " : ", ";
        phrase += words[static_cast<std::size_t>(allowed[index])];
    }
    return phrase;
}

// A word of a keyword line: blank is the first allowed word (the default), a word that is not in the list the card
// allows at this place is an error that names the field.
template <typename Word, std::size_t COUNT>
[[nodiscard]] Word readGroupWord(FieldReader& reader, int field, const char* name,
                                 const std::array<std::string_view, COUNT>& words, std::span<const Word> allowed)
{
    const std::string_view text = reader.text(field);
    if (text.empty())
        return allowed.front();
    if (const std::optional<Word> word = parseKeyword<Word>(words, text);
        word && std::find(allowed.begin(), allowed.end(), *word) != allowed.end())
        return *word;
    reader.error(
        "card.bad-value",
        std::string(name) + " must be " + allowedWordList<Word>(words, allowed) + ", not \"" + std::string(text) + "\"",
        name);
    return allowed.front();
}

// The words of the line whose keyword is in field `keywordField`: the words are in the fields that follow, and the rest
// of the line must be blank.
[[nodiscard]] BehaviorGroup readBehaviorGroup(FieldReader& reader, int keywordField, const BehaviorRules& rules);

// True when the writer puts the line of `group` in the card: always when the card is written without omitting defaults,
// else when the line says something the defaults do not.
[[nodiscard]] bool writesBehaviorGroup(const FieldWriter& writer, const BehaviorRules& rules,
                                       const BehaviorGroup& group);

// Writes the keyword and the words of the line, from field `keywordField`; a word that is the default is left blank.
void writeBehaviorGroup(FieldWriter& writer, int keywordField, const BehaviorRules& rules, const BehaviorGroup& group);

// The first word that the rules do not allow, if there is one.
[[nodiscard]] std::optional<CardProblem> checkBehaviorGroup(const BehaviorGroup& group, const BehaviorRules& rules);

// A card whose continuation lines from `firstField` on are all keyword lines (PBARN1, PBEMN1, PRODN1, PSHEARN, PSLDN1):
// the lines may come in any order and may be missing (the group keeps its default), blank lines are padding, a keyword
// may not be given twice. `groups` has one entry for each of `rules`, in the same order.
void readBehaviorLines(FieldReader& reader, int firstField, std::span<const BehaviorRules> rules,
                       std::span<BehaviorGroup> groups);

// Writes the lines that need writing, one after the other from `firstField`, in the order of the rules; returns the
// next free field.
int writeBehaviorLines(FieldWriter& writer, int firstField, std::span<const BehaviorRules> rules,
                       std::span<const BehaviorGroup> groups);

[[nodiscard]] std::optional<CardProblem> checkBehaviorLines(std::span<const BehaviorRules> rules,
                                                            std::span<const BehaviorGroup> groups);

// Reports the fields of the first line that the Card layout of the card leaves blank and the deck fills.
void requireUnusedFields(FieldReader& reader, std::initializer_list<int> fields);

} // namespace oor::model
