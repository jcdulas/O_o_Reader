#pragma once

#include "oor/model/cards/elements/damperScalarProperty/DamperScalarPropertyCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct DamperScalarPropertyFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DamperScalarPropertyCard& card);
    [[nodiscard]] static std::optional<DamperScalarPropertyCard> read(FieldReader& reader);
    static void write(const DamperScalarPropertyCard& card, FieldWriter& writer);
};

// All the CDAMP3 elements.
class DamperScalarPropertyTable final : public TypedTable<DamperScalarPropertyTable, DamperScalarPropertyCard>
{
public:
    static constexpr std::string_view NAME = "DamperScalarProperty";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = DamperScalarPropertyFormat;

    DamperScalarPropertyTable() : m_scalarPoint1(0), m_scalarPoint2(0) {}

    [[nodiscard]] static base::EntityId cardId(const DamperScalarPropertyCard& card) noexcept { return card.id; }
    std::uint32_t append(const DamperScalarPropertyCard& card, base::CardKey key);
    [[nodiscard]] DamperScalarPropertyCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const DamperScalarPropertyCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<base::EntityId>& scalarPoint1Column() noexcept { return m_scalarPoint1; }
    [[nodiscard]] Column<base::EntityId>& scalarPoint2Column() noexcept { return m_scalarPoint2; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<base::EntityId> m_scalarPoint1;
    Column<base::EntityId> m_scalarPoint2;
};

} // namespace oor::model
