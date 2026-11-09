#pragma once

#include "oor/model/cards/properties/springPropFreq/SpringPropFreqCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct SpringPropFreqFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SpringPropFreqCard& card);
    [[nodiscard]] static std::optional<SpringPropFreqCard> read(FieldReader& reader);
    static void write(const SpringPropFreqCard& card, FieldWriter& writer);
};

// All the PELAST cards. The ID space is the card type's own: the ID is the one of the PELAS that the card extends, so
// it is also a reference to a property.
class SpringPropFreqTable final : public TypedTable<SpringPropFreqTable, SpringPropFreqCard>
{
public:
    static constexpr std::string_view NAME = "SpringPropFreq";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::PerCardType};
    using Format = SpringPropFreqFormat;

    [[nodiscard]] static base::EntityId cardId(const SpringPropFreqCard& card) noexcept { return card.id; }
    std::uint32_t append(const SpringPropFreqCard& card, base::CardKey key);
    [[nodiscard]] SpringPropFreqCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const SpringPropFreqCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& stiffnessTableIdColumn() noexcept { return m_stiffnessTableId; }
    [[nodiscard]] Column<base::EntityId>& structuralDampingTableIdColumn() noexcept
    {
        return m_structuralDampingTableId;
    }
    [[nodiscard]] Column<base::EntityId>& nonlinearForceTableIdColumn() noexcept { return m_nonlinearForceTableId; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_stiffnessTableId;
    Column<base::EntityId> m_structuralDampingTableId;
    Column<base::EntityId> m_nonlinearForceTableId;
};

} // namespace oor::model
