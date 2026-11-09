#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/superelements/superelementBulkOptions/SuperelementBulkOptionsCard.hpp"
#include "oor/model/core/Model.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace oor::analysis
{

// What a superelement is (the `superelement_type` column of `parts_table`). The SEBULK TYPE when the deck has one
// (PRIMARY is then a part, or a list superelement when it has no BEGIN SUPER section), else a part for BEGIN SUPER, a
// secondary superelement for CSUPER, a list superelement for SESET, SEELT, CSUPEXT and the SEID of GRID.
enum class SuperelementType : std::uint8_t
{
    Primary,               // a part superelement: BEGIN SUPER = n
    List,                  // a main Bulk Data superelement: SESET, the SEID of GRID, SEELT, CSUPEXT
    Repeat,                // SEBULK REPEAT: an identical copy of the part RSEID
    Mirror,                // SEBULK MIRROR: a mirror image of the part RSEID (it needs an SEMPLN)
    External,              // SEBULK EXTERNAL: from a database of an earlier run
    ExternalBinaryResults, // SEBULK TYPE, the external binary results word: from the result file on unit UNITNO
    ExternalMatrixFile,    // SEBULK TYPE, the external matrix file word: from the matrix file on unit UNITNO
    ExternalPunch,         // SEBULK EXTPCH: from the DMIG entries of a punch file
    ExternalReducedModel,  // SEBULK EXTROM: a reduced order model on unit UNITNO
    Collector,             // SEBULK COLLCTR: holds no point
    Secondary              // CSUPER: an image of a primary, or matrices from an external source
};

// Primary, List, Repeat, ..., as `partsTable` writes them: the neutral word of the SuperelementBulkOptions kind (List
// and Secondary, which no such card gives, name a list superelement and a CSUPER secondary).
[[nodiscard]] std::string superelementTypeName(SuperelementType type);

// One superelement of a deck, with what decides its boundary.
struct SuperelementEntry
{
    base::EntityId seid = 0;
    SuperelementType type = SuperelementType::Primary;
    // The section BEGIN SUPER = seid (the one without a MASSID), null when it has none.
    const model::Model* part = nullptr;
    // The model its point IDs are named in: its part, the part of its primary for an image, the main Bulk Data for a
    // list superelement.
    const model::Model* idModel = nullptr;
    // The primary of an image: the RSEID of an SEBULK REPEAT or MIRROR, the PSID of a CSUPER (0 for none, or for a
    // CSUPER whose matrices come from an external source).
    base::EntityId imageOf = 0;
    model::SuperelementBulkOptionsMethod method = model::SuperelementBulkOptionsMethod::Auto;
    // a TOL of 0.0 is the default, the user parameter CONFAC (1.0E-5 unless the deck
    // sets it).
    double tolerance = 0.0;
    bool hasOwnTolerance = false; // the SEBULK gives a TOL
    bool checksLocation = true;   // SEBULK LOC = YES
    // The superelement downstream of it: the SEID of the SETREE that lists it as upstream, else the residual structure.
    base::EntityId downstream = 0;
    base::EntityId unitNumber = 0; // SEBULK UNITNO (external binary results, external matrix file, EXTROM)

    [[nodiscard]] bool isExternal() const noexcept;
    // A copy of another superelement (REPEAT, MIRROR, CSUPER): it has no points of its own.
    [[nodiscard]] bool isImage() const noexcept;
    // A superelement whose points are those of a BEGIN SUPER section, its own or its primary's.
    [[nodiscard]] bool hasPartPoints() const noexcept;
};

// The superelements of a deck (every number SuperelementIds finds), with their type, primary, method, tolerance,
// downstream superelement and the model of their IDs; the tolerances of the location search between two of them.
// Built from the residual structure of the deck the model belongs to; holds pointers into that deck, valid while it is
// not edited.
class SuperelementCatalogue
{
public:
    explicit SuperelementCatalogue(const model::Model& model);

    // By ascending number; the residual structure is not one of them.
    [[nodiscard]] const std::vector<SuperelementEntry>& entries() const noexcept { return m_entries; }
    // The entry of a number, or null (0, the residual structure, has none).
    [[nodiscard]] const SuperelementEntry* find(base::EntityId seid) const noexcept;
    [[nodiscard]] const model::Model& residual() const noexcept { return *m_residual; }
    // PARAM CONFAC, else its format reference default 1.0E-5.
    [[nodiscard]] double parameterTolerance() const noexcept { return m_parameterTolerance; }

    // The tolerance of the location search between two superelements (0 is the residual structure). The rule of the
    // format reference: the TOL of an SECONCT between them, else the TOL of their SEBULK (SEBULK 0 for the residual
    // structure), else CONFAC. The format reference does not say which SEBULK wins when both give one: the larger is
    // taken (an assumption).
    [[nodiscard]] double searchTolerance(base::EntityId first, base::EntityId second) const noexcept;
    // The largest tolerance of any pair (the size of the cells of the search).
    [[nodiscard]] double largestTolerance() const noexcept;
    // The tolerance of the pairs of an SECONCT between SEIDA and SEIDB: its TOL, else the SEBULK TOL of SEIDA, else
    // CONFAC.
    [[nodiscard]] double connectionTolerance(base::EntityId seida, double pairTolerance) const noexcept;

private:
    [[nodiscard]] double ownTolerance(base::EntityId seid) const noexcept;

    const model::Model* m_residual = nullptr;
    std::vector<SuperelementEntry> m_entries;
    double m_parameterTolerance = 1.0e-5;
    double m_residualTolerance = 0.0; // SEBULK 0 TOL; 0 when not given
    // The SECONCT TOLs (non-zero) by pair of superelements, the smaller number first.
    std::map<std::pair<base::EntityId, base::EntityId>, double> m_connectionTolerances;
};

} // namespace oor::analysis
