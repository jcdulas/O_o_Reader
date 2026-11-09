#pragma once

#include "oor/model/cards/properties/bushProp1D/BushProp1DCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct BushProp1DFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const BushProp1DCard& card);
    [[nodiscard]] static std::optional<BushProp1DCard> read(FieldReader& reader);
    static void write(const BushProp1DCard& card, FieldWriter& writer);
};

// All the PBUSH1D cards. The continuations are columns of their own (a bit per continuation that the card has, then the
// definition type and the values of each); a continuation that a card does not have holds zeros.
class BushProp1DTable final : public TypedTable<BushProp1DTable, BushProp1DCard>
{
public:
    static constexpr std::string_view NAME = "BushProp1D";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = BushProp1DFormat;

    // The bits of the `groups` column.
    static constexpr std::uint8_t HAS_SHOCKA = 1;
    static constexpr std::uint8_t HAS_SPRING = 2;
    static constexpr std::uint8_t HAS_DAMPER = 4;
    static constexpr std::uint8_t HAS_GENER = 8;

    BushProp1DTable() : m_shock({0.0, 0.0, 1.0, 1.0}) {}

    [[nodiscard]] static base::EntityId cardId(const BushProp1DCard& card) noexcept { return card.id; }
    std::uint32_t append(const BushProp1DCard& card, base::CardKey key);
    [[nodiscard]] BushProp1DCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const BushProp1DCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& stiffnessColumn() noexcept { return m_stiffness; }
    [[nodiscard]] Column<double>& dampingColumn() noexcept { return m_damping; }
    [[nodiscard]] Column<double>& massColumn() noexcept { return m_mass; }
    [[nodiscard]] Column<double>& stressFactorColumn() noexcept { return m_stressFactor; }
    [[nodiscard]] Column<double>& strainFactorColumn() noexcept { return m_strainFactor; }
    [[nodiscard]] Column<std::uint8_t>& groupsColumn() noexcept { return m_groups; }
    [[nodiscard]] Column<std::uint8_t>& shockTypeColumn() noexcept { return m_shockType; }
    [[nodiscard]] Column<std::array<double, 4>>& shockColumn() noexcept { return m_shock; }
    [[nodiscard]] Column<std::array<base::EntityId, 5>>& shockIdsColumn() noexcept { return m_shockIds; }
    [[nodiscard]] Column<std::uint8_t>& springTypeColumn() noexcept { return m_springType; }
    [[nodiscard]] Column<std::array<base::EntityId, 4>>& springIdsColumn() noexcept { return m_springIds; }
    [[nodiscard]] Column<std::uint8_t>& damperTypeColumn() noexcept { return m_damperType; }
    [[nodiscard]] Column<std::array<base::EntityId, 4>>& damperIdsColumn() noexcept { return m_damperIds; }
    [[nodiscard]] Column<std::array<base::EntityId, 6>>& generalIdsColumn() noexcept { return m_generalIds; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_stiffness;
    Column<double> m_damping;
    Column<double> m_mass;
    Column<double> m_stressFactor;
    Column<double> m_strainFactor;
    Column<std::uint8_t> m_groups;
    Column<std::uint8_t> m_shockType;
    Column<std::array<double, 4>> m_shock;            // CVT, CVC, EXPVT, EXPVC
    Column<std::array<base::EntityId, 5>> m_shockIds; // IDTS, IDETS, IDECS, IDETSD, IDECSD
    Column<std::uint8_t> m_springType;
    Column<std::array<base::EntityId, 4>> m_springIds; // IDT, IDC, IDTDU, IDCDU
    Column<std::uint8_t> m_damperType;
    Column<std::array<base::EntityId, 4>> m_damperIds;  // IDT, IDC, IDTDV, IDCDV
    Column<std::array<base::EntityId, 6>> m_generalIds; // IDT, IDC, IDTDU, IDCDU, IDTDV, IDCDV
};

} // namespace oor::model
