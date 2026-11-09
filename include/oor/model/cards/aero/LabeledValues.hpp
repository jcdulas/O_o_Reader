#pragma once

#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/FieldWriter.hpp"

#include <optional>
#include <span>
#include <string>
#include <vector>

// A list of (label, real) pairs in consecutive fields: the linked variables and their coefficients of an AELINK, the
// control parameters and their magnitudes of a UXVEC. One kind of value shared by two cards of the family; each card
// says where its first pair is and what its labels name. A label is the LABEL of an AESURF, AESTAT or AEPARM: one to
// eight characters that read back from every format (`checkNameWord`), kept as written and compared without regard to
// the case by whoever looks it up.
namespace oor::model
{

struct LabeledValue
{
    std::string label;
    double value = 0.0;

    [[nodiscard]] bool operator==(const LabeledValue&) const noexcept = default;
};

// Reads the pairs from field `firstField` (label) and `firstField` + 1 (value), then every two fields, up to the last
// used field. A pair with neither field is the padding of a line and is skipped; a label without a value, or a value
// without a label, is an error. `labelName` and `valueName` are the field names of the lists (`independentLabels` and
// `coefficients` for LABL1 and C1): the diagnostics show their keywords. They must be string literals.
[[nodiscard]] std::vector<LabeledValue> readLabeledValues(FieldReader& reader, int firstField, const char* labelName,
                                                          const char* valueName);

// Writes the pairs from field `firstField` on and returns the next free field.
int writeLabeledValues(FieldWriter& writer, int firstField, std::span<const LabeledValue> values);

// The first problem of a list: a label that is not a valid name, a value that is not a finite number. A list may be
// empty and may repeat a label (the card says whether that is allowed). The names are string literals.
[[nodiscard]] std::optional<CardProblem> checkLabeledValues(std::span<const LabeledValue> values, const char* labelName,
                                                            const char* valueName);

} // namespace oor::model
