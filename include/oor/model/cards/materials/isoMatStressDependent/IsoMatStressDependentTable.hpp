#pragma once

#include "oor/model/cards/materials/isoMatStressDependent/IsoMatStressDependentCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string_view>
#include <vector>

namespace oor::model
{

struct IsoMatStressDependentFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const IsoMatStressDependentCard& card);
    [[nodiscard]] static std::optional<IsoMatStressDependentCard> read(FieldReader& reader);
    static void write(const IsoMatStressDependentCard& card, FieldWriter& writer);

    // NLELAST or PLASTIC, and the type of a word in any case (nothing for another word).
    [[nodiscard]] static const char* typeWord(IsoMatStressDependentType type) noexcept;
    [[nodiscard]] static std::optional<IsoMatStressDependentType> parseType(std::string_view text) noexcept;
};

// All the MATS1 cards. The ID space is the card type's own: the ID is the one of the MAT1 that the card extends, so it
// is also a reference to a material (renumbering the materials follows into it), and the table is a reference to a
// TABLES1 or TABLEST (ReferenceTarget::StressTable). H, LIMIT1 and LIMIT2 are NaN for a card that has none.
class IsoMatStressDependentTable final : public TypedTable<IsoMatStressDependentTable, IsoMatStressDependentCard>
{
public:
    static constexpr std::string_view NAME = "IsoMatStressDependent";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::PerCardType};
    using Format = IsoMatStressDependentFormat;

    IsoMatStressDependentTable();

    [[nodiscard]] static base::EntityId cardId(const IsoMatStressDependentCard& card) noexcept { return card.id; }
    std::uint32_t append(const IsoMatStressDependentCard& card, base::CardKey key);
    [[nodiscard]] IsoMatStressDependentCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const IsoMatStressDependentCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& tableIdColumn() noexcept { return m_tableId; }
    [[nodiscard]] Column<std::uint8_t>& typeColumn() noexcept { return m_type; }
    [[nodiscard]] Column<double>& hColumn() noexcept { return m_h; }
    [[nodiscard]] Column<std::int32_t>& yieldFunctionColumn() noexcept { return m_yieldFunction; }
    [[nodiscard]] Column<std::int32_t>& hardeningRuleColumn() noexcept { return m_hardeningRule; }
    [[nodiscard]] Column<double>& initialYieldStressColumn() noexcept { return m_initialYieldStress; }
    [[nodiscard]] Column<double>& frictionAngleColumn() noexcept { return m_frictionAngle; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_tableId;
    Column<std::uint8_t> m_type;
    Column<double> m_h;
    Column<std::int32_t> m_yieldFunction;
    Column<std::int32_t> m_hardeningRule;
    Column<double> m_initialYieldStress;
    Column<double> m_frictionAngle;
};

} // namespace oor::model
