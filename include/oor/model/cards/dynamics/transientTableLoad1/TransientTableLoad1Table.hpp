#pragma once

#include "oor/model/cards/dynamics/transientTableLoad1/TransientTableLoad1Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct TransientTableLoad1Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const TransientTableLoad1Card& card);
    [[nodiscard]] static std::optional<TransientTableLoad1Card> read(FieldReader& reader);
    static void write(const TransientTableLoad1Card& card, FieldWriter& writer);
};

// All the TLOAD1 cards. The ID is a dynamic load set ID shared by the RLOAD, TLOAD and DAREA cards (IdSpace::Group).
class TransientTableLoad1Table final : public TypedTable<TransientTableLoad1Table, TransientTableLoad1Card>
{
public:
    static constexpr std::string_view NAME = "TransientTableLoad1";
    static inline const CardTypeInfo INFO{NAME, CardCategory::DynamicLoad, base::IdSpace::Group};
    using Format = TransientTableLoad1Format;

    [[nodiscard]] static base::EntityId cardId(const TransientTableLoad1Card& card) noexcept { return card.id; }
    std::uint32_t append(const TransientTableLoad1Card& card, base::CardKey key);
    [[nodiscard]] TransientTableLoad1Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const TransientTableLoad1Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& excitationIdColumn() noexcept { return m_excitationId; }
    [[nodiscard]] Column<base::EntityId>& delayIdColumn() noexcept { return m_delayId; }
    [[nodiscard]] Column<double>& delayColumn() noexcept { return m_delay; }
    [[nodiscard]] Column<std::uint8_t>& kindColumn() noexcept { return m_kind; }
    [[nodiscard]] Column<base::EntityId>& tableIdColumn() noexcept { return m_tableId; }
    [[nodiscard]] Column<double>& valueColumn() noexcept { return m_value; }
    [[nodiscard]] Column<double>& initialDisplacementFactorColumn() noexcept { return m_initialDisplacementFactor; }
    [[nodiscard]] Column<double>& initialVelocityFactorColumn() noexcept { return m_initialVelocityFactor; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_excitationId;
    Column<base::EntityId> m_delayId;
    Column<double> m_delay;
    Column<std::uint8_t> m_kind;
    Column<base::EntityId> m_tableId;
    Column<double> m_value;
    Column<double> m_initialDisplacementFactor;
    Column<double> m_initialVelocityFactor;
};

} // namespace oor::model
