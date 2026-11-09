#pragma once

#include "oor/model/cards/matrices/monitorDisplacement/MonitorDisplacementCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct MonitorDisplacementFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const MonitorDisplacementCard& card);
    [[nodiscard]] static std::optional<MonitorDisplacementCard> read(FieldReader& reader);
    static void write(const MonitorDisplacementCard& card, FieldWriter& writer);
};

// All the MONDSP1 cards. A monitor point has no numeric ID (IdSpace::None): it is found by its NAME, which is not
// unique (a structural and an aerodynamic point may share one).
class MonitorDisplacementTable final : public TypedTable<MonitorDisplacementTable, MonitorDisplacementCard>
{
public:
    static constexpr std::string_view NAME = "MonitorDisplacement";
    static inline const CardTypeInfo INFO{NAME, CardCategory::MonitorPoint, base::IdSpace::None};
    using Format = MonitorDisplacementFormat;

    MonitorDisplacementTable();

    [[nodiscard]] static base::EntityId cardId(const MonitorDisplacementCard&) noexcept { return 0; }
    std::uint32_t append(const MonitorDisplacementCard& card, base::CardKey key);
    [[nodiscard]] MonitorDisplacementCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const MonitorDisplacementCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<std::string>& nameColumn() noexcept { return m_name; }
    [[nodiscard]] Column<std::string>& labelColumn() noexcept { return m_label; }
    [[nodiscard]] Column<std::int32_t>& outputAxesColumn() noexcept { return m_outputAxes; }
    [[nodiscard]] Column<std::string>& componentColumn() noexcept { return m_component; }
    [[nodiscard]] Column<base::EntityId>& positionFrameIdColumn() noexcept { return m_positionFrameId; }
    [[nodiscard]] Column<double>& xCoordColumn() noexcept { return m_xCoord; }
    [[nodiscard]] Column<double>& yCoordColumn() noexcept { return m_yCoord; }
    [[nodiscard]] Column<double>& zCoordColumn() noexcept { return m_zCoord; }
    [[nodiscard]] Column<base::EntityId>& displacementFrameIdColumn() noexcept { return m_displacementFrameId; }
    [[nodiscard]] Column<std::int32_t>& sampledComponentsColumn() noexcept { return m_sampledComponents; }

private:
    Column<std::string> m_name;
    Column<std::string> m_label;
    Column<std::int32_t> m_outputAxes;
    Column<std::string> m_component;
    Column<base::EntityId> m_positionFrameId;
    Column<double> m_xCoord;
    Column<double> m_yCoord;
    Column<double> m_zCoord;
    Column<base::EntityId> m_displacementFrameId;
    Column<std::int32_t> m_sampledComponents;
};

} // namespace oor::model
