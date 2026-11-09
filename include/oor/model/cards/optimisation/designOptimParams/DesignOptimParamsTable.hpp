#pragma once

#include "oor/model/cards/optimisation/designOptimParams/DesignOptimParamsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct DesignOptimParamsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DesignOptimParamsCard& card);
    [[nodiscard]] static std::optional<DesignOptimParamsCard> read(FieldReader& reader);
    static void write(const DesignOptimParamsCard& card, FieldWriter& writer);
};

// All the DOPTPRM cards (no ID: IdSpace::None). A model has one; a second is reported by `design.doptprm-repeated`.
class DesignOptimParamsTable final : public TypedTable<DesignOptimParamsTable, DesignOptimParamsCard>
{
public:
    static constexpr std::string_view NAME = "DesignOptimParams";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = DesignOptimParamsFormat;

    [[nodiscard]] static base::EntityId cardId(const DesignOptimParamsCard&) noexcept { return 0; }
    std::uint32_t append(const DesignOptimParamsCard& card, base::CardKey key);
    [[nodiscard]] DesignOptimParamsCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const DesignOptimParamsCard& card) { m_cards.set(row, card); }
    void keepRows(const std::vector<std::uint32_t>& rows) { m_cards.keepRows(rows); }

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {}

private:
    CardRows<DesignOptimParamsCard> m_cards;
};

} // namespace oor::model
