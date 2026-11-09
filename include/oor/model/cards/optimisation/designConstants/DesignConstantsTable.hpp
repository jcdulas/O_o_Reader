#pragma once

#include "oor/model/cards/optimisation/designConstants/DesignConstantsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct DesignConstantsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DesignConstantsCard& card);
    [[nodiscard]] static std::optional<DesignConstantsCard> read(FieldReader& reader);
    static void write(const DesignConstantsCard& card, FieldWriter& writer);
};

// All the DTABLE cards. A card has no ID (IdSpace::None): the constants are found by label, and the labels must be
// unique over every DTABLE and DTABLE2 card (the check `design.dtable-label-repeated`).
class DesignConstantsTable final : public TypedTable<DesignConstantsTable, DesignConstantsCard>
{
public:
    static constexpr std::string_view NAME = "DesignConstants";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = DesignConstantsFormat;

    [[nodiscard]] static base::EntityId cardId(const DesignConstantsCard&) noexcept { return 0; }
    std::uint32_t append(const DesignConstantsCard& card, base::CardKey key);
    [[nodiscard]] DesignConstantsCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const DesignConstantsCard& card) { m_cards.set(row, card); }
    void keepRows(const std::vector<std::uint32_t>& rows) { m_cards.keepRows(rows); }

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

private:
    CardRows<DesignConstantsCard> m_cards;
};

} // namespace oor::model
