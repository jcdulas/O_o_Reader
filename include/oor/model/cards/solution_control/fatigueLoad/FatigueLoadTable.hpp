#pragma once

#include "oor/model/cards/solution_control/fatigueLoad/FatigueLoadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/references/ReferenceVisitor.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct FatigueLoadFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FatigueLoadCard& card);
    [[nodiscard]] static std::optional<FatigueLoadCard> read(FieldReader& reader);
    static void write(const FatigueLoadCard& card, FieldWriter& writer);
};

// What the TID of a card names, from its TYPE: a TABLFTG when TYPE is blank, a UDNAME for DB, a data acquisition file,
// the load history file word, PSD and STATIC, and nothing for CONST, SINE and NARROW (the field is ignored or blank) or
// when the field is blank.
[[nodiscard]] std::optional<ReferenceTarget> fatigueLoadTidTarget(const FatigueLoadCard& card) noexcept;

// All the FTGLOAD cards, in the category FatigueLoad that an FTGEVNT names (IdSpace::PerCardType). The TID is a
// FatigueTable or a UserDefinedName reference according to the TYPE (fatigueLoadTidTarget), the LCID of a PSD load a
// RandomSet. The cards are kept whole (CardRows) next to their ID: they have optional lines of any length, and the
// query engine reads the ID.
class FatigueLoadTable final : public TypedTable<FatigueLoadTable, FatigueLoadCard>
{
public:
    static constexpr std::string_view NAME = "FatigueLoad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::FatigueLoad, base::IdSpace::PerCardType};
    using Format = FatigueLoadFormat;

    [[nodiscard]] static base::EntityId cardId(const FatigueLoadCard& card) noexcept { return card.id; }
    std::uint32_t append(const FatigueLoadCard& card, base::CardKey key);
    [[nodiscard]] FatigueLoadCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const FatigueLoadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<FatigueLoadCard> m_cards;
};

} // namespace oor::model
