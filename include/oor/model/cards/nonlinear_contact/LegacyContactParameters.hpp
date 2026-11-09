#pragma once

#include "oor/model/cards/nonlinear_contact/ContactParameters.hpp"
#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"

#include <limits>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

// The parameter lists of the legacy contact cards that no format reference describes (BCTPARM, BCTPARA): a NAME
// and a value, pair after pair. One kind of value used by two cards. Unlike the lists of BCPARA and its relatives, the
// list of names is open: a card checks the names it knows and reads any other name as a number, because the decks of
// the older contact use many more names (FRICMOD, FPARA1 to FPARA5, EPSN, EPST, PENTYP, ...). Each card has its own
// array of rules for the names it knows.
namespace oor::model
{

inline constexpr double LEGACY_NO_LOW_LIMIT = -std::numeric_limits<double>::infinity();
inline constexpr double LEGACY_NO_HIGH_LIMIT = std::numeric_limits<double>::infinity();

// What one name holds.
struct LegacyParameterRule
{
    std::string_view name;
    ContactParameterKind kind; // Integer or Real for a name the card knows
    bool blankAllowed;         // a blank value is allowed and means the default of the parameter
    double low;                // the values the parameter may take, both ends included
    double high;
};

// A parameter name: a letter, then letters, digits and underscores, up to eight characters.
[[nodiscard]] bool isLegacyParameterName(std::string_view name);

// The rule of a neutral name: the card's, or for a name it does not know a number that may not be blank and has no
// limit.
[[nodiscard]] LegacyParameterRule findLegacyRule(std::span<const LegacyParameterRule> rules, std::string_view name);

[[nodiscard]] std::vector<ContactParameter> readLegacyContactParameters(FieldReader& reader, int firstPairField,
                                                                        std::span<const LegacyParameterRule> rules,
                                                                        std::string_view owner);

// The first problem of a list of parameters: a name whose keyword is not a word, a value of the wrong kind (a real is
// not an integer), a blank that is not allowed, a value outside the limits of its rule. A name given twice is kept as
// written. The messages show the keywords of the card type `owner`.
[[nodiscard]] std::optional<CardProblem> checkLegacyContactParameters(const std::vector<ContactParameter>& parameters,
                                                                      std::span<const LegacyParameterRule> rules,
                                                                      std::string_view owner);

} // namespace oor::model
