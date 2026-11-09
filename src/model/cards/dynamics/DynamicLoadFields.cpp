#include "oor/model/cards/dynamics/DynamicLoadFields.hpp"

#include "oor/base/text/NumberParser.hpp"
#include "oor/base/text/TextUtilities.hpp"

#include <string>
#include <variant>

namespace oor::model
{

namespace
{

struct NamedType
{
    const char* word;
    ExcitationType type;
};
constexpr NamedType NAMED_TYPES[] = {{"Load", ExcitationType::Load},
                                     {"Displacement", ExcitationType::Displacement},
                                     {"Velocity", ExcitationType::Velocity},
                                     {"Acceleration", ExcitationType::Acceleration}};

constexpr ExcitationType CODED_TYPES[] = {ExcitationType::Load,          ExcitationType::Displacement,
                                          ExcitationType::Velocity,      ExcitationType::Acceleration,
                                          ExcitationType::Flow,          ExcitationType::RigidBodyVelocity,
                                          ExcitationType::RigidBodyForce};

} // namespace

std::optional<ExcitationType> parseExcitationType(std::string_view text)
{
    text = base::trimAscii(text);
    if (text.empty())
        return std::nullopt;
    if (const auto code = base::NumberParser::parseInteger(text))
    {
        for (const ExcitationType type : CODED_TYPES)
        {
            if (static_cast<std::int64_t>(type) == *code)
                return type;
        }
        return std::nullopt;
    }
    for (const NamedType& entry : NAMED_TYPES)
    {
        const std::string_view word = entry.word;
        if (text == word)
            return entry.type;
    }
    return std::nullopt;
}

const char* excitationTypeWord(ExcitationType type) noexcept
{
    for (const NamedType& entry : NAMED_TYPES)
    {
        if (entry.type == type)
            return entry.word;
    }
    return "";
}

ExcitationType readExcitationType(FieldReader& reader, int field)
{
    const std::string_view text = reader.text(field);
    if (text.empty())
        return ExcitationType::Load;
    if (const auto type = parseExcitationType(text))
        return *type;
    reader.error("card.bad-value",
                 std::string("kind") +
                     " must be 0 to 3, 4, 12, 13 or Load, Displacement, Velocity, Acceleration, found \"" +
                     std::string(text) + "\"",
                 "kind");
    return ExcitationType::Load;
}

void writeExcitationType(FieldWriter& writer, int field, ExcitationType type)
{
    if (type == ExcitationType::Load)
    {
        writer.textUnlessDefault(field, "Load", "Load");
        return;
    }
    if (const std::string_view word = excitationTypeWord(type); !word.empty())
        writer.text(field, word);
    else
        writer.integer(field, static_cast<std::int64_t>(type));
}

ReferenceTarget excitationTarget(ExcitationType type) noexcept
{
    return type == ExcitationType::Load ? ReferenceTarget::ExcitationSet : ReferenceTarget::Other;
}

base::EntityId visitIfSet(ReferenceVisitor& visitor, const char* field, ReferenceTarget target, base::EntityId id)
{
    return id != 0 ? visitor.visit(field, target, id) : id;
}

void readIdOrReal(FieldReader& reader, int field, const char* idName, base::EntityId& id, double& value)
{
    id = 0;
    value = 0.0;
    const auto given = reader.integerOrReal(field, idName);
    if (!given)
        return;
    if (const auto* integerValue = std::get_if<std::int32_t>(&*given))
        id = *integerValue;
    else
        value = std::get<double>(*given);
}

void writeIdOrReal(FieldWriter& writer, int field, base::EntityId id, double value)
{
    if (id != 0)
        writer.integer(field, id);
    else
        writer.realUnlessDefault(field, value, 0.0);
}

std::optional<CardProblem> checkIdOrReal(base::EntityId id, double value, const char* idName)
{
    if (id < 0)
        return CardProblem{"card.bad-id", "an ID must be a positive integer", idName};
    if (id != 0 && value != 0.0)
        return CardProblem{"card.bad-value", "the field holds an ID or a real, not both", idName};
    return std::nullopt;
}

} // namespace oor::model
