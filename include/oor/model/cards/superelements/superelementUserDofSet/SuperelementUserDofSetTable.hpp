#pragma once

#include "oor/model/cards/superelements/superelementUserDofSet/SuperelementUserDofSetCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct SuperelementUserDofSetFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SuperelementUserDofSetCard& card);
    [[nodiscard]] static std::optional<SuperelementUserDofSetCard> read(FieldReader& reader);
    static void write(const SuperelementUserDofSetCard& card, FieldWriter& writer);
};

// All the SEUSET cards, category Superelement, no ID of their own (IdSpace::None). They are kept whole next to columns
// of their SEIDs and set names.
class SuperelementUserDofSetTable final : public TypedTable<SuperelementUserDofSetTable, SuperelementUserDofSetCard>
{
public:
    static constexpr std::string_view NAME = "SuperelementUserDofSet";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Superelement, base::IdSpace::None};
    using Format = SuperelementUserDofSetFormat;

    [[nodiscard]] static base::EntityId cardId(const SuperelementUserDofSetCard&) noexcept { return 0; }
    std::uint32_t append(const SuperelementUserDofSetCard& card, base::CardKey key);
    [[nodiscard]] SuperelementUserDofSetCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const SuperelementUserDofSetCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    // SEID is a superelement and the points are those of it.
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& superelementIdColumn() noexcept { return m_superelementId; }
    [[nodiscard]] Column<std::string>& setNameColumn() noexcept { return m_setName; }

private:
    Column<base::EntityId> m_superelementId;
    Column<std::string> m_setName;
    CardRows<SuperelementUserDofSetCard> m_cards;
};

} // namespace oor::model
