#pragma once

#include "oor/model/cards/thermal/forcedConvectionProp/ForcedConvectionPropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ForcedConvectionPropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ForcedConvectionPropCard& card);
    [[nodiscard]] static std::optional<ForcedConvectionPropCard> read(FieldReader& reader);
    static void write(const ForcedConvectionPropCard& card, FieldWriter& writer);
};

// All the PCONVM cards. The ID is the PCONID a CONVM names, in a space of its own (category ForcedConvectionProperty,
// IdSpace::PerCardType).
class ForcedConvectionPropTable final : public TypedTable<ForcedConvectionPropTable, ForcedConvectionPropCard>
{
public:
    static constexpr std::string_view NAME = "ForcedConvectionProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ForcedConvectionProperty, base::IdSpace::PerCardType};
    using Format = ForcedConvectionPropFormat;

    [[nodiscard]] static base::EntityId cardId(const ForcedConvectionPropCard& card) noexcept { return card.id; }
    std::uint32_t append(const ForcedConvectionPropCard& card, base::CardKey key);
    [[nodiscard]] ForcedConvectionPropCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ForcedConvectionPropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& matIdColumn() noexcept { return m_matId; }
    [[nodiscard]] Column<std::int32_t>& matrixFormColumn() noexcept { return m_matrixForm; }
    [[nodiscard]] Column<std::int32_t>& massFlowConvectionColumn() noexcept { return m_massFlowConvection; }
    [[nodiscard]] Column<double>& coefficientColumn() noexcept { return m_coefficient; }
    [[nodiscard]] Column<double>& reynoldsExponentColumn() noexcept { return m_reynoldsExponent; }
    [[nodiscard]] Column<double>& prandtlExponentInnerColumn() noexcept { return m_prandtlExponentInner; }
    [[nodiscard]] Column<double>& prandtlExponentOuterColumn() noexcept { return m_prandtlExponentOuter; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_matId;
    Column<std::int32_t> m_matrixForm;
    Column<std::int32_t> m_massFlowConvection;
    Column<double> m_coefficient;
    Column<double> m_reynoldsExponent;
    Column<double> m_prandtlExponentInner;
    Column<double> m_prandtlExponentOuter;
};

} // namespace oor::model
