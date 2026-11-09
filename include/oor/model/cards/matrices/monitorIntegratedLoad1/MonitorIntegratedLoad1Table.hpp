#pragma once

#include "oor/model/cards/matrices/monitorIntegratedLoad1/MonitorIntegratedLoad1Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct MonitorIntegratedLoad1Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const MonitorIntegratedLoad1Card& card);
    [[nodiscard]] static std::optional<MonitorIntegratedLoad1Card> read(FieldReader& reader);
    static void write(const MonitorIntegratedLoad1Card& card, FieldWriter& writer);
};

// All the MONPNT1 cards. A monitor point has no numeric ID (IdSpace::None): it is found by its NAME, which is not
// unique (a structural and an aerodynamic point may share one).
class MonitorIntegratedLoad1Table final : public TypedTable<MonitorIntegratedLoad1Table, MonitorIntegratedLoad1Card>
{
public:
    static constexpr std::string_view NAME = "MonitorIntegratedLoad1";
    static inline const CardTypeInfo INFO{NAME, CardCategory::MonitorPoint, base::IdSpace::None};
    using Format = MonitorIntegratedLoad1Format;

    [[nodiscard]] static base::EntityId cardId(const MonitorIntegratedLoad1Card&) noexcept { return 0; }
    std::uint32_t append(const MonitorIntegratedLoad1Card& card, base::CardKey key);
    [[nodiscard]] MonitorIntegratedLoad1Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const MonitorIntegratedLoad1Card& card);
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
};

} // namespace oor::model
