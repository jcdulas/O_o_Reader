#pragma once

#include "oor/model/cards/loads/beamDistributedLoad/BeamDistributedLoadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string_view>
#include <vector>

namespace oor::model
{

struct BeamDistributedLoadFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const BeamDistributedLoadCard& card);
    [[nodiscard]] static std::optional<BeamDistributedLoadCard> read(FieldReader& reader);
    static void write(const BeamDistributedLoadCard& card, FieldWriter& writer);

    // The words of the TYPE and SCALE fields.
    [[nodiscard]] static std::string_view typeName(BarLoadType type) noexcept;
    [[nodiscard]] static std::optional<BarLoadType> typeFromName(std::string_view name) noexcept;
    [[nodiscard]] static std::string_view scaleName(BarLoadScale scale) noexcept;
    [[nodiscard]] static std::optional<BarLoadScale> scaleFromName(std::string_view name) noexcept;
};

// All the PLOAD1 cards. The ID is a load set ID shared by many cards (IdSpace::Group).
class BeamDistributedLoadTable final : public TypedTable<BeamDistributedLoadTable, BeamDistributedLoadCard>
{
public:
    static constexpr std::string_view NAME = "BeamDistributedLoad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = BeamDistributedLoadFormat;

    [[nodiscard]] static base::EntityId cardId(const BeamDistributedLoadCard& card) noexcept { return card.id; }
    std::uint32_t append(const BeamDistributedLoadCard& card, base::CardKey key);
    [[nodiscard]] BeamDistributedLoadCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const BeamDistributedLoadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& elemIdColumn() noexcept { return m_elemId; }
    [[nodiscard]] Column<std::uint8_t>& kindColumn() noexcept { return m_kind; }
    [[nodiscard]] Column<std::uint8_t>& scaleFactorColumn() noexcept { return m_scaleFactor; }
    [[nodiscard]] Column<double>& xCoord1Column() noexcept { return m_xCoord1; }
    [[nodiscard]] Column<double>& pressure1Column() noexcept { return m_pressure1; }
    [[nodiscard]] Column<double>& xCoord2Column() noexcept { return m_xCoord2; }
    // NaN for a blank P2 (BlankReal.hpp).
    [[nodiscard]] Column<double>& pressure2Column() noexcept { return m_pressure2; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_elemId;
    Column<std::uint8_t> m_kind;
    Column<std::uint8_t> m_scaleFactor;
    Column<double> m_xCoord1;
    Column<double> m_pressure1;
    Column<double> m_xCoord2;
    Column<double> m_pressure2;
};

} // namespace oor::model
