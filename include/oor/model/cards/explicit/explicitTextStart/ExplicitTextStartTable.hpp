#pragma once

#include "oor/model/cards/explicit/explicitTextStart/ExplicitTextStartCard.hpp"
#include "oor/model/core/TypedTable.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ExplicitTextStartFormat
{
    // A TODYNA has no value that can be wrong: nothing to check.
    [[nodiscard]] static std::optional<CardProblem> check(const ExplicitTextStartCard& card);
    [[nodiscard]] static std::optional<ExplicitTextStartCard> read(FieldReader& reader);
    static void write(const ExplicitTextStartCard& card, FieldWriter& writer);
};

// All the TODYNA cards: a row each and nothing else, since the card has no field and no ID (IdSpace::None).
class ExplicitTextStartTable final : public TypedTable<ExplicitTextStartTable, ExplicitTextStartCard>
{
public:
    static constexpr std::string_view NAME = "ExplicitTextStart";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = ExplicitTextStartFormat;

    [[nodiscard]] static base::EntityId cardId(const ExplicitTextStartCard&) noexcept { return 0; }
    std::uint32_t append(const ExplicitTextStartCard&, base::CardKey key) { return m_rows.append(key); }
    [[nodiscard]] ExplicitTextStartCard card(std::uint32_t) const { return {}; }
    void store(std::uint32_t, const ExplicitTextStartCard&) {}
    void keepRows(const std::vector<std::uint32_t>&) {}

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // no field, no reference
};

} // namespace oor::model
