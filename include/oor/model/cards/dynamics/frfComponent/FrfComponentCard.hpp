#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <string>

namespace oor::model
{

enum class FrfMedium : std::uint8_t
{
    Database,          // the database of the FRF generation job
    BinaryResultsFile, // a binary results file of an earlier run
    UniversalFile      // a universal file
};

// One FRFCOMP card: a component whose frequency response functions were made by an earlier run and that an FRF based
// assembly (FBA) puts together with others.
//
// Card layout: FRFCOMP  COMPID  COMPNAME  MEDIUM  UNITNO  LSCALFAC  FSCALFAC
//
// The ID is the number that the other FRF cards write in their COMPID fields; the name is the other way to name the
// component (they write either) and the one the ASMOUT keyword of the Case Control FRF command takes. COMPNAME is kept
// as written, and may stay blank: the format does not mark it optional, but nothing on the card depends on it and a
// deck without one is read rather than refused (the cards that an FRF generation job punches have both). It is not
// CONNINFO, ALL, COMP or ASSEMBLY (the words of ASMOUT) and it is not a whole number (every card that names the
// component would read that as an ID). UNITNO is the Fortran unit of a binary or universal file (needed: it is
// the unit that the ASSIGN statement of the file opens); a database has no unit, and a number given with DB is kept as
// written. LSCALFAC and FSCALFAC scale the universal file only and default to 1.0: a default that does not
// depend on another field, so it is omitted from the written card when it is 1.0.
struct FrfComponentCard
{
    base::EntityId id = 0;
    std::string name;
    FrfMedium medium = FrfMedium::Database;
    std::int32_t unitNumber = 0; // 0 when the field is blank
    double lengthScaleFactor = 1.0;
    double forceScaleFactor = 1.0;

    [[nodiscard]] bool operator==(const FrfComponentCard&) const noexcept = default;
};

} // namespace oor::model
