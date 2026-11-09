#pragma once

#include "oor/model/cards/thermal/freeConvectionProp/FreeConvectionPropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct FreeConvectionPropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FreeConvectionPropCard& card);
    [[nodiscard]] static std::optional<FreeConvectionPropCard> read(FieldReader& reader);
    static void write(const FreeConvectionPropCard& card, FieldWriter& writer);
};

// All the PCONV cards. The ID is the PCONID a CONV names, in a space of its own (category ConvectionProperty,
// IdSpace::PerCardType: a PCONVM may have the same number). The absent values are NaN.
class FreeConvectionPropTable final : public TypedTable<FreeConvectionPropTable, FreeConvectionPropCard>
{
public:
    static constexpr std::string_view NAME = "FreeConvectionProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ConvectionProperty, base::IdSpace::PerCardType};
    using Format = FreeConvectionPropFormat;

    FreeConvectionPropTable();

    [[nodiscard]] static base::EntityId cardId(const FreeConvectionPropCard& card) noexcept { return card.id; }
    std::uint32_t append(const FreeConvectionPropCard& card, base::CardKey key);
    [[nodiscard]] FreeConvectionPropCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const FreeConvectionPropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& matIdColumn() noexcept { return m_matId; }
    [[nodiscard]] Column<std::int32_t>& matrixFormColumn() noexcept { return m_matrixForm; }
    [[nodiscard]] Column<double>& convectionExponentColumn() noexcept { return m_convectionExponent; }
    [[nodiscard]] Column<std::int32_t>& formulaKindColumn() noexcept { return m_formulaKind; }
    [[nodiscard]] Column<base::EntityId>& tableIdColumn() noexcept { return m_tableId; }
    [[nodiscard]] Column<double>& characteristicLengthColumn() noexcept { return m_characteristicLength; }
    [[nodiscard]] Column<base::EntityId>& inletNodeColumn() noexcept { return m_inletNode; }
    [[nodiscard]] Column<base::EntityId>& directionFrameIdColumn() noexcept { return m_directionFrameId; }
    [[nodiscard]] Column<double>& direction1Column() noexcept { return m_direction1; }
    [[nodiscard]] Column<double>& direction2Column() noexcept { return m_direction2; }
    [[nodiscard]] Column<double>& direction3Column() noexcept { return m_direction3; }
    [[nodiscard]] Column<std::array<double, 8>>& convectionCoefficientColumn() noexcept
    {
        return m_convectionCoefficient;
    }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_matId;
    Column<std::int32_t> m_matrixForm;
    Column<double> m_convectionExponent;
    Column<std::int32_t> m_formulaKind;
    Column<base::EntityId> m_tableId;
    Column<double> m_characteristicLength;
    Column<base::EntityId> m_inletNode;
    Column<base::EntityId> m_directionFrameId;
    Column<double> m_direction1;
    Column<double> m_direction2;
    Column<double> m_direction3;
    Column<std::array<double, 8>> m_convectionCoefficient;
};

} // namespace oor::model
