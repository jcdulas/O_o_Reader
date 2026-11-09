#pragma once

#include "oor/model/cards/elements/crackTipProp3D/CrackTipProp3DCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct CrackTipProp3DFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CrackTipProp3DCard& card);
    [[nodiscard]] static std::optional<CrackTipProp3DCard> read(FieldReader& reader);
    static void write(const CrackTipProp3DCard& card, FieldWriter& writer);
};

// All the PRAC3D properties. The ID is a property ID, in the space every property shares; MID names a material.
class CrackTipProp3DTable final : public TypedTable<CrackTipProp3DTable, CrackTipProp3DCard>
{
public:
    static constexpr std::string_view NAME = "CrackTipProp3D";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = CrackTipProp3DFormat;

    CrackTipProp3DTable()
        : m_gammaCoefficient(CrackTipProp3DCard::DEFAULT_GAMMA), m_phiAngle(CrackTipProp3DCard::DEFAULT_PHI)
    {
    }

    [[nodiscard]] static base::EntityId cardId(const CrackTipProp3DCard& card) noexcept { return card.id; }
    std::uint32_t append(const CrackTipProp3DCard& card, base::CardKey key);
    [[nodiscard]] CrackTipProp3DCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const CrackTipProp3DCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& matIdColumn() noexcept { return m_matId; }
    [[nodiscard]] Column<double>& gammaCoefficientColumn() noexcept { return m_gammaCoefficient; }
    [[nodiscard]] Column<double>& phiAngleColumn() noexcept { return m_phiAngle; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_matId;
    Column<double> m_gammaCoefficient;
    Column<double> m_phiAngle;
};

} // namespace oor::model
