#pragma once

#include "oor/model/cards/properties/damperPropFreq/DamperPropFreqCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct DamperPropFreqFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DamperPropFreqCard& card);
    [[nodiscard]] static std::optional<DamperPropFreqCard> read(FieldReader& reader);
    static void write(const DamperPropFreqCard& card, FieldWriter& writer);
};

// All the PDAMPT cards. The ID space is the card type's own: the ID is the one of the PDAMP that the card extends, so
// it is also a reference to a property.
class DamperPropFreqTable final : public TypedTable<DamperPropFreqTable, DamperPropFreqCard>
{
public:
    static constexpr std::string_view NAME = "DamperPropFreq";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::PerCardType};
    using Format = DamperPropFreqFormat;

    [[nodiscard]] static base::EntityId cardId(const DamperPropFreqCard& card) noexcept { return card.id; }
    std::uint32_t append(const DamperPropFreqCard& card, base::CardKey key);
    [[nodiscard]] DamperPropFreqCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const DamperPropFreqCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& dampingTableIdColumn() noexcept { return m_dampingTableId; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_dampingTableId;
};

} // namespace oor::model
