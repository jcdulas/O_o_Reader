#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace oor::model
{

// The kind of property an ELEMUDS extends (the PTYPE field). The format names one: PBUSH2D. The word table holds the
// neutral card names, as a record writes them.
enum class UserElemServicePropertyType : std::uint8_t
{
    BushProp2D
};

inline constexpr std::array<std::string_view, 1> USER_ELEM_SERVICE_PROPERTY_WORDS{"BushProp2D"};

// Whether the user element depends on the frequency (the DEPEN field): NOM (or blank) not, FREQ yes.
enum class UserElemServiceDependence : std::uint8_t
{
    Nominal,  // NOM, the default
    Frequency // FREQ
};

inline constexpr std::array<std::string_view, 2> USER_ELEM_SERVICE_DEPENDENCE_WORDS{"NoMassDependence", "Frequency"};

// The words that open the three sections of data of an ELEMUDS.
inline constexpr std::string_view USER_ELEM_SERVICE_INTEGER_WORD = "IntegerBlock";
inline constexpr std::string_view USER_ELEM_SERVICE_REAL_WORD = "RealBlock";
inline constexpr std::string_view USER_ELEM_SERVICE_CHARACTER_WORD = "TextBlock";

// One ELEMUDS card: the link between a PBUSH2D and a user service (a routine the user supplies, called through the
// CONNECT SERVICE statement of the File Management Section) that computes the stiffness, mass and damping of its
// elements. The card carries the data the routine is given besides the ones the property has.
//
// Card layout: ELEMUDS  PID  PTYPE  GROUP  UNAME  DEPEN  NMLSPD                     fields 1 to 6
//                      "INT"   IDATA1  IDATA2  IDATA3  IDATA4  IDATA5  IDATA6  IDATA7     fields 9 to 16
//                              IDATA8  IDATA9  ...                                       fields 18 on
//                      "REAL"  RDATA1  ... (the same layout)
//                      "CHAR"  CDATA1  ... (the same layout)
// Each section begins on a line of its own with its word in the first field of the line; its data run over seven fields
// to a line, the first field of the lines after the first being blank. A section is left out when it has no data, and
// the sections may come in any order but each at most once.
//
// PID is the ID of the PBUSH2D the card extends (it must match an existing PID); the card has an ID space
// of its own, one ELEMUDS per PBUSH2D. PTYPE is PBUSH2D, the only property the format supports. GROUP is the name the
// CONNECT SERVICE statement gives the service (the two must match) and has no default. UNAME is the name of
// the user routine, truncated to eight characters; the format does not mark it required and the format asks for
// THPAD only when the external bearing interface is used, so a blank one stays blank. DEPEN is NOM when blank. NMLSPD
// is the nominal speed in cycles per second that a CBUSH2DA uses for its nominal stiffness, mass and damping; it has no
// default, so a blank one stays blank. A CDATA value cannot be one of the words INT, REAL or CHAR, which
// would start a section.
//
// The words are kept as the deck writes them: the GROUP must match the File Management Section, the UNAME and the CDATA
// are for the user routine.
struct UserElemServiceCard
{
    base::EntityId id = 0; // unique among the ELEMUDS cards; the PBUSH2D it extends
    UserElemServicePropertyType propType = UserElemServicePropertyType::BushProp2D;
    std::string groupName;
    std::string userRoutineName; // may be empty
    UserElemServiceDependence dependence = UserElemServiceDependence::Nominal;
    std::optional<double> nominalSpeed;
    std::vector<std::int32_t> integerData;
    std::vector<double> realData;
    std::vector<std::string> textData;

    [[nodiscard]] bool operator==(const UserElemServiceCard&) const noexcept = default;
};

} // namespace oor::model
