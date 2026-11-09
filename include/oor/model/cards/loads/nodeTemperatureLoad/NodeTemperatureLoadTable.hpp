#pragma once

#include "oor/model/cards/loads/nodeTemperatureLoad/NodeTemperatureLoadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct NodeTemperatureLoadFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const NodeTemperatureLoadCard& card);
    [[nodiscard]] static std::optional<NodeTemperatureLoadCard> read(FieldReader& reader);
    static void write(const NodeTemperatureLoadCard& card, FieldWriter& writer);
};

// All the TEMP cards. The ID is a load set ID shared by many cards (IdSpace::Group). The grid points and their
// temperatures are two lists in CSR form.
class NodeTemperatureLoadTable final : public TypedTable<NodeTemperatureLoadTable, NodeTemperatureLoadCard>
{
public:
    static constexpr std::string_view NAME = "NodeTemperatureLoad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = NodeTemperatureLoadFormat;

    [[nodiscard]] static base::EntityId cardId(const NodeTemperatureLoadCard& card) noexcept { return card.id; }
    std::uint32_t append(const NodeTemperatureLoadCard& card, base::CardKey key);
    [[nodiscard]] NodeTemperatureLoadCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const NodeTemperatureLoadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& nodeColumn() noexcept { return m_node; }
    [[nodiscard]] VariableLengthColumn<double>& temperatureColumn() noexcept { return m_temperature; }

private:
    Column<base::EntityId> m_id;
    VariableLengthColumn<base::EntityId> m_node;
    VariableLengthColumn<double> m_temperature;
};

} // namespace oor::model
