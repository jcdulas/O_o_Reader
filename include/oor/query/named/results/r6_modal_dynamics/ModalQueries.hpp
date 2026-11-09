#pragma once

#include "oor/query/named/NamedQueryRegistry.hpp"

namespace oor::query
{

// Family R6, modal, buckling and dynamics: the modes with their effective mass, the buckling factors, the largest value
// of the mode shapes, the strain energy by property, and the frequency responses, peaks and time histories.
void addModesTableQuery(NamedQueryRegistry& registry);
void addBucklingFactorsQuery(NamedQueryRegistry& registry);
void addModeShapeMaxQuery(NamedQueryRegistry& registry);
void addStrainEnergyByPropertyQuery(NamedQueryRegistry& registry);
void addFrfTableQuery(NamedQueryRegistry& registry);
void addPeakResponseQuery(NamedQueryRegistry& registry);
void addTimeHistoryQuery(NamedQueryRegistry& registry);
void addMaxOverTimeQuery(NamedQueryRegistry& registry);

void addR6Queries(NamedQueryRegistry& registry);

} // namespace oor::query
