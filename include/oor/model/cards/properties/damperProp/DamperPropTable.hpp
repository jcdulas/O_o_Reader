#pragma once

#include "oor/model/cards/properties/damperProp/DamperPropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct DamperPropFormat
{
    // The first thing wrong with the values of a card, if anything (used by readAll and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DamperPropCard& card);
    // One property, or up to four when the later pairs of fields are there.
    [[nodiscard]] static std::vector<DamperPropCard> readAll(FieldReader& reader);
    static void write(const DamperPropCard& card, FieldWriter& writer);
};

// All the PDAMP properties, one row each.
class DamperPropTable final : public TypedTable<DamperPropTable, DamperPropCard>
{
public:
    static constexpr std::string_view NAME = "DamperProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = DamperPropFormat;

    [[nodiscard]] static base::EntityId cardId(const DamperPropCard& card) noexcept { return card.id; }
    std::uint32_t append(const DamperPropCard& card, base::CardKey key);
    [[nodiscard]] DamperPropCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const DamperPropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& dampingColumn() noexcept { return m_damping; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_damping;
};

} // namespace oor::model
