#pragma once

#include "oor/model/cards/properties/springProp/SpringPropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct SpringPropFormat
{
    // The first thing wrong with the values of a card, if anything (used by readAll and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SpringPropCard& card);
    // One property, or two when the second group of four fields is there.
    [[nodiscard]] static std::vector<SpringPropCard> readAll(FieldReader& reader);
    static void write(const SpringPropCard& card, FieldWriter& writer);
};

// All the PELAS properties, one row each.
class SpringPropTable final : public TypedTable<SpringPropTable, SpringPropCard>
{
public:
    static constexpr std::string_view NAME = "SpringProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = SpringPropFormat;

    [[nodiscard]] static base::EntityId cardId(const SpringPropCard& card) noexcept { return card.id; }
    std::uint32_t append(const SpringPropCard& card, base::CardKey key);
    [[nodiscard]] SpringPropCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const SpringPropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& stiffnessColumn() noexcept { return m_stiffness; }
    [[nodiscard]] Column<double>& structuralDampingColumn() noexcept { return m_structuralDamping; }
    [[nodiscard]] Column<double>& stressCoefficientColumn() noexcept { return m_stressCoefficient; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_stiffness;
    Column<double> m_structuralDamping;
    Column<double> m_stressCoefficient;
};

} // namespace oor::model
