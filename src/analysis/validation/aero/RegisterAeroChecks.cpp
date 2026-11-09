#include "oor/analysis/validation/aero/AeroChecks.hpp"

namespace oor::analysis
{

void registerAeroChecks(CheckRegistry& registry)
{
    addPanelOverlapCheck(registry);
    addPanelPropertyCheck(registry);
    addDivisionListCheck(registry);
    addListBoxCheck(registry);
    addSplineBoxCheck(registry);
    addSplineIdRepeatedCheck(registry);
    addTrimIdRepeatedCheck(registry);
    addLabelRepeatedCheck(registry);
    addLabelUndefinedCheck(registry);
    addLinkDependentRepeatedCheck(registry);
    addSurfaceLabelCheck(registry);
    addComponentNameRepeatedCheck(registry);
    addComponentLabelUndefinedCheck(registry);
    addMonitorComponentUndefinedCheck(registry);
    addFlutterFactorCountCheck(registry);
    addGustVelocityCheck(registry);
    addAeroCardMissingCheck(registry);
    addAeroLiftingPanelPropBodyKindCheck(registry);
    addAeroLiftingPanelPropBodyGroupCheck(registry);
    addBodyElementsCheck(registry);
    addBodyGroupCheck(registry);
    addAeroMachBoxPanel3ListsCheck(registry);
    addStripIdsCheck(registry);
    addAeroSplineEquation3ComponentCheck(registry);
    addSplineBodyCheck(registry);
    addSplineSetSizeCheck(registry);
    addSurfaceLimitsCheck(registry);
    addLinkTrimCheck(registry);
    addMachOneCheck(registry);
    addSystemRectangularCheck(registry);
}

} // namespace oor::analysis
