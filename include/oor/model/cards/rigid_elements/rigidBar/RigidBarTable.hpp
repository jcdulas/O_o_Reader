#pragma once

#include "oor/model/cards/rigid_elements/rigidBar/RigidBarCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

struct RigidBarFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RigidBarCard& card);
    [[nodiscard]] static std::optional<RigidBarCard> read(FieldReader& reader);
    static void write(const RigidBarCard& card, FieldWriter& writer);

    // Gives the defaults that depend on other fields their value (CNA, CMA and CMB when they are blank): the reader and
    // the Python constructors both call it, so that a card built in memory is the card read from the same text.
    static void resolveDefaults(RigidBarCard& card) noexcept;
};

// All the RBAR cards. A rigid element is an element: its ID is in the ID space shared by all the elements.
class RigidBarTable final : public TypedTable<RigidBarTable, RigidBarCard>
{
public:
    static constexpr std::string_view NAME = "RigidBar";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = RigidBarFormat;

    [[nodiscard]] static base::EntityId cardId(const RigidBarCard& card) noexcept { return card.id; }
    std::uint32_t append(const RigidBarCard& card, base::CardKey key);
    [[nodiscard]] RigidBarCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const RigidBarCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& nodeAColumn() noexcept { return m_nodeA; }
    [[nodiscard]] Column<base::EntityId>& nodeBColumn() noexcept { return m_nodeB; }
    [[nodiscard]] Column<std::int32_t>& independentComponentsAColumn() noexcept { return m_independentComponentsA; }
    [[nodiscard]] Column<std::int32_t>& independentComponentsBColumn() noexcept { return m_independentComponentsB; }
    [[nodiscard]] Column<std::int32_t>& componentsAtAColumn() noexcept { return m_componentsAtA; }
    [[nodiscard]] Column<std::int32_t>& componentsAtBColumn() noexcept { return m_componentsAtB; }
    [[nodiscard]] Column<double>& alphaCoefficientColumn() noexcept { return m_alphaCoefficient; }
    [[nodiscard]] Column<double>& referenceTemperatureColumn() noexcept { return m_referenceTemperature; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_nodeA;
    Column<base::EntityId> m_nodeB;
    Column<std::int32_t> m_independentComponentsA;
    Column<std::int32_t> m_independentComponentsB;
    Column<std::int32_t> m_componentsAtA;
    Column<std::int32_t> m_componentsAtB;
    Column<double> m_alphaCoefficient;
    Column<double> m_referenceTemperature;
};

} // namespace oor::model
