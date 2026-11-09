#pragma once

#include "oor/model/cards/thermal/acousticBarrierProp/AcousticBarrierPropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AcousticBarrierPropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AcousticBarrierPropCard& card);
    [[nodiscard]] static std::optional<AcousticBarrierPropCard> read(FieldReader& reader);
    static void write(const AcousticBarrierPropCard& card, FieldWriter& writer);
};

// All the PACBAR cards. The ID is a property ID, in the space every property shares (IdSpace::Property: a CHACBR names
// it as its PID). The absent values are NaN.
class AcousticBarrierPropTable final : public TypedTable<AcousticBarrierPropTable, AcousticBarrierPropCard>
{
public:
    static constexpr std::string_view NAME = "AcousticBarrierProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = AcousticBarrierPropFormat;

    AcousticBarrierPropTable();

    [[nodiscard]] static base::EntityId cardId(const AcousticBarrierPropCard& card) noexcept { return card.id; }
    std::uint32_t append(const AcousticBarrierPropCard& card, base::CardKey key);
    [[nodiscard]] AcousticBarrierPropCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AcousticBarrierPropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a PACBAR refers to no card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& backingMassColumn() noexcept { return m_backingMass; }
    [[nodiscard]] Column<double>& septumMassColumn() noexcept { return m_septumMass; }
    [[nodiscard]] Column<double>& resonanceFrequencyColumn() noexcept { return m_resonanceFrequency; }
    [[nodiscard]] Column<double>& resonanceStiffnessColumn() noexcept { return m_resonanceStiffness; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_backingMass;
    Column<double> m_septumMass;
    Column<double> m_resonanceFrequency;
    Column<double> m_resonanceStiffness;
};

} // namespace oor::model
