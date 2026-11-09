#pragma once

#include "oor/model/cards/properties/viscousDamperProp/ViscousDamperPropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ViscousDamperPropFormat
{
    // The first thing wrong with the values of a card, if anything (used by readAll and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ViscousDamperPropCard& card);
    // One property, or two when the second group of three fields (from field 5) is there.
    [[nodiscard]] static std::vector<ViscousDamperPropCard> readAll(FieldReader& reader);
    static void write(const ViscousDamperPropCard& card, FieldWriter& writer);
};

// All the PVISC properties, one row each.
class ViscousDamperPropTable final : public TypedTable<ViscousDamperPropTable, ViscousDamperPropCard>
{
public:
    static constexpr std::string_view NAME = "ViscousDamperProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = ViscousDamperPropFormat;

    [[nodiscard]] static base::EntityId cardId(const ViscousDamperPropCard& card) noexcept { return card.id; }
    std::uint32_t append(const ViscousDamperPropCard& card, base::CardKey key);
    [[nodiscard]] ViscousDamperPropCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ViscousDamperPropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& extensionalDampingColumn() noexcept { return m_extensionalDamping; }
    [[nodiscard]] Column<double>& rotationalDampingColumn() noexcept { return m_rotationalDamping; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_extensionalDamping;
    Column<double> m_rotationalDamping;
};

} // namespace oor::model
