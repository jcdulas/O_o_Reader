#pragma once

#include "oor/query/providers/quantities/QuantityProvider.hpp"
#include "oor/query/results/RowAccumulator.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <map>
#include <string>
#include <tuple>
#include <vector>

namespace oor::query
{

// The integrated load of one monitor point at one step of one subcase, for one contribution (monitor
// point loads): the six components along the CD axes of the point, about its point; NaN for a component the point does
// not monitor.
struct MonitorLoadRow
{
    std::size_t set = 0;
    std::string analysis;
    std::int32_t subcase = 0;
    std::int32_t designCycle = -1;
    results::StepKind stepKind = results::StepKind::None;
    double step = std::nan("");
    Precision precision = Precision::Double;
    bool isComplex = false;
    std::string name;
    std::string card;         // the card type: MonitorIntegratedLoad1, MonitorElemLoad2, MonitorNodeForceSum3, ...
    std::string side;         // structural, aerodynamic
    std::string contribution; // inertial, external, flexible, gust, totalAero, total, resultant
    std::int32_t displacementFrameId = 0;
    std::array<double, 3> point = {std::nan(""), std::nan(""), std::nan("")}; // basic system
    std::array<double, 6> real = {std::nan(""), std::nan(""), std::nan(""), std::nan(""), std::nan(""), std::nan("")};
    std::array<double, 6> imaginary = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    std::int16_t provider = 0;
    std::string status; // of the named result the row was read from
};

// What the query asks: the points by name and the cards (empty: every one).
struct MonitorRequest
{
    std::vector<std::string> names;
    std::vector<std::string> cards;
    [[nodiscard]] bool wants(const std::string& name, const std::string& card) const;
};

// The rows the providers gave. A point of a subcase that an earlier provider gave is not given again by a later one.
class MonitorLoadSet
{
public:
    void beginProvider(std::int16_t index, const ProviderInfo& info);
    [[nodiscard]] std::int64_t rowCount() const noexcept { return static_cast<std::int64_t>(m_rows.size()); }
    // True when an earlier provider gave this point of this subcase.
    [[nodiscard]] bool covered(std::size_t set, const std::string& name, std::int32_t subcase,
                               std::int32_t designCycle) const;
    void add(MonitorLoadRow&& row);
    [[nodiscard]] const std::vector<MonitorLoadRow>& rows() const noexcept { return m_rows; }
    [[nodiscard]] const std::vector<ProviderInfo>& providers() const noexcept { return m_providers; }

private:
    using Key = std::tuple<std::size_t, std::string, std::int32_t, std::int32_t>;
    std::vector<MonitorLoadRow> m_rows;
    std::map<Key, std::int16_t> m_owner;
    std::vector<ProviderInfo> m_providers;
    std::int16_t m_current = 0;
};

using MonitorProvider = QuantityProvider<MonitorRequest, MonitorLoadSet>;
using MonitorChain = ProviderChain<MonitorRequest, MonitorLoadSet>;

// The ways of the monitor point loads: the stored monitor point table (the named result `monitorPointLoads`), then a
// MONPNT3 computed from the grid point forces.
[[nodiscard]] const MonitorChain& monitorChain();

} // namespace oor::query
