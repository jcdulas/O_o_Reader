#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// The bulk viscosity type of a MATBV card.
enum class BulkViscosityMatType : std::uint8_t
{
    Standard, // the default: the standard model
    Enhanced  // the enhanced model
};

// One MATBV card: the bulk viscosity of a material (SOL 700). The material is a basic one: the MAT1, MAT2, MAT8, MATORT
// or MATHE with the same ID.
//
// Card layout:
//   MATBV  MID  BULKTYP  BULKQ  BULKL  CSCALE  DMPFAC  IIMMREL                    fields 1 to 7
// BULKQ and BULKL are the quadratic and the linear coefficients (0.0 and 1.0 when blank). CSCALE (1.0) scales the
// compressive stresses of membrane shell elements, 0.0 making a tension only material; DMPFAC (0.0) is the damping
// applied to their stresses (0.05 for 5%); IIMMREL (1.0E-3) is the relaxation factor of the Initial Metric Method,
// greater than 0.0 and less than 1.0.
struct BulkViscosityMatCard
{
    static constexpr double DEFAULT_INITIAL_METRIC_RELAXATION = 1.0e-3;

    base::EntityId id = 0; // the material that the card extends
    BulkViscosityMatType bulkViscosityKind = BulkViscosityMatType::Standard;
    double quadraticBulkViscosity = 0.0;
    double linearBulkViscosity = 1.0;
    double compressionScale = 1.0;
    double membraneDamping = 0.0;
    double initialMetricRelaxation = DEFAULT_INITIAL_METRIC_RELAXATION;
};

} // namespace oor::model
