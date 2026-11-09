#pragma once

#include "oor/model/cards/dynamics/frfReleasedDofs/FrfReleasedDofsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct FrfReleasedDofsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FrfReleasedDofsCard& card);
    [[nodiscard]] static std::optional<FrfReleasedDofsCard> read(FieldReader& reader);
    static void write(const FrfReleasedDofsCard& card, FieldWriter& writer);
};

// All the FRFRELS cards. The SID is a set ID that several cards may share (IdSpace::Group, category Other). A component
// given by its ID is a reference to an FRFCOMP (ReferenceTarget::FrfComponent), one given by its name is kept as
// written; the grid points are plain numbers of the components' own models. The cards are kept whole next to columns of
// their IDs and component digits.
class FrfReleasedDofsTable final : public TypedTable<FrfReleasedDofsTable, FrfReleasedDofsCard>
{
public:
    static constexpr std::string_view NAME = "FrfReleasedDofs";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::Group};
    using Format = FrfReleasedDofsFormat;

    [[nodiscard]] static base::EntityId cardId(const FrfReleasedDofsCard& card) noexcept { return card.id; }
    std::uint32_t append(const FrfReleasedDofsCard& card, base::CardKey key);
    [[nodiscard]] FrfReleasedDofsCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const FrfReleasedDofsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::int32_t>& componentsColumn() noexcept { return m_components; }

private:
    Column<base::EntityId> m_id;
    Column<std::int32_t> m_components;
    CardRows<FrfReleasedDofsCard> m_cards;
};

} // namespace oor::model
