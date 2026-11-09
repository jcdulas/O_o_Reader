#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/FieldWriter.hpp"
#include "oor/model/references/ReferenceVisitor.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

// One design variable of a linear relation and the coefficient it is multiplied by: DVIDi and COEFi of a DVPREL1, a
// DVMREL1 or a DVCREL1, IDVi and Ci of a DLINK.
struct DesignTerm
{
    base::EntityId designVarId = 0;
    double coefficient = 0.0;

    [[nodiscard]] bool operator==(const DesignTerm&) const noexcept = default;
};

// The terms from `firstField` to the end of the card, two fields each (the DVID and the coefficient; the pairs go on
// over the continuation lines with no break: field 9 follows field 8). A pair with both fields blank is skipped, one
// with only one of them is an error. With `allowFromProperty` the coefficient of the first term may be the word PVAL,
// which tells the solver to take that coefficient from the value the property has in the model:
// the term then has coefficient 0 and `fromProperty` is set.
[[nodiscard]] std::vector<DesignTerm> readDesignTerms(FieldReader& reader, int firstField, const char* idName,
                                                      const char* coefficientName, bool allowFromProperty,
                                                      bool& fromProperty);

// Writes the terms the way readDesignTerms reads them.
void writeDesignTerms(FieldWriter& writer, int firstField, const std::vector<DesignTerm>& terms, bool fromProperty);

// The problem with the terms of a card, if there is one: no term at all, a DVID that is not positive, a coefficient
// that is not finite, and with `distinct` a design variable that appears twice (DLINK does not allow it, the relations
// do).
[[nodiscard]] std::optional<CardProblem> checkDesignTerms(const std::vector<DesignTerm>& terms, bool fromProperty,
                                                          bool distinct, const char* idName,
                                                          const char* coefficientName);

// Visits the design variable of each term.
void visitDesignTerms(ReferenceVisitor& visitor, std::vector<DesignTerm>& terms, const char* idName);

} // namespace oor::model
