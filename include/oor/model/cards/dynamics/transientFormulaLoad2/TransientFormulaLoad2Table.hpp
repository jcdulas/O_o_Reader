#pragma once

#include "oor/model/cards/dynamics/transientFormulaLoad2/TransientFormulaLoad2Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct TransientFormulaLoad2Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const TransientFormulaLoad2Card& card);
    [[nodiscard]] static std::optional<TransientFormulaLoad2Card> read(FieldReader& reader);
    static void write(const TransientFormulaLoad2Card& card, FieldWriter& writer);
};

// All the TLOAD2 cards. The ID is a dynamic load set ID shared by the RLOAD, TLOAD and DAREA cards (IdSpace::Group).
class TransientFormulaLoad2Table final : public TypedTable<TransientFormulaLoad2Table, TransientFormulaLoad2Card>
{
public:
    static constexpr std::string_view NAME = "TransientFormulaLoad2";
    static inline const CardTypeInfo INFO{NAME, CardCategory::DynamicLoad, base::IdSpace::Group};
    using Format = TransientFormulaLoad2Format;

    [[nodiscard]] static base::EntityId cardId(const TransientFormulaLoad2Card& card) noexcept { return card.id; }
    std::uint32_t append(const TransientFormulaLoad2Card& card, base::CardKey key);
    [[nodiscard]] TransientFormulaLoad2Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const TransientFormulaLoad2Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& excitationIdColumn() noexcept { return m_excitationId; }
    [[nodiscard]] Column<base::EntityId>& delayIdColumn() noexcept { return m_delayId; }
    [[nodiscard]] Column<double>& delayColumn() noexcept { return m_delay; }
    [[nodiscard]] Column<std::uint8_t>& kindColumn() noexcept { return m_kind; }
    [[nodiscard]] Column<double>& time1Column() noexcept { return m_time1; }
    [[nodiscard]] Column<double>& time2Column() noexcept { return m_time2; }
    [[nodiscard]] Column<double>& frequencyColumn() noexcept { return m_frequency; }
    [[nodiscard]] Column<double>& phaseColumn() noexcept { return m_phase; }
    [[nodiscard]] Column<double>& coefficientCColumn() noexcept { return m_coefficientC; }
    [[nodiscard]] Column<double>& exponentBColumn() noexcept { return m_exponentB; }
    [[nodiscard]] Column<double>& initialDisplacementFactorColumn() noexcept { return m_initialDisplacementFactor; }
    [[nodiscard]] Column<double>& initialVelocityFactorColumn() noexcept { return m_initialVelocityFactor; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_excitationId;
    Column<base::EntityId> m_delayId;
    Column<double> m_delay;
    Column<std::uint8_t> m_kind;
    Column<double> m_time1;
    Column<double> m_time2;
    Column<double> m_frequency;
    Column<double> m_phase;
    Column<double> m_coefficientC;
    Column<double> m_exponentB;
    Column<double> m_initialDisplacementFactor;
    Column<double> m_initialVelocityFactor;
};

} // namespace oor::model
