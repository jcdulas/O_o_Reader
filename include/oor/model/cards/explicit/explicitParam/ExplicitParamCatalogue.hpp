#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/explicit/explicitParam/ExplicitParamCard.hpp"
#include "oor/model/core/CardProblem.hpp"
#include "oor/model/references/ReferenceVisitor.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

// What each name of a DYPARAM card takes, typed by hand from the 80 entries of the format reference
// (ExplicitParamCatalogueData.cpp). The catalogue describes the entries of ONE card type, the way the PARAM catalogue
// describes the PARAM names: it is not a table of the fields of several cards.
namespace oor::model
{

enum class ExplicitParamSlotKind : std::uint8_t
{
    Integer,       // a whole number
    Real,          // a real number; an integer is accepted and kept as the real of the same value
    Word,          // one of a list of words, compared without regard to the case
    IntegerOrWord, // a whole number, or one word of the list (the batch size of SMP: a number or AUTOMAT)
    Table,         // the number of a TABLED1
    Grid,          // the number of a grid point
    RigidBody      // the name of a rigid body, RigidAssembly(id) or RigidMat(id), or one word of the list (Automatic)
};

[[nodiscard]] std::string_view explicitParamSlotKindName(ExplicitParamSlotKind kind) noexcept;

// The numbers a slot accepts: the limits the format reference gives with the kind (a positive real, a real from 0.0
// to 0.15).
struct ExplicitParamRange
{
    double minimum = -std::numeric_limits<double>::infinity();
    double maximum = std::numeric_limits<double>::infinity();
    bool excludesMinimum = false; // the minimum itself is refused (a positive real)

    [[nodiscard]] bool contains(double value) const noexcept;
    [[nodiscard]] bool isLimited() const noexcept;
    // "must be greater than zero", "must be between 0 and 0.15": the end of a message that begins with the slot name.
    [[nodiscard]] std::string describe() const;
};

enum class ExplicitParamDefaultKind : std::uint8_t
{
    None,     // the field may be blank and the format reference states no value for it
    Required, // the field must be given
    Integer,  // blank is this integer
    Real,     // blank is this real
    Word,     // blank is this word
    SameAs,   // blank is the value of another slot of the entry (NBY is NBX)
    Computed // blank is a value the format reference derives from other data (a fraction of another parameter, a solver
             // choice)
};

struct ExplicitParamDefault
{
    ExplicitParamDefaultKind kind = ExplicitParamDefaultKind::None;
    std::int32_t integerValue = 0;
    double realValue = 0.0;
    std::string_view word;
    std::size_t slot = 0; // SameAs: the index of the slot whose value stands in
};

// One value of an entry: what it holds, the field of the format reference (VALUE, NSUBMAX, ...) and what a blank means.
// The format reference gives many entries the same generic field names (VALUE, OPTION1); the neutral name says what the
// value is for its entry (outputInterval for the VALUE of ATBAOUT, transportScheme for the OPTION1 of EULTRAN).
struct ExplicitParamSlot
{
    const char* name = ""; // the neutral name, camelCase: the field a reference visitor is given
    ExplicitParamSlotKind kind = ExplicitParamSlotKind::Real;
    ExplicitParamRange range; // Integer, Real and IntegerOrWord
    std::string_view words;   // the allowed words separated by '|' ("YES|NO"); empty: any word
    ExplicitParamDefault defaultValue;

    [[nodiscard]] bool required() const noexcept { return defaultValue.kind == ExplicitParamDefaultKind::Required; }
    [[nodiscard]] std::vector<std::string_view> allowedWords() const;
};

// One layout of the values of an entry. Most names have one; CONTACT has one per option and SMP one per keyword.
struct ExplicitParamForm
{
    std::string_view selector; // the word that picks the form among those of its name; empty for the only form
    std::vector<ExplicitParamSlot> slots;  // the values in the order of the fields from field 2
    std::optional<ExplicitParamSlot> tail; // a list that goes on after the slots: every further value is one of these
    std::size_t tailMinimum = 0;           // the number of values the list needs at least
};

// A name of the format reference chapter: the sub-entry (or the sub-entries, when it has several forms).
struct ExplicitParamEntry
{
    std::string_view name; // the neutral name (OccupantAccelerationOutput for ATBAOUT)
    // An entry with several forms: the index of the value that holds the selector word (0 is field 2) and the format
    // reference name of that value. -1 for an entry with one form.
    int selectorIndex = -1;
    const char* selectorName = "";
    // How many leading values, with the name, tell which setting a card gives (CONTACT: the option; SMP: the entity
    // and the keyword). Two cards of the same setting are a repeat (see explicitParamIdentity). -1: the entry may be
    // given any number of times (MATRMERG merges one group of rigid bodies per card).
    int identityValues = 0;
    std::vector<ExplicitParamForm> forms;
};

// The names of the format reference and what is known of each. A name the catalogue does not know is a valid DYPARAM
// all the same: the card keeps its values as written.
class ExplicitParamCatalogue
{
public:
    [[nodiscard]] static const ExplicitParamCatalogue& instance();

    // The entries in the order of the format reference.
    [[nodiscard]] std::span<const ExplicitParamEntry> entries() const noexcept { return m_entries; }

    // The entry of a neutral DYPARAM name, nullptr when the format reference has none.
    [[nodiscard]] const ExplicitParamEntry* find(std::string_view name) const;

    // The form of `entry` that a card with these values follows: the only one, or the one whose selector is the word at
    // the entry's selector place. nullptr when that word is missing or is not the selector of any form.
    [[nodiscard]] static const ExplicitParamForm* formOf(const ExplicitParamEntry& entry,
                                                         std::span<const ExplicitParamValue> values);

    // The slot that value `index` of the card fills (a slot of the form, or the tail); nullptr for a name the catalogue
    // does not know, a form that cannot be chosen and a value beyond the layout.
    [[nodiscard]] const ExplicitParamSlot* slotOf(const ExplicitParamCard& card, std::size_t index) const;

    // What is wrong with the values of a card of a known name, in a sentence with the format reference name of the
    // field; nullopt when they fit or the name is not in the catalogue.
    [[nodiscard]] std::optional<CardProblem> problemWith(const ExplicitParamCard& card) const;

private:
    ExplicitParamCatalogue();

    const std::vector<ExplicitParamEntry>& m_entries;
    std::vector<std::size_t> m_byName; // indexes of the entries, sorted by name
};

// The card as the reader gives it and as a built card should be: a name the catalogue does not know in upper case, a
// word of its slot in the spelling of the catalogue (a neutral name) and any other text in upper case, the blank fields
// after the last value dropped, an integer given where the entry takes a real turned into the real.
void normaliseExplicitParam(ExplicitParamCard& card);

// The word of a slot that a text is, whatever its case, in the spelling of the catalogue; nullopt for any other text.
[[nodiscard]] std::optional<std::string_view> slotWord(const ExplicitParamSlot& slot, std::string_view text);

// The values of the card with every blank slot filled by its default (nothing for a slot with no default or a computed
// one). The vector has at least one value for each slot of the form; the values of a name the catalogue does not know
// are returned as written.
[[nodiscard]] std::vector<ExplicitParamValue> resolveExplicitParamValues(const ExplicitParamCard& card);

// The text that says which setting of its entry a card gives ("CONTACT,FORCE", "SMP,BEAMS,BATCHSIZ", "HICGRAV");
// nullopt for a name the catalogue does not know and for an entry that may be repeated.
[[nodiscard]] std::optional<std::string> explicitParamIdentity(const ExplicitParamCard& card);

// A value that names another card.
struct ExplicitParamReference
{
    ReferenceTarget target;
    base::EntityId id;
};

// What value `index` of the card refers to: the TABLED1 of a Table slot, the grid point of a Grid slot and the rigid
// material of an RigidMat(id) rigid body name. An RigidAssembly(id) name is not a reference: the new assembly of a
// RigidBodyMerge does not exist yet, and the others may be assemblies that an earlier card creates.
[[nodiscard]] std::optional<ExplicitParamReference> explicitParamReference(const ExplicitParamCard& card,
                                                                           std::size_t index);

// Visits every reference of the card and stores the number the visitor returns (the name of a rigid body keeps its
// prefix). The field names are the neutral names of the slots (maxTimeTable, monitoredGrid, mergedBodyNames).
void visitExplicitParamReferences(ExplicitParamCard& card, ReferenceVisitor& visitor);

} // namespace oor::model
