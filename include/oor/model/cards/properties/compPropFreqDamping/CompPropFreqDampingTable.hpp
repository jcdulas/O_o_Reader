#pragma once

#include "oor/model/cards/properties/compPropFreqDamping/CompPropFreqDampingCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct CompPropFreqDampingFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CompPropFreqDampingCard& card);
    [[nodiscard]] static std::optional<CompPropFreqDampingCard> read(FieldReader& reader);
    static void write(const CompPropFreqDampingCard& card, FieldWriter& writer);
};

// All the PCOMPFQ cards. The ID space is the card type's own: the ID is the one of the PCOMP or PCOMPG that the card
// extends, so it is also a reference to a property.
class CompPropFreqDampingTable final : public TypedTable<CompPropFreqDampingTable, CompPropFreqDampingCard>
{
public:
    static constexpr std::string_view NAME = "CompPropFreqDamping";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::PerCardType};
    using Format = CompPropFreqDampingFormat;

    [[nodiscard]] static base::EntityId cardId(const CompPropFreqDampingCard& card) noexcept { return card.id; }
    std::uint32_t append(const CompPropFreqDampingCard& card, base::CardKey key);
    [[nodiscard]] CompPropFreqDampingCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const CompPropFreqDampingCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& structuralDampingColumn() noexcept { return m_structuralDamping; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_structuralDamping;
};

} // namespace oor::model
