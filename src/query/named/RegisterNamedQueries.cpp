#include "oor/query/named/RegisterNamedQueries.hpp"

#include "oor/query/named/results/r10_thermal_results/ThermalResultQueries.hpp"
#include "oor/query/named/results/r1_nodal/NodalResultQueries.hpp"
#include "oor/query/named/results/r2_stress_strain/StressStrainQueries.hpp"
#include "oor/query/named/results/r3_forces/ForceQueries.hpp"
#include "oor/query/named/results/r4_equilibrium_cuts/EquilibriumQueries.hpp"
#include "oor/query/named/results/r5_margins/MarginQueries.hpp"
#include "oor/query/named/results/r6_modal_dynamics/ModalQueries.hpp"
#include "oor/query/named/results/r7_checks/CheckQueries.hpp"
#include "oor/query/named/results/r8_optimisation/OptimisationQueries.hpp"
#include "oor/query/named/results/r9_aero_results/AeroResultQueries.hpp"
#include "oor/query/named/topology/t10_aero/AeroQueries.hpp"
#include "oor/query/named/topology/t11_thermal/ThermalQueries.hpp"
#include "oor/query/named/topology/t12_laminates/LaminateQueries.hpp"
#include "oor/query/named/topology/t13_comparison/ComparisonQueries.hpp"
#include "oor/query/named/topology/t14_superelements/SuperelementQueries.hpp"
#include "oor/query/named/topology/t1_inventory/InventoryQueries.hpp"
#include "oor/query/named/topology/t2_connectivity/ConnectivityQueries.hpp"
#include "oor/query/named/topology/t3_boundaries/BoundaryQueries.hpp"
#include "oor/query/named/topology/t4_components/ComponentQueries.hpp"
#include "oor/query/named/topology/t5_mesh_checks/MeshCheckQueries.hpp"
#include "oor/query/named/topology/t6_connections/ConnectionQueries.hpp"
#include "oor/query/named/topology/t7_mass/MassQueries.hpp"
#include "oor/query/named/topology/t8_loads_constraints/LoadConstraintQueries.hpp"
#include "oor/query/named/topology/t9_design/DesignQueries.hpp"

namespace oor::query
{

void registerNamedQueries(NamedQueryRegistry& registry)
{
    addT1Queries(registry);
    addT2Queries(registry);
    addT3Queries(registry);
    addT4Queries(registry);
    addT5Queries(registry);
    addT6Queries(registry);
    addT7Queries(registry);
    addT8Queries(registry);
    addT9Queries(registry);
    addT10Queries(registry);
    addT11Queries(registry);
    addT12Queries(registry);
    addT13Queries(registry);
    addT14Queries(registry);
    addR1Queries(registry);
    addR2Queries(registry);
    addR3Queries(registry);
    addR4Queries(registry);
    addR5Queries(registry);
    addR6Queries(registry);
    addR7Queries(registry);
    addR8Queries(registry);
    addR9Queries(registry);
    addR10Queries(registry);
}

} // namespace oor::query
