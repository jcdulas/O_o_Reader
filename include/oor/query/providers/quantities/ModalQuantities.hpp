#pragma once

#include "oor/query/providers/quantities/QuantityProvider.hpp"
#include "oor/query/results/RowAccumulator.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <map>
#include <optional>
#include <tuple>
#include <vector>

namespace oor::query
{

// One mode of a modal analysis as the eigenvalue table lists it.
struct ModeRow
{
    std::size_t set = 0;
    std::string analysis;
    std::int32_t subcase = 0;
    std::int32_t designCycle = -1;
    std::int32_t mode = 0;
    double eigenvalue = 0.0;
    double radians = 0.0;
    double cycles = 0.0;
    double generalizedMass = 0.0;
    double generalizedStiffness = 0.0;
    Precision precision = Precision::Single;
    std::string frequencyWay; // "stored cycles" or "derived from the eigenvalue"
};

// Which eigenvalue tables a query reads: those of the real modes, or those of the buckling modes.
enum class ModeAnalysis : std::uint8_t
{
    RealModes,
    Buckling
};

// The modes of the eigenvalue tables of the analysis `analysis` (a buckling table has the analysis code of the buckling
// modes, 8; every other real eigenvalue table is of real modes) of every result set that the call keeps, in the order
// of the file. The subcase of a table that has none (the eigenvalue tables carry subcase 0) is the one subcase of the
// eigenvector tables of the same set when there is only one.
[[nodiscard]] std::vector<ModeRow> modeRowsOf(const ResultQueryContext& context, ModeAnalysis analysis);

// What a query of the effective mass asks of its providers: the modes it needs the mass fractions of.
struct ModalRequest
{
    std::vector<ModeRow> modes;
};

// The effective mass fraction of each mode in the six directions, from the providers: a mode that an earlier way gave
// is not given again.
struct ModalFractions
{
    std::size_t set = 0;
    std::int32_t subcase = 0;
    std::int32_t designCycle = -1;
    std::int32_t mode = 0;
    std::array<double, 6> fraction = {std::nan(""), std::nan(""), std::nan(""),
                                      std::nan(""), std::nan(""), std::nan("")};
    std::int16_t provider = 0;
};

class ModalFractionSet
{
public:
    void beginProvider(std::int16_t index, const ProviderInfo& info);
    [[nodiscard]] std::int64_t rowCount() const noexcept { return static_cast<std::int64_t>(m_rows.size()); }
    bool add(ModalFractions&& row);
    [[nodiscard]] bool covers(std::size_t set, std::int32_t subcase, std::int32_t designCycle, std::int32_t mode) const;
    [[nodiscard]] const std::vector<ModalFractions>& rows() const noexcept { return m_rows; }
    [[nodiscard]] const std::vector<ProviderInfo>& providers() const noexcept { return m_providers; }

private:
    std::vector<ModalFractions> m_rows;
    std::map<std::tuple<std::size_t, std::int32_t, std::int32_t, std::int32_t>, std::size_t> m_index;
    std::vector<ProviderInfo> m_providers;
    std::int16_t m_current = 0;
};

using ModalProvider = QuantityProvider<ModalRequest, ModalFractionSet>;
using ModalChain = ProviderChain<ModalRequest, ModalFractionSet>;

// The effective mass of the modes: the stored tables (the modal effective mass and the rigid body mass matrices of the
// result file), else computed from the eigenvectors and the lumped masses of the model.
[[nodiscard]] const ModalChain& effectiveMassChain();

// The ways of obtaining the frequency of a mode, for the catalogue of the quantities.
[[nodiscard]] QuantityInfo modeFrequencyInfo();
// The ways of obtaining the mass and the centre of gravity.
[[nodiscard]] QuantityInfo massSourceInfo();

} // namespace oor::query
