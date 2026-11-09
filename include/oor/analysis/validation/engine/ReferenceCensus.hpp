#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/Model.hpp"
#include "oor/model/references/ReferenceVisitor.hpp"

#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

namespace oor::analysis
{

// A reference field that points at nothing (and has to point at something).
struct UnresolvedUse
{
    model::CardLocation location; // the card; unused for a Case Control command
    std::string field;
    model::ReferenceTarget target = model::ReferenceTarget::Other;
    base::EntityId id = 0;
    bool inCaseControl = false;
    std::string scope; // Case Control: the kind of the section, "subcase", "global", ... (subcaseKindName)
    std::int64_t scopeNumber = 0;
};

// A reference field of a card to a property or a material, for the checks that compare the types of what refers to
// what.
struct CardReference
{
    model::CardLocation location; // the card that refers
    std::string field;
    base::EntityId id = 0; // the property or material it names (it may not exist)
};

// Every reference field of a model looked at once (ReferenceChecker::scan): the ones that point at nothing, and the IDs
// that something points at. The reference checks and the orphan checks share it, so that the cards are visited once
// per validation.
struct ReferenceCensus
{
    std::vector<UnresolvedUse> unresolved;        // the cards in table and row order, then the Case Control commands
    std::vector<CardReference> elementProperties; // the property of each element card (CQUAD4 pid, ...)
    std::vector<CardReference> propertyMaterials; // the materials of each property card (PSHELL mid1, a ply's mid, ...)
    std::vector<CardReference>
        elementListMembers; // the elements an ELIST card names (the ends of a THRU range included)
    std::vector<CardReference>
        propertyExtensions; // the property that a property card names (the PID of a PBARN1, PCOMPA, ...)
    std::unordered_set<base::EntityId> nodes;       // grid and scalar point IDs some card names
    std::unordered_set<base::EntityId> properties;  // property IDs
    std::unordered_set<base::EntityId> materials;   // material IDs
    std::unordered_set<base::EntityId> coordinates; // coordinate system IDs
    std::unordered_set<base::EntityId>
        loadSets; // load set IDs a LOAD, DLOAD or dynamic load card or the Case Control names
    std::unordered_set<base::EntityId> constraintSets; // constraint set IDs an SPCADD, MPCADD or the Case Control names

    [[nodiscard]] static ReferenceCensus build(const model::Model& model);
};

} // namespace oor::analysis
