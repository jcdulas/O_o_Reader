#pragma once

#include "oor/base/text/NumberFormatter.hpp"
#include "oor/model/format/FieldValue.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace oor::model
{

// Collects typed values in internal card slots; CardLayouts compacts them into neutral model records.
class FieldWriter
{
public:
    // omitDefaults: the *UnlessDefault writers write a blank when the value is the default (static defaults only: a
    // field whose default depends on another field must be written with integer()/real()).
    // significantDigits: 0 writes roundtrip-exact reals; N rounds to at most N significant digits.
    // compressRanges: a card that lists IDs may write a run of consecutive IDs as "first Through last" (only the cards
    // that have a range form look at it). governedFieldsExplicit: the card's type has fields that another card of the
    // model gives a default to when a model is read (CardTable::fieldsGovernedByDefaults): the card writes those fields
    // even when they hold the default value or zero, so that reading the model again does not give them the default.
    explicit FieldWriter(bool omitDefaults = true, int significantDigits = 0, bool compressRanges = false,
                         bool governedFieldsExplicit = false) noexcept
        : m_omitDefaults(omitDefaults),
          m_significantDigits(significantDigits),
          m_compressRanges(compressRanges),
          m_governedFieldsExplicit(governedFieldsExplicit)
    {
    }

    void integer(int field, std::int64_t value);
    void real(int field, double value);
    void text(int field, std::string_view value);
    // Reserves an internal slot span while keeping the complete text in its first slot.
    // CardLayouts emits the text as one compact record value.
    void spanText(int firstField, int fieldCount, std::string_view value);
    void integerUnlessDefault(int field, std::int64_t value, std::int64_t defaultValue);
    void realUnlessDefault(int field, double value, double defaultValue);
    void textUnlessDefault(int field, std::string_view value, std::string_view defaultValue);
    // A named numeric option, formatted as exact neutral name=value text.
    void namedReal(int field, std::string_view name, double value);

    // True when the *UnlessDefault writers leave a default value blank. A card that lays out optional fields one
    // after the other (LanczosEigenMethod options) needs it to know which of them are written.
    [[nodiscard]] bool omitsDefaults() const noexcept { return m_omitDefaults; }
    // A lossless write keeps the sign of a zero: -0.0 is written "-0." and is not the default 0.0, so
    // that reading the model again gives the same bits.
    void keepSignOfZero(bool keep) noexcept { m_signOfZero = keep; }
    [[nodiscard]] bool compressesRanges() const noexcept { return m_compressRanges; }
    // True when every field supplied by a defaults card (such as NodeDefaults or Bar2Defaults) must be written
    // explicitly, whatever its value.
    [[nodiscard]] bool governedFieldsExplicit() const noexcept { return m_governedFieldsExplicit; }

    // The number of fields (up to the last one written).
    [[nodiscard]] int fieldCount() const noexcept { return static_cast<int>(m_slots.size()); }
    // True when an internal slot holds an integer or a real. Text that contains digits remains text.
    [[nodiscard]] bool holdsNumber(int field) const noexcept;
    // True when an internal slot holds a real.
    [[nodiscard]] bool holdsReal(int field) const noexcept;

    // Internal slot texts with standard decimal reals and the optional significant-digit precision.
    [[nodiscard]] std::vector<std::string> fields() const;
    [[nodiscard]] std::span<const FieldValue> typedFields() const noexcept { return m_slots; }

private:
    using Slot = FieldValue;

    Slot& slot(int field);

    bool m_omitDefaults;
    int m_significantDigits;
    bool m_signOfZero = false;
    bool m_compressRanges;
    bool m_governedFieldsExplicit;
    std::vector<Slot> m_slots;
};

} // namespace oor::model
