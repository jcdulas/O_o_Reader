#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace oor::model
{

// One NLFREQC card: the forcing frequencies of a nonlinear harmonic response by the improved continuation procedure
// (SOL 128, CONTP = 2 of NHRMPRM).
//
// Card layout: NLFREQC  ID  FREQINI  STEPINIT  MINSIZE  MAXSIZE  MAXSTEP  MINFREQ  MAXFREQ
//                      DERINC  PRTOUT
//                      DFOUT  DFQ  DFQTOL  BEGFQ  ENDFQ
// The third line is the user output request: it is there only with the keyword DFOUT, which then needs DFQ.
struct NonlinearHarmonicFreqContinuationCard
{
    base::EntityId id = 0;                 // the NLFREQ field of an NLHARM names it
    double initialFreq = 0.0;              // FREQINI: the frequency the continuation starts at (not negative)
    double initialStep = 0.0;              // STEPINIT: the first step, up or down (not zero)
    std::optional<double> minimumStepSize; // MINSIZE: blank is 1.0E-5 * |STEPINIT|
    std::optional<double> maximumStepSize; // MAXSIZE: blank is 100 * |STEPINIT|
    std::int32_t maximumSteps = 3;         // MAXSTEP: the most steps before the procedure stops
    double minimumFreq = 0.0;              // MINFREQ: the frequency that stops it going down
    std::optional<double> maximumFreq;     // MAXFREQ: blank is FREQINI + |STEPINIT| * MAXSTEP
    double derivativeStep = 1.0E-3;        // DERINC: the central difference step of the derivatives of the loads
    std::string printOnFailure = "No";     // PRTOUT: YES prints output when the convergence fails
    bool outputRequest = false;            // the output request line is there
    double outputFreqStep = 0.0;           // DFQ: the frequency step of the output
    double outputFreqTolerance = 5.0;      // DFQTOL: the tolerance in percent of DFQ for the closest step
    std::optional<double> outputStartFreq; // BEGFQ: blank is FREQINI
    std::optional<double> outputEndFreq;   // ENDFQ: blank is MAXFREQ
};

} // namespace oor::model
