#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace oor::model
{

// The value of a parameter, an integer or a real as the deck wrote it. A parameter that the catalogue says is real
// takes 15 as well as 15.0 (an integer is a real that is whole), and the card keeps what was written.
struct ConnectorSearchParamsValue
{
    std::variant<std::int32_t, double> number = std::int32_t{0};

    [[nodiscard]] bool isInteger() const noexcept { return std::holds_alternative<std::int32_t>(number); }
    // The value as a real, whichever it was written as.
    [[nodiscard]] double asReal() const noexcept
    {
        return isInteger() ? static_cast<double>(std::get<std::int32_t>(number)) : std::get<double>(number);
    }

    [[nodiscard]] bool operator==(const ConnectorSearchParamsValue&) const noexcept = default;
};

// A parameter and its value.
struct ConnectorSearchParamsParameter
{
    std::string name; // upper case
    ConnectorSearchParamsValue value;

    [[nodiscard]] bool operator==(const ConnectorSearchParamsParameter&) const noexcept = default;
};

// The keywords that start a section of the card: the parameters after one apply to that type of connector element
// only, until the next keyword.
enum class ConnectorSearchParamsElement : std::uint8_t
{
    Fastener,
    Weld,
    Seam
};

inline constexpr std::array<std::string_view, 3> CONNECTOR_SEARCH_ELEMENT_WORDS{"Fastener", "Weld", "Seam"};

// The parameters after one keyword.
struct ConnectorSearchParamsSection
{
    ConnectorSearchParamsElement element = ConnectorSearchParamsElement::Fastener;
    std::vector<ConnectorSearchParamsParameter> parameters;

    [[nodiscard]] bool operator==(const ConnectorSearchParamsSection&) const noexcept = default;
};

// One SWLDPRM card: values for the parameters of the connectivity search of the connector elements CFAST, CWELD and
// CSEAM (`ConnectorSearchParamsCatalogue.hpp`: the kind, range and default of
// each).
//
// Card layout: SWLDPRM  PARAM1  VAL1  PARAM2  VAL2  PARAM3  VAL3  PARAM4  VAL4        fields 1 to 8
//                      PARAM5  VAL5  -etc.-                                           fields 9 to 16, and so on
// and, with the keywords, the same run of name and value pairs in which a field that holds CFAST, CWELD or CSEAM starts
// a section (the keyword takes one field and the next pair follows it). The parameters before the first keyword are
// global (`global`); those after a keyword apply to that element type only (`sections`, in the order of the card). A
// parameter that does not concern the element type of its section is ignored by the solver, so it is kept.
// Blank fields between the pairs are allowed for readability and are not kept; a name is followed in the next field by
// its value. The format reference says none of the parameters is required, so a card may have none.
//
// A name that the catalogue does not list is kept with its value as written: the format reference says the allowable
// names are the table's, but its own text names SKIN, which the table lacks, and a later release may add names. A
// listed name takes the values the table gives it (the card is refused otherwise), and CHKRUN is only valid before any
// keyword. Only one SWLDPRM card is allowed in the Bulk Data (a rule between cards, not checked
// here).
struct ConnectorSearchParamsCard
{
    std::vector<ConnectorSearchParamsParameter> global;
    std::vector<ConnectorSearchParamsSection> sections;
};

} // namespace oor::model
