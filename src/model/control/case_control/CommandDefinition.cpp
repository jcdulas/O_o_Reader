#include "oor/model/control/case_control/CommandDefinition.hpp"

#include "oor/base/text/TextUtilities.hpp"

namespace oor::model
{

CommandDefinition& CommandDefinition::variant(std::string option)
{
    variants.push_back(std::move(option));
    return *this;
}

CommandDefinition& CommandDefinition::words(std::vector<std::string> allowed)
{
    keywords = std::move(allowed);
    return *this;
}

CommandDefinition& CommandDefinition::qualifiedWord(std::string word, std::vector<std::string> qualifiers)
{
    qualifiedWords.push_back(QualifiedWord{std::move(word), std::move(qualifiers)});
    return *this;
}

CommandDefinition& CommandDefinition::withLoadSequence()
{
    takesLoadSequence = true;
    return *this;
}

CommandDefinition& CommandDefinition::refersTo(ReferenceTarget target)
{
    reference = target;
    return *this;
}

CommandDefinition& CommandDefinition::inBlock()
{
    isBlockCommand = true;
    return *this;
}

CommandDefinition& CommandDefinition::ofSubcaseKind(SubcaseKind kind)
{
    subcaseKind = kind;
    return *this;
}

CommandDefinition& CommandDefinition::ofBlockKind(std::string kind)
{
    blockKind = std::move(kind);
    return *this;
}

bool CommandDefinition::hasVariant(std::string_view option) const
{
    for (const std::string& candidate : variants)
    {
        if (candidate == option)
            return true;
    }
    return false;
}

} // namespace oor::model
