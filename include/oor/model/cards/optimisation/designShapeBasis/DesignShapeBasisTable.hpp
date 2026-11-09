#pragma once

#include "oor/model/cards/optimisation/designShapeBasis/DesignShapeBasisCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct DesignShapeBasisFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DesignShapeBasisCard& card);
    [[nodiscard]] static std::optional<DesignShapeBasisCard> read(FieldReader& reader);
    static void write(const DesignShapeBasisCard& card, FieldWriter& writer);
};

// All the DVSHAP cards (no ID: IdSpace::None). The DVID is a design variable.
class DesignShapeBasisTable final : public TypedTable<DesignShapeBasisTable, DesignShapeBasisCard>
{
public:
    static constexpr std::string_view NAME = "DesignShapeBasis";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = DesignShapeBasisFormat;

    [[nodiscard]] static base::EntityId cardId(const DesignShapeBasisCard&) noexcept { return 0; }
    std::uint32_t append(const DesignShapeBasisCard& card, base::CardKey key);
    [[nodiscard]] DesignShapeBasisCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const DesignShapeBasisCard& card) { m_cards.set(row, card); }
    void keepRows(const std::vector<std::uint32_t>& rows) { m_cards.keepRows(rows); }

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

private:
    CardRows<DesignShapeBasisCard> m_cards;
};

} // namespace oor::model
