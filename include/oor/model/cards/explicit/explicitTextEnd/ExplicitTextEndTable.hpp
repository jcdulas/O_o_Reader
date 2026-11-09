#pragma once

#include "oor/model/cards/explicit/explicitTextEnd/ExplicitTextEndCard.hpp"
#include "oor/model/core/TypedTable.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ExplicitTextEndFormat
{
    // An ENDDYNA has no value that can be wrong: nothing to check.
    [[nodiscard]] static std::optional<CardProblem> check(const ExplicitTextEndCard& card);
    [[nodiscard]] static std::optional<ExplicitTextEndCard> read(FieldReader& reader);
    static void write(const ExplicitTextEndCard& card, FieldWriter& writer);
};

// All the ENDDYNA cards: a row each and nothing else, since the card has no field and no ID (IdSpace::None).
class ExplicitTextEndTable final : public TypedTable<ExplicitTextEndTable, ExplicitTextEndCard>
{
public:
    static constexpr std::string_view NAME = "ExplicitTextEnd";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = ExplicitTextEndFormat;

    [[nodiscard]] static base::EntityId cardId(const ExplicitTextEndCard&) noexcept { return 0; }
    std::uint32_t append(const ExplicitTextEndCard&, base::CardKey key) { return m_rows.append(key); }
    [[nodiscard]] ExplicitTextEndCard card(std::uint32_t) const { return {}; }
    void store(std::uint32_t, const ExplicitTextEndCard&) {}
    void keepRows(const std::vector<std::uint32_t>&) {}

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // no field, no reference
};

} // namespace oor::model
