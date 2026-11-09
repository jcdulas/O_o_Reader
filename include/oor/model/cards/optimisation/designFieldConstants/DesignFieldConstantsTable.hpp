#pragma once

#include "oor/model/cards/optimisation/designFieldConstants/DesignFieldConstantsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct DesignFieldConstantsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DesignFieldConstantsCard& card);
    [[nodiscard]] static std::optional<DesignFieldConstantsCard> read(FieldReader& reader);
    static void write(const DesignFieldConstantsCard& card, FieldWriter& writer);
};

// All the DTABLE2 cards (no ID: IdSpace::None). The PIDi of an entry names a property, a material or an element, which
// the card named by PNAMEi says (a card type the library does not type is not looked up).
class DesignFieldConstantsTable final : public TypedTable<DesignFieldConstantsTable, DesignFieldConstantsCard>
{
public:
    static constexpr std::string_view NAME = "DesignFieldConstants";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = DesignFieldConstantsFormat;

    [[nodiscard]] static base::EntityId cardId(const DesignFieldConstantsCard&) noexcept { return 0; }
    std::uint32_t append(const DesignFieldConstantsCard& card, base::CardKey key);
    [[nodiscard]] DesignFieldConstantsCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const DesignFieldConstantsCard& card) { m_cards.set(row, card); }
    void keepRows(const std::vector<std::uint32_t>& rows) { m_cards.keepRows(rows); }

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

private:
    CardRows<DesignFieldConstantsCard> m_cards;
};

} // namespace oor::model
