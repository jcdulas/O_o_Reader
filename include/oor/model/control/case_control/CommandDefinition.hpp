#pragma once

#include "oor/model/control/case_control/Subcase.hpp"
#include "oor/model/references/ReferenceVisitor.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace oor::model
{

// The syntax of a Case Control command: what follows its name.
enum class CommandShape : std::uint8_t
{
    Integer,          // NAME = n                         (SPC = 10)
    IntegerOrKeyword, // NAME = n | WORD                  (SUPER = ALL)
    Real,             // NAME = x                         (XPAPER = 29.)
    Keyword,          // NAME = WORD                      (ANALYSIS = FLUTTER)
    Text,             // NAME = the rest of the line      (TITLE = a plate)
    IntegerList,      // NAME = n1, n2, ...               (SEPLOT 0,1,2)
    RealList,         // NAME = x1, x2, ...               (SUBSEQ = 1.0,-0.5)
    Output,           // NAME(options) = ALL | NONE | n   (an output request; the value may be missing)
    Flag,             // NAME or NAME(options), no value  (AUXCASE)
    SubcaseHeader,    // SUBCASE n, SUBCOM n, SYM n, SYMCOM n, REPCASE n, STEP n, SUBSTEP n
    SetDefinition,    // SET n = members
    Parameter,        // PARAM,NAME,value
    BlockHeader       // OUTPUT(PLOT), OUTPUT(POST), OUTPUT(XYPLOT), SETS DEFINITION
};

// A word of an IntegerOrKeyword command that may be followed by a comma and one of `qualifiers`: BCONTACT = AUTO,PGLUE
// (format reference BCONTACT: "AUTO(,ctype)"). The value is the keyword with its qualifier, "AUTO,PGLUE".
struct QualifiedWord
{
    std::string word;
    std::vector<std::string> qualifiers;
};

struct CommandDefinition
{
    std::string name;
    CommandShape shape = CommandShape::Text;
    std::vector<std::string> variants;
    std::vector<std::string> keywords; // Keyword shape: the words the format reference lists, when it lists them all
    std::vector<QualifiedWord> qualifiedWords;      // IntegerOrKeyword shape: the words that take a qualifier
    std::optional<ReferenceTarget> reference;       // Integer shape: what the number is the ID of
    SubcaseKind subcaseKind = SubcaseKind::Subcase; // SubcaseHeader shape
    std::string blockKind;                          // BlockHeader shape: PLOT, POST, XYPLOT
    bool isBlockCommand =
        false; // a plot or X-Y plot command: it belongs to the OUTPUT(PLOT/POST/XYPLOT) block before it
    // IntegerOrKeyword shape: the integer may be followed by a comma and a load sequence number (SUPER = 17, 3), kept
    // as the one item of the value's `integers`.
    bool takesLoadSequence = false;

    CommandDefinition& variant(std::string option);
    CommandDefinition& words(std::vector<std::string> allowed);
    CommandDefinition& qualifiedWord(std::string word, std::vector<std::string> qualifiers);
    CommandDefinition& refersTo(ReferenceTarget target);
    CommandDefinition& inBlock();
    CommandDefinition& ofSubcaseKind(SubcaseKind kind);
    CommandDefinition& ofBlockKind(std::string kind);
    CommandDefinition& withLoadSequence();

    // True when the option (its neutral name) selects a variant of the command.
    [[nodiscard]] bool hasVariant(std::string_view option) const;
    // The shape holds one value that is a Bulk Data set ID (the references the model resolves and renumbers).
    [[nodiscard]] bool holdsReference() const noexcept { return reference.has_value(); }
};

} // namespace oor::model
