#include "oor/query/named/results/r10_thermal_results/ThermalResultQueries.hpp"
#include "oor/query/providers/quantities/CompositeQuantities.hpp"
#include "oor/query/providers/quantities/ElementQuantities.hpp"
#include "oor/query/providers/quantities/ModalQuantities.hpp"
#include "oor/query/providers/quantities/MonitorPointLoads.hpp"
#include "oor/query/providers/quantities/QuantityProvider.hpp"
#include "oor/query/providers/quantities/Resultants.hpp"

#include <algorithm>
#include <mutex>

namespace oor::query
{

namespace
{

std::mutex& switchMutex()
{
    static std::mutex mutex;
    return mutex;
}

std::vector<std::string>& switchedOff()
{
    static std::vector<std::string> names;
    return names;
}

} // namespace

bool ProviderSwitches::isDisabled(const std::string& name)
{
    const std::lock_guard<std::mutex> lock(switchMutex());
    const std::vector<std::string>& names = switchedOff();
    return std::find(names.begin(), names.end(), name) != names.end();
}

void ProviderSwitches::setDisabled(std::vector<std::string> names)
{
    const std::lock_guard<std::mutex> lock(switchMutex());
    switchedOff() = std::move(names);
}

std::vector<std::string> ProviderSwitches::disabled()
{
    const std::lock_guard<std::mutex> lock(switchMutex());
    return switchedOff();
}

const std::vector<QuantityInfo>& allQuantities()
{
    static const std::vector<QuantityInfo> QUANTITIES = []
    {
        std::vector<QuantityInfo> all;
        // The order of the catalogue. The quantities with a class for each way come from their chain; the
        // others are two ways inside one query and are listed by name.
        all.push_back(stressChain().info());
        all.push_back(strainChain().info());
        all.push_back({"von Mises, principal values, maximum shear",
                       {{"stored column", false}, {"computed from the components", false}}});
        all.push_back(
            {"value in a requested frame",
             {{"stored in that frame", false}, {"rotated from the stored frame (validated pair only)", false}}});
        all.push_back(shellResultantChain().info());
        all.push_back(oneDimensionalForceChain().info());
        all.push_back(fastenerForceChain().info());
        all.push_back(appliedLoadChain().info());
        all.push_back(reactionChain().info());
        all.push_back(cutChain().info());
        all.push_back(monitorChain().info());
        all.push_back(failureIndexChain().info());
        all.push_back(effectiveMassChain().info());
        all.push_back({"frequency of a mode", {{"stored cycles", false}, {"derived from the eigenvalue", false}}});
        all.push_back(
            {"mass and centre of gravity", {{"model", false}, {"grid point weight table of the result file", false}}});
        all.push_back({"margin allowable", {{"caller", false}, {"material card", false}}});
        all.push_back(heatFluxChain().info());
        all.push_back(appliedHeatQuantity());
        all.push_back({"flutter and divergence speed", {{"interpolated from the flutter summary", true}}});
        all.push_back({"predicted response", {{"sensitivities times the change of the design variables", true}}});
        return all;
    }();
    return QUANTITIES;
}

} // namespace oor::query
