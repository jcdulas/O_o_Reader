#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/FieldWriter.hpp"
#include "oor/model/references/ReferenceVisitor.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace oor::model
{

// A section is described by one comma-separated expression with exact neutral keys.
// Example: outerPerimeter=10,thickness(2)=[1.2,points=(123,204)],nonStructMass=0.01.
enum class ArbitrarySectionForm : std::uint8_t
{
    General,
    OpenProfile,
    ClosedProfile
};

inline constexpr std::array<std::string_view, 3> ARBITRARY_SECTION_FORM_WORDS{"GeneralSection", "OpenProfile",
                                                                              "ClosedProfile"};

// Keys describe geometry sets, thickness, composite core and wrapping layers, or mass per unit length.
enum class ArbitrarySectionKey : std::uint8_t
{
    OuterPerimeter,
    OuterMesh,
    InnerPerimeter,
    Branch,
    Thickness,
    Core,
    Layer,
    NonstructuralMass,
};

[[nodiscard]] constexpr unsigned sectionKeyBit(ArbitrarySectionKey key) noexcept
{
    return 1U << static_cast<unsigned>(key);
}

inline constexpr unsigned BAR_SECTION_KEYS =
    sectionKeyBit(ArbitrarySectionKey::OuterPerimeter) | sectionKeyBit(ArbitrarySectionKey::InnerPerimeter) |
    sectionKeyBit(ArbitrarySectionKey::Branch) | sectionKeyBit(ArbitrarySectionKey::Thickness) |
    sectionKeyBit(ArbitrarySectionKey::NonstructuralMass);
inline constexpr unsigned BEAM_SECTION_KEYS = BAR_SECTION_KEYS | sectionKeyBit(ArbitrarySectionKey::OuterMesh) |
                                              sectionKeyBit(ArbitrarySectionKey::Core) |
                                              sectionKeyBit(ArbitrarySectionKey::Layer);

// The first and last point of the segment to which a thickness or core applies.
struct SectionSegment
{
    base::EntityId start = 0;
    base::EntityId end = 0;

    [[nodiscard]] bool operator==(const SectionSegment&) const noexcept = default;
};

// Items retain their order; label zero means no item selector, and target identifies a set, mesh or property.
struct ArbitrarySectionItem
{
    ArbitrarySectionKey key = ArbitrarySectionKey::OuterPerimeter;
    base::EntityId label = 0;
    base::EntityId target = 0;
    base::EntityId layerSet = 0;
    double value = 0.0;
    std::optional<SectionSegment> segment;

    [[nodiscard]] bool operator==(const ArbitrarySectionItem&) const noexcept = default;
};

[[nodiscard]] const char* sectionKeyName(ArbitrarySectionKey key) noexcept;

[[nodiscard]] std::string printSectionItem(const ArbitrarySectionItem& item);
[[nodiscard]] std::vector<std::string> printSectionItems(const std::vector<ArbitrarySectionItem>& items);

// Parses neutral expressions; whitespace may separate tokens, while names and numbers remain intact.
[[nodiscard]] std::optional<CardProblem> parseSectionText(std::string_view text, unsigned allowedKeys,
                                                          std::vector<ArbitrarySectionItem>& items);

[[nodiscard]] std::vector<ArbitrarySectionItem> readSectionItems(FieldReader& reader, int firstField,
                                                                 unsigned allowedKeys);

// Writes one complete description value, independent of its length.
void writeSectionItems(FieldWriter& writer, int firstField, const std::vector<ArbitrarySectionItem>& items);

[[nodiscard]] std::optional<CardProblem> checkSectionItems(const std::vector<ArbitrarySectionItem>& items,
                                                           ArbitrarySectionForm form, unsigned allowedKeys);

void visitSectionItems(std::vector<ArbitrarySectionItem>& items, ReferenceVisitor& visitor);

} // namespace oor::model
