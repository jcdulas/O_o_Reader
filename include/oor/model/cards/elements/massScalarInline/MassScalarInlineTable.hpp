#pragma once

#include "oor/model/cards/elements/massScalarInline/MassScalarInlineCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct MassScalarInlineFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const MassScalarInlineCard& card);
    // One element, or two when the second group of four fields is there.
    [[nodiscard]] static std::vector<MassScalarInlineCard> readAll(FieldReader& reader);
    static void write(const MassScalarInlineCard& card, FieldWriter& writer);
};

// All the CMASS4 elements.
class MassScalarInlineTable final : public TypedTable<MassScalarInlineTable, MassScalarInlineCard>
{
public:
    static constexpr std::string_view NAME = "MassScalarInline";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = MassScalarInlineFormat;

    MassScalarInlineTable() : m_scalarPoint1(0), m_scalarPoint2(0) {}

    [[nodiscard]] static base::EntityId cardId(const MassScalarInlineCard& card) noexcept { return card.id; }
    std::uint32_t append(const MassScalarInlineCard& card, base::CardKey key);
    [[nodiscard]] MassScalarInlineCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const MassScalarInlineCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& massColumn() noexcept { return m_mass; }
    [[nodiscard]] Column<base::EntityId>& scalarPoint1Column() noexcept { return m_scalarPoint1; }
    [[nodiscard]] Column<base::EntityId>& scalarPoint2Column() noexcept { return m_scalarPoint2; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_mass;
    Column<base::EntityId> m_scalarPoint1;
    Column<base::EntityId> m_scalarPoint2;
};

} // namespace oor::model
