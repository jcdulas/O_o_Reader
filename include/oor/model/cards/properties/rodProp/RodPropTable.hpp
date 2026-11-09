#pragma once

#include "oor/model/cards/properties/rodProp/RodPropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct RodPropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RodPropCard& card);
    [[nodiscard]] static std::optional<RodPropCard> read(FieldReader& reader);
    static void write(const RodPropCard& card, FieldWriter& writer);
};

class RodPropTable final : public TypedTable<RodPropTable, RodPropCard>
{
public:
    static constexpr std::string_view NAME = "RodProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = RodPropFormat;

    [[nodiscard]] static base::EntityId cardId(const RodPropCard& card) noexcept { return card.id; }
    std::uint32_t append(const RodPropCard& card, base::CardKey key);
    [[nodiscard]] RodPropCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const RodPropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& matIdColumn() noexcept { return m_matId; }
    [[nodiscard]] Column<double>& areaColumn() noexcept { return m_area; }
    [[nodiscard]] Column<double>& torsionColumn() noexcept { return m_torsion; }
    [[nodiscard]] Column<double>& stressCoefficientColumn() noexcept { return m_stressCoefficient; }
    [[nodiscard]] Column<double>& nonStructMassColumn() noexcept { return m_nonStructMass; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_matId;
    Column<double> m_area;
    Column<double> m_torsion;
    Column<double> m_stressCoefficient;
    Column<double> m_nonStructMass;
};

} // namespace oor::model
