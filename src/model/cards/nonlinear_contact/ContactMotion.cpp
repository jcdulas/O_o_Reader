#include "oor/model/cards/nonlinear_contact/ContactMotion.hpp"

#include <array>
#include <cmath>

namespace oor::model
{

namespace
{

// The names of the fields of the APPROV and GROW lines, one for each direction.
constexpr std::array<const char*, 3> APPROACH_AXIS_NAMES{"approachAxis1", "approachAxis2", "approachAxis3"};
constexpr std::array<const char*, 3> APPROACH_VELOCITY_NAMES{"approachVelocity1", "approachVelocity2",
                                                             "approachVelocity3"};
constexpr std::array<const char*, 3> GROWTH_FACTOR_NAMES{"growthFactor1", "growthFactor2", "growthFactor3"};
constexpr std::array<const char*, 3> GROWTH_TABLE_NAMES{"growthTable1", "growthTable2", "growthTable3"};

} // namespace

bool ContactMotion::isDefault() const noexcept
{
    const auto blank = [](const ContactValue& value) { return std::holds_alternative<std::monostate>(value); };
    return loadNode == 0 && blank(angularVelocity) && blank(axisCosines[0]) && blank(axisCosines[1]) &&
           blank(axisCosines[2]) && blank(rigidVelocities[0]) && blank(rigidVelocities[1]) && blank(rigidVelocities[2]);
}

ContactMotion readContactMotion(FieldReader& reader, int lineStart)
{
    ContactMotion motion;
    motion.loadNode = reader.integer(lineStart, "loadNode", 0);
    motion.angularVelocity = readContactValue(reader, lineStart + 1, "angularVelocity");
    motion.axisCosines[0] = readContactValue(reader, lineStart + 2, "axisCosine1");
    motion.axisCosines[1] = readContactValue(reader, lineStart + 3, "axisCosine2");
    motion.axisCosines[2] = readContactValue(reader, lineStart + 4, "axisCosine3");
    motion.rigidVelocities[0] = readContactValue(reader, lineStart + 5, "rigidVelocity1");
    motion.rigidVelocities[1] = readContactValue(reader, lineStart + 6, "rigidVelocity2");
    motion.rigidVelocities[2] = readContactValue(reader, lineStart + 7, "rigidVelocity3");
    return motion;
}

int writeContactMotion(FieldWriter& writer, int lineStart, const ContactMotion& motion)
{
    if (motion.isDefault())
        return lineStart;
    writer.integerUnlessDefault(lineStart, motion.loadNode, 0);
    writeContactValue(writer, lineStart + 1, motion.angularVelocity);
    writeContactValue(writer, lineStart + 2, motion.axisCosines[0]);
    writeContactValue(writer, lineStart + 3, motion.axisCosines[1]);
    writeContactValue(writer, lineStart + 4, motion.axisCosines[2]);
    writeContactValue(writer, lineStart + 5, motion.rigidVelocities[0]);
    writeContactValue(writer, lineStart + 6, motion.rigidVelocities[1]);
    writeContactValue(writer, lineStart + 7, motion.rigidVelocities[2]);
    return lineStart + CONTACT_LINE_FIELDS;
}

std::optional<CardProblem> checkContactMotion(const ContactMotion& motion)
{
    if (motion.loadNode < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a grid point ID, or blank",
                           "loadNode"};
    if (!isContactNumber(motion.angularVelocity))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a real or the ID of a table",
                           "angularVelocity"};
    for (const ContactValue& value : motion.axisCosines)
    {
        if (!isContactNumber(value))
            return CardProblem{"card.bad-value", "a direction cosine must be a real or the ID of a table",
                               "axisCosine1"};
    }
    for (const ContactValue& value : motion.rigidVelocities)
    {
        if (!isContactNumber(value))
            return CardProblem{"card.bad-value", "a velocity must be a real or the ID of a table", "rigidVelocity1"};
    }
    return std::nullopt;
}

ContactApproach readContactApproach(FieldReader& reader, const ContactSection& section)
{
    ContactApproach approach;
    const int first = section.first;
    approach.angularVelocity = reader.real(first + 1, "approachAngularVelocity", 0.0);
    for (int index = 0; index < 3; ++index)
    {
        approach.axis[static_cast<std::size_t>(index)] =
            reader.real(first + 2 + index, APPROACH_AXIS_NAMES[static_cast<std::size_t>(index)], 0.0);
        approach.velocity[static_cast<std::size_t>(index)] =
            reader.real(first + 5 + index, APPROACH_VELOCITY_NAMES[static_cast<std::size_t>(index)], 0.0);
    }
    return approach;
}

int writeContactApproach(FieldWriter& writer, int lineStart, const ContactApproach& approach)
{
    writer.text(lineStart, APPROV);
    writer.realUnlessDefault(lineStart + 1, approach.angularVelocity, 0.0);
    for (int index = 0; index < 3; ++index)
    {
        writer.realUnlessDefault(lineStart + 2 + index, approach.axis[static_cast<std::size_t>(index)], 0.0);
        writer.realUnlessDefault(lineStart + 5 + index, approach.velocity[static_cast<std::size_t>(index)], 0.0);
    }
    return lineStart + CONTACT_LINE_FIELDS;
}

std::optional<CardProblem> checkContactApproach(const ContactApproach& approach)
{
    bool finite = std::isfinite(approach.angularVelocity);
    for (int index = 0; index < 3; ++index)
        finite = finite && std::isfinite(approach.axis[static_cast<std::size_t>(index)]) &&
                 std::isfinite(approach.velocity[static_cast<std::size_t>(index)]);
    if (!finite)
        return CardProblem{"card.bad-value", "the values of APPROV must be finite numbers", "approachAngularVelocity"};
    return std::nullopt;
}

ContactGrowth readContactGrowth(FieldReader& reader, const ContactSection& section)
{
    ContactGrowth growth;
    const int first = section.first;
    for (int index = 0; index < 3; ++index)
    {
        growth.factors[static_cast<std::size_t>(index)] =
            reader.real(first + 1 + index, GROWTH_FACTOR_NAMES[static_cast<std::size_t>(index)], 1.0);
        growth.tables[static_cast<std::size_t>(index)] =
            reader.integer(first + 4 + index, GROWTH_TABLE_NAMES[static_cast<std::size_t>(index)], 0);
    }
    return growth;
}

int writeContactGrowth(FieldWriter& writer, int lineStart, const ContactGrowth& growth)
{
    writer.text(lineStart, GROW);
    for (int index = 0; index < 3; ++index)
    {
        writer.realUnlessDefault(lineStart + 1 + index, growth.factors[static_cast<std::size_t>(index)], 1.0);
        writer.integerUnlessDefault(lineStart + 4 + index, growth.tables[static_cast<std::size_t>(index)], 0);
    }
    return lineStart + CONTACT_LINE_FIELDS;
}

std::optional<CardProblem> checkContactGrowth(const ContactGrowth& growth)
{
    for (int index = 0; index < 3; ++index)
    {
        if (!std::isfinite(growth.factors[static_cast<std::size_t>(index)]))
            return CardProblem{"card.bad-value", "a growth factor must be a finite number", "growthFactor1"};
        if (growth.tables[static_cast<std::size_t>(index)] < 0)
            return CardProblem{"card.bad-id", "a table ID must not be negative", "growthTable1"};
    }
    return std::nullopt;
}

} // namespace oor::model
