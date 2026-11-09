#pragma once

#include "oor/model/cards/properties/viscousDamperPropNonlinear/ViscousDamperPropNonlinearCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ViscousDamperPropNonlinearFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ViscousDamperPropNonlinearCard& card);
    [[nodiscard]] static std::optional<ViscousDamperPropNonlinearCard> read(FieldReader& reader);
    static void write(const ViscousDamperPropNonlinearCard& card, FieldWriter& writer);
};

// All the PVISC1 properties, kept in columns (IdSpace::Property, shared with every other property).
class ViscousDamperPropNonlinearTable final
    : public TypedTable<ViscousDamperPropNonlinearTable, ViscousDamperPropNonlinearCard>
{
public:
    static constexpr std::string_view NAME = "ViscousDamperPropNonlinear";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = ViscousDamperPropNonlinearFormat;

    [[nodiscard]] static base::EntityId cardId(const ViscousDamperPropNonlinearCard& card) noexcept { return card.id; }
    std::uint32_t append(const ViscousDamperPropNonlinearCard& card, base::CardKey key);
    [[nodiscard]] ViscousDamperPropNonlinearCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ViscousDamperPropNonlinearCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& tableIdColumn() noexcept { return m_tableId; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_tableId;
};

} // namespace oor::model
