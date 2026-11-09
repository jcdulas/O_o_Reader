#pragma once

#include "oor/model/cards/elements/acousticInfiniteProp/AcousticInfinitePropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AcousticInfinitePropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AcousticInfinitePropCard& card);
    [[nodiscard]] static std::optional<AcousticInfinitePropCard> read(FieldReader& reader);
    static void write(const AcousticInfinitePropCard& card, FieldWriter& writer);
};

// All the PACINF properties: the ID is in the space every property type shares (IdSpace::Property), so the PID of a
// CACINF3 or CACINF4 finds it; the MID is a material (a MAT10).
class AcousticInfinitePropTable final : public TypedTable<AcousticInfinitePropTable, AcousticInfinitePropCard>
{
public:
    static constexpr std::string_view NAME = "AcousticInfiniteProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = AcousticInfinitePropFormat;

    [[nodiscard]] static base::EntityId cardId(const AcousticInfinitePropCard& card) noexcept { return card.id; }
    std::uint32_t append(const AcousticInfinitePropCard& card, base::CardKey key);
    [[nodiscard]] AcousticInfinitePropCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AcousticInfinitePropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& matIdColumn() noexcept { return m_matId; }
    [[nodiscard]] Column<std::int32_t>& radialOrderColumn() noexcept { return m_radialOrder; }
    [[nodiscard]] Column<double>& poleXColumn() noexcept { return m_poleX; }
    [[nodiscard]] Column<double>& poleYColumn() noexcept { return m_poleY; }
    [[nodiscard]] Column<double>& poleZColumn() noexcept { return m_poleZ; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_matId;
    Column<std::int32_t> m_radialOrder;
    Column<double> m_poleX;
    Column<double> m_poleY;
    Column<double> m_poleZ;
};

} // namespace oor::model
