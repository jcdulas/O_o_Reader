#pragma once

#include "oor/model/cards/matrices/monitorNodeForceSum3/MonitorNodeForceSum3Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct MonitorNodeForceSum3Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const MonitorNodeForceSum3Card& card);
    [[nodiscard]] static std::optional<MonitorNodeForceSum3Card> read(FieldReader& reader);
    static void write(const MonitorNodeForceSum3Card& card, FieldWriter& writer);
};

// All the MONPNT3 cards. A monitor point has no numeric ID (IdSpace::None): it is found by its NAME, which the format
// reference says is unique (a repeated NAME is reported when the deck is read).
class MonitorNodeForceSum3Table final : public TypedTable<MonitorNodeForceSum3Table, MonitorNodeForceSum3Card>
{
public:
    static constexpr std::string_view NAME = "MonitorNodeForceSum3";
    static inline const CardTypeInfo INFO{NAME, CardCategory::MonitorPoint, base::IdSpace::None};
    using Format = MonitorNodeForceSum3Format;

    [[nodiscard]] static base::EntityId cardId(const MonitorNodeForceSum3Card&) noexcept { return 0; }
    std::uint32_t append(const MonitorNodeForceSum3Card& card, base::CardKey key);
    [[nodiscard]] MonitorNodeForceSum3Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const MonitorNodeForceSum3Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;
    // Two MONPNT3 cards of one NAME are reported.
    void finishRead(Model& model, const ReadContext& context) override;

    [[nodiscard]] Column<std::string>& nameColumn() noexcept { return m_name; }
    [[nodiscard]] Column<std::string>& labelColumn() noexcept { return m_label; }
    [[nodiscard]] Column<std::int32_t>& outputAxesColumn() noexcept { return m_outputAxes; }
    [[nodiscard]] Column<base::EntityId>& nodeSetIdColumn() noexcept { return m_nodeSetId; }
    [[nodiscard]] Column<base::EntityId>& elementSetColumn() noexcept { return m_elementSet; }
    [[nodiscard]] Column<base::EntityId>& positionFrameIdColumn() noexcept { return m_positionFrameId; }
    [[nodiscard]] Column<double>& xCoordColumn() noexcept { return m_xCoord; }
    [[nodiscard]] Column<double>& yCoordColumn() noexcept { return m_yCoord; }
    [[nodiscard]] Column<double>& zCoordColumn() noexcept { return m_zCoord; }
    [[nodiscard]] Column<std::string>& excludedColumn() noexcept { return m_excluded; }
    [[nodiscard]] Column<base::EntityId>& displacementFrameIdColumn() noexcept { return m_displacementFrameId; }

private:
    Column<std::string> m_name;
    Column<std::string> m_label;
    Column<std::int32_t> m_outputAxes;
    Column<base::EntityId> m_nodeSetId;
    Column<base::EntityId> m_elementSet;
    Column<base::EntityId> m_positionFrameId;
    Column<double> m_xCoord;
    Column<double> m_yCoord;
    Column<double> m_zCoord;
    Column<std::string> m_excluded;
    Column<base::EntityId> m_displacementFrameId;
};

} // namespace oor::model
