#pragma once

#include "oor/model/cards/elements/beam2/Beam2Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct Beam2Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const Beam2Card& card);
    [[nodiscard]] static std::optional<Beam2Card> read(FieldReader& reader);
    static void write(const Beam2Card& card, FieldWriter& writer);
};

// All the CBEAM elements. The orientation vector and the two offset vectors are (N, 3) NumPy views without copy.
// Blank SA and SB are 0.
class Beam2Table final : public TypedTable<Beam2Table, Beam2Card>
{
public:
    static constexpr std::string_view NAME = "Beam2";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = Beam2Format;

    Beam2Table() : m_offsetKind(std::string(Beam2Card::DEFAULT_OFFSET_KIND)) {}

    [[nodiscard]] static base::EntityId cardId(const Beam2Card& card) noexcept { return card.id; }
    std::uint32_t append(const Beam2Card& card, base::CardKey key);
    [[nodiscard]] Beam2Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const Beam2Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;
    // Applies the BEAMOR defaults to the PID, orientation and OFFT that were blank (nothing when there is no BEAMOR).
    void finishRead(Model& model, const ReadContext& context) override;
    // True when the model has a BEAMOR: the cards then write PID, the orientation and OFFT explicitly.
    [[nodiscard]] bool fieldsGovernedByDefaults(const Model& model) const override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<base::EntityId>& nodeAColumn() noexcept { return m_nodeA; }
    [[nodiscard]] Column<base::EntityId>& nodeBColumn() noexcept { return m_nodeB; }
    [[nodiscard]] Column<std::array<double, 3>>& orientationColumn() noexcept { return m_orientation; }
    [[nodiscard]] Column<base::EntityId>& orientationNodeColumn() noexcept { return m_orientationNode; }
    [[nodiscard]] Column<std::string>& offsetKindColumn() noexcept { return m_offsetKind; }
    [[nodiscard]] Column<std::int32_t>& pinFlagsAColumn() noexcept { return m_pinFlagsA; }
    [[nodiscard]] Column<std::int32_t>& pinFlagsBColumn() noexcept { return m_pinFlagsB; }
    [[nodiscard]] Column<std::array<double, 3>>& offsetAColumn() noexcept { return m_offsetA; }
    [[nodiscard]] Column<std::array<double, 3>>& offsetBColumn() noexcept { return m_offsetB; }
    [[nodiscard]] Column<base::EntityId>& warpingPointAColumn() noexcept { return m_warpingPointA; }
    [[nodiscard]] Column<base::EntityId>& warpingPointBColumn() noexcept { return m_warpingPointB; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<base::EntityId> m_nodeA;
    Column<base::EntityId> m_nodeB;
    Column<std::array<double, 3>> m_orientation;
    Column<base::EntityId> m_orientationNode;
    Column<std::string> m_offsetKind;
    Column<std::int32_t> m_pinFlagsA;
    Column<std::int32_t> m_pinFlagsB;
    Column<std::array<double, 3>> m_offsetA;
    Column<std::array<double, 3>> m_offsetB;
    Column<base::EntityId> m_warpingPointA;
    Column<base::EntityId> m_warpingPointB;
    Column<std::uint8_t> m_blankFields; // Beam2Card::BLANK_* bits; nothing stored once the read is finished
};

} // namespace oor::model
