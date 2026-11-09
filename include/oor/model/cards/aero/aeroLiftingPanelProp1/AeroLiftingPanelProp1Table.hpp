#pragma once

#include "oor/model/cards/aero/aeroLiftingPanelProp1/AeroLiftingPanelProp1Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

struct AeroLiftingPanelProp1Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroLiftingPanelProp1Card& card);
    [[nodiscard]] static std::optional<AeroLiftingPanelProp1Card> read(FieldReader& reader);
    static void write(const AeroLiftingPanelProp1Card& card, FieldWriter& writer);
};

// All the PAERO1 cards. The PID is in the space the five PAERO cards share (IdSpace::AeroProperty, category
// AeroProperty): a CAERO names it (ReferenceTarget::AeroProperty). Each body is the EID of a CAERO2
// (ReferenceTarget::AeroElement: the type of the card it names is a rule between cards).
class AeroLiftingPanelProp1Table final : public TypedTable<AeroLiftingPanelProp1Table, AeroLiftingPanelProp1Card>
{
public:
    static constexpr std::string_view NAME = "AeroLiftingPanelProp1";
    static inline const CardTypeInfo INFO{NAME, CardCategory::AeroProperty, base::IdSpace::AeroProperty};
    using Format = AeroLiftingPanelProp1Format;

    [[nodiscard]] static base::EntityId cardId(const AeroLiftingPanelProp1Card& card) noexcept { return card.id; }
    std::uint32_t append(const AeroLiftingPanelProp1Card& card, base::CardKey key);
    [[nodiscard]] AeroLiftingPanelProp1Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const AeroLiftingPanelProp1Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::array<base::EntityId, AeroLiftingPanelProp1Card::BODY_COUNT>>& bodiesColumn() noexcept
    {
        return m_bodies;
    }

private:
    Column<base::EntityId> m_id;
    Column<std::array<base::EntityId, AeroLiftingPanelProp1Card::BODY_COUNT>> m_bodies;
};

} // namespace oor::model
