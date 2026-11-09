#pragma once

#include "oor/model/cards/dynamics/frfConnection/FrfConnectionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct FrfConnectionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FrfConnectionCard& card);
    [[nodiscard]] static std::optional<FrfConnectionCard> read(FieldReader& reader);
    static void write(const FrfConnectionCard& card, FieldWriter& writer);
};

// All the FRFCONN cards. The CONNID is unique among them (IdSpace::PerCardType, category Other). A component given by
// its ID is a reference to an FRFCOMP (ReferenceTarget::FrfComponent); one given by its name is not an ID and is kept
// as written. The points are plain numbers. The cards are kept whole next to a column of their IDs.
class FrfConnectionTable final : public TypedTable<FrfConnectionTable, FrfConnectionCard>
{
public:
    static constexpr std::string_view NAME = "FrfConnection";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = FrfConnectionFormat;

    [[nodiscard]] static base::EntityId cardId(const FrfConnectionCard& card) noexcept { return card.id; }
    std::uint32_t append(const FrfConnectionCard& card, base::CardKey key);
    [[nodiscard]] FrfConnectionCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const FrfConnectionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<FrfConnectionCard> m_cards;
};

} // namespace oor::model
