#pragma once

#include "oor/model/cards/properties/bushProp/BushPropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct BushPropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const BushPropCard& card);
    [[nodiscard]] static std::optional<BushPropCard> read(FieldReader& reader);
    static void write(const BushPropCard& card, FieldWriter& writer);
};

// All the PBUSH cards. The stiffness, damping and structural damping are (N, 6) NumPy views, the recovery coefficients
// (SA, ST, EA, ET) an (N, 4) view and (ALPHA, TREF, COINL) an (N, 3) view.
class BushPropTable final : public TypedTable<BushPropTable, BushPropCard>
{
public:
    static constexpr std::string_view NAME = "BushProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = BushPropFormat;

    BushPropTable() : m_recovery({1.0, 1.0, 1.0, 1.0}) {}

    [[nodiscard]] static base::EntityId cardId(const BushPropCard& card) noexcept { return card.id; }
    std::uint32_t append(const BushPropCard& card, base::CardKey key);
    [[nodiscard]] BushPropCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const BushPropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::array<double, 6>>& stiffnessColumn() noexcept { return m_stiffness; }
    [[nodiscard]] Column<std::array<double, 6>>& dampingColumn() noexcept { return m_damping; }
    [[nodiscard]] Column<std::array<double, 6>>& structuralDampingColumn() noexcept { return m_structuralDamping; }
    [[nodiscard]] Column<std::array<double, 4>>& recoveryColumn() noexcept { return m_recovery; } // SA ST EA ET
    [[nodiscard]] Column<double>& massColumn() noexcept { return m_mass; }
    [[nodiscard]] Column<std::array<double, 3>>& thermalColumn() noexcept { return m_thermal; } // ALPHA TREF COINL

private:
    Column<base::EntityId> m_id;
    Column<std::array<double, 6>> m_stiffness;
    Column<std::array<double, 6>> m_damping;
    Column<std::array<double, 6>> m_structuralDamping;
    Column<std::array<double, 4>> m_recovery;
    Column<double> m_mass;
    Column<std::array<double, 3>> m_thermal;
};

} // namespace oor::model
