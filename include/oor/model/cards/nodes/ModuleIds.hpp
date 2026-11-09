#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/FieldWriter.hpp"
#include "oor/model/references/ReferenceVisitor.hpp"

#include <optional>
#include <string>
#include <string_view>

// The IDs that the contact cards across modules (MDBCNCT, MDBCTB1) give as a pair: the number of the module that
// defines a card and the ID of that card inside the module. One kind of value; each card says where its pairs are in
// its fields.
//
// Module 0 is the main Bulk Data (where an MDBCPAR belongs), and a blank module field means the same: the pair is
// stored with module 0. The other modules are the BEGIN MODULE sections of a deck, which are parts of the deck (models
// of their own): an ID that a pair places in module n is looked up in the part of module n
// (ReferenceVisitor::visitInPart).
namespace oor::model
{

struct ModuleId
{
    base::EntityId module = 0; // the module that defines the card, 0 for the main Bulk Data
    base::EntityId id = 0;     // the ID of the card inside that module; 0 only in a pair that is not given

    [[nodiscard]] bool operator==(const ModuleId&) const noexcept = default;
};

// Reads a pair from its two fields. Both blank gives nothing (the padding of a line); an ID without a module is a pair
// of module 0; a module without an ID is an error on `idName`, and so is a bad integer.
[[nodiscard]] inline std::optional<ModuleId> readModuleId(FieldReader& reader, int moduleField, int idField,
                                                          const char* moduleName, const char* idName)
{
    if (reader.isBlank(moduleField) && reader.isBlank(idField))
        return std::nullopt;
    ModuleId pair;
    pair.module = reader.integer(moduleField, moduleName, 0);
    pair.id = reader.requiredInteger(idField, idName);
    return pair;
}

// Writes a pair. The module of the main Bulk Data is left blank, unless `explicitModule` says the layout allows no
// blank before the ID (MDBCTB1 refuses a blank between IDs).
inline void writeModuleId(FieldWriter& writer, int moduleField, int idField, const ModuleId& pair, bool explicitModule)
{
    if (explicitModule)
        writer.integer(moduleField, pair.module);
    else
        writer.integerUnlessDefault(moduleField, pair.module, 0);
    writer.integer(idField, pair.id);
}

// The problem with a pair that is given: a module that is zero or more and a positive ID.
[[nodiscard]] inline std::optional<CardProblem> checkModuleId(const ModuleId& pair, const char* moduleName,
                                                              const char* idName)
{
    if (pair.module < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must not be negative", moduleName};
    if (pair.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", idName};
    return std::nullopt;
}

// Visits the ID of a pair as a reference to `target` in its module: the main Bulk Data for module 0, the part of the
// module otherwise; stores what the visitor returns. A pair that is not given (ID 0) is not visited.
inline void visitModuleId(ReferenceVisitor& visitor, std::string_view field, ReferenceTarget target, ModuleId& pair)
{
    if (pair.id <= 0)
        return;
    pair.id = pair.module == 0 ? visitor.visit(field, target, pair.id)
                               : visitor.visitInPart(field, target, pair.id, "ModuleSection", pair.module);
}

} // namespace oor::model
