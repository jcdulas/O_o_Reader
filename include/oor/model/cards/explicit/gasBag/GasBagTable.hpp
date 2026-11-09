#pragma once

#include "oor/model/cards/explicit/gasBag/GasBagCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct GasBagFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const GasBagCard& card);
    [[nodiscard]] static std::optional<GasBagCard> read(FieldReader& reader);
    static void write(const GasBagCard& card, FieldWriter& writer);
};

// All the GBAG cards. The GID is unique among the gas bags (IdSpace::PerCardType, category GasBag: the check of the
// category finds the GID that two cards of other types share). The card names a surface, a set of leakages, a set of
// inflators and an initial gas composition; the set of heat losses is a plain ID.
class GasBagTable final : public TypedTable<GasBagTable, GasBagCard>
{
public:
    static constexpr std::string_view NAME = "GasBag";
    static inline const CardTypeInfo INFO{NAME, CardCategory::GasBag, base::IdSpace::PerCardType};
    using Format = GasBagFormat;

    [[nodiscard]] static base::EntityId cardId(const GasBagCard& card) noexcept { return card.id; }
    std::uint32_t append(const GasBagCard& card, base::CardKey key);
    [[nodiscard]] GasBagCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const GasBagCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<GasBagCard> m_cards;
};

} // namespace oor::model
