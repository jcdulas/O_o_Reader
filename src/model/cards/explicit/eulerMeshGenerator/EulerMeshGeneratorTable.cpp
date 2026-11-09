#include "oor/model/cards/explicit/eulerMeshGenerator/EulerMeshGeneratorTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <algorithm>
#include <array>
#include <string>

namespace oor::model
{

namespace
{

// The last field of the card: eight lines of eight fields.
constexpr int LAST_FIELD = 64;

// A field of a group that has to be given together, with its format reference name.
struct GivenField
{
    bool given;
    const char* name;
};

// The name of the first field of the group that is blank, or nullptr when all are given.
const char* firstMissing(const std::array<GivenField, 3>& group)
{
    for (const GivenField& field : group)
    {
        if (!field.given)
            return field.name;
    }
    return nullptr;
}

bool anyGiven(const std::array<GivenField, 3>& group)
{
    return std::any_of(group.begin(), group.end(), [](const GivenField& field) { return field.given; });
}

CardProblem missingField(const char* field, const std::string& message)
{
    return CardProblem{"card.missing-field", message, field};
}

CardProblem badValue(const char* field, const std::string& message)
{
    return CardProblem{"card.bad-value", message, field};
}

// A count that may be blank: not below `minimum`.
std::optional<CardProblem> checkOptionalCount(const std::optional<std::int32_t>& value, const char* field,
                                              std::int32_t minimum)
{
    if (value && *value < minimum)
        return badValue(field, std::string(CardProblem::FIELD) + " must be at least " + std::to_string(minimum));
    return std::nullopt;
}

// Which of the sizes, widths and counts a mesh must give.
std::optional<CardProblem> checkSizes(const EulerMeshGeneratorCard& card)
{
    const std::array<GivenField, 3> sizes{{{card.elemSizeX.has_value(), "elemSizeX"},
                                           {card.elemSizeY.has_value(), "elemSizeY"},
                                           {card.elemSizeZ.has_value(), "elemSizeZ"}}};
    const std::array<GivenField, 3> widths{{{card.widthX.has_value(), "widthX"},
                                            {card.widthY.has_value(), "widthY"},
                                            {card.widthZ.has_value(), "widthZ"}}};
    const std::array<GivenField, 3> counts{{{card.elemCountX.has_value(), "elemCountX"},
                                            {card.elemCountY.has_value(), "elemCountY"},
                                            {card.elemCountZ.has_value(), "elemCountZ"}}};
    const bool sizesGiven = firstMissing(sizes) == nullptr;
    const bool widthsGiven = firstMissing(widths) == nullptr;
    const bool countsGiven = firstMissing(counts) == nullptr;

    if (card.type == EulerMeshGeneratorType::Box)
    {
        if (!countsGiven)
            return missingField(firstMissing(counts), "a BOX mesh needs " + CardProblem::fieldNamed("elemCountX") +
                                                          ", " + CardProblem::fieldNamed("elemCountY") + " and " +
                                                          CardProblem::fieldNamed("elemCountZ"));
        if (!sizesGiven && !widthsGiven)
            return missingField(anyGiven(widths) ? firstMissing(widths) : firstMissing(sizes),
                                "a BOX mesh needs (" + CardProblem::fieldNamed("elemSizeX") + ", " +
                                    CardProblem::fieldNamed("elemSizeY") + ", " + CardProblem::fieldNamed("elemSizeZ") +
                                    ") or (" + CardProblem::fieldNamed("widthX") + ", " +
                                    CardProblem::fieldNamed("widthY") + ", " + CardProblem::fieldNamed("widthZ") + ")");
        return std::nullopt;
    }
    if (sizesGiven || (widthsGiven && countsGiven))
        return std::nullopt;
    const std::string message =
        "an ADAPT mesh needs (" + CardProblem::fieldNamed("elemSizeX") + ", " + CardProblem::fieldNamed("elemSizeY") +
        ", " + CardProblem::fieldNamed("elemSizeZ") + "), or (" + CardProblem::fieldNamed("widthX") + ", " +
        CardProblem::fieldNamed("widthY") + ", " + CardProblem::fieldNamed("widthZ") + ") with (" +
        CardProblem::fieldNamed("elemCountX") + ", " + CardProblem::fieldNamed("elemCountY") + ", " +
        CardProblem::fieldNamed("elemCountZ") + ")";
    if (anyGiven(widths) || anyGiven(counts))
        return missingField(!widthsGiven ? firstMissing(widths) : firstMissing(counts), message);
    return missingField(firstMissing(sizes), message);
}

// The field GROWi: a biased mesh gives the grow factor of each direction unless a BIAS card of that
// direction overrules it.
std::optional<CardProblem> checkBias(const EulerMeshGeneratorCard& card)
{
    if (!card.bias)
        return std::nullopt;
    if (!card.growthX && card.biasIdX == 0)
        return missingField("growthX", std::string(CardProblem::FIELD) + " is required when " +
                                           CardProblem::fieldNamed("biasStart") + " is given (and " +
                                           CardProblem::fieldNamed("biasIdX") + " is blank)");
    if (!card.growthY && card.biasIdY == 0)
        return missingField("growthY", std::string(CardProblem::FIELD) + " is required when " +
                                           CardProblem::fieldNamed("biasStart") + " is given (and " +
                                           CardProblem::fieldNamed("biasIdY") + " is blank)");
    if (!card.growthZ && card.biasIdZ == 0)
        return missingField("growthZ", std::string(CardProblem::FIELD) + " is required when " +
                                           CardProblem::fieldNamed("biasStart") + " is given (and " +
                                           CardProblem::fieldNamed("biasIdZ") + " is blank)");
    return std::nullopt;
}

// The auxiliary output box is a start point and three widths: all six values, or none.
std::optional<CardProblem> checkOutputBox(const EulerMeshGeneratorCard& card)
{
    const std::array<GivenField, 6> fields{{{card.boxOriginX.has_value(), "boxOriginX"},
                                            {card.boxOriginY.has_value(), "boxOriginY"},
                                            {card.boxOriginZ.has_value(), "boxOriginZ"},
                                            {card.boxWidthX.has_value(), "boxWidthX"},
                                            {card.boxWidthY.has_value(), "boxWidthY"},
                                            {card.boxWidthZ.has_value(), "boxWidthZ"}}};
    const bool any = std::any_of(fields.begin(), fields.end(), [](const GivenField& field) { return field.given; });
    if (!any)
        return std::nullopt;
    for (const GivenField& field : fields)
    {
        if (!field.given)
            return missingField(field.name, "the auxiliary box needs " + CardProblem::fieldNamed("boxOriginX") + ", " +
                                                CardProblem::fieldNamed("boxOriginY") + ", " +
                                                CardProblem::fieldNamed("boxOriginZ") + ", " +
                                                CardProblem::fieldNamed("boxWidthX") + ", " +
                                                CardProblem::fieldNamed("boxWidthY") + " and " +
                                                CardProblem::fieldNamed("boxWidthZ") + " together");
    }
    return std::nullopt;
}

// A cube count that is a multiple of the partition count of its direction (PROCDIR = USER).
std::optional<CardProblem> checkMultiple(const std::optional<std::int32_t>& cubes,
                                         const std::optional<std::int32_t>& blocks, const char* field,
                                         const char* partner)
{
    if (cubes && blocks && *blocks > 0 && *cubes % *blocks != 0)
        return badValue(field, std::string(CardProblem::FIELD) + " must be a multiple of " +
                                   CardProblem::fieldNamed(partner) + " for " +
                                   CardProblem::fieldNamed("partitionDirection") + "=USER");
    return std::nullopt;
}

// The rules about the cubes and their distribution over the processors.
std::optional<CardProblem> checkCubes(const EulerMeshGeneratorCard& card)
{
    // NBY and NBZ default to NBX, NPY and NPZ to NPX: the card holds the resolved values, so that it writes and reads
    // back as the same card.
    if (card.cubeCountX && !card.cubeCountY)
        return missingField("cubeCountY", std::string(CardProblem::FIELD) + " takes the value of " +
                                              CardProblem::fieldNamed("cubeCountX") +
                                              " when blank, so the card must hold it");
    if (card.cubeCountX && !card.cubeCountZ)
        return missingField("cubeCountZ", std::string(CardProblem::FIELD) + " takes the value of " +
                                              CardProblem::fieldNamed("cubeCountX") +
                                              " when blank, so the card must hold it");
    if (card.partitionCountX && !card.partitionCountY)
        return missingField("partitionCountY", std::string(CardProblem::FIELD) + " takes the value of " +
                                                   CardProblem::fieldNamed("partitionCountX") +
                                                   " when blank, so the card must hold it");
    if (card.partitionCountX && !card.partitionCountZ)
        return missingField("partitionCountZ", std::string(CardProblem::FIELD) + " takes the value of " +
                                                   CardProblem::fieldNamed("partitionCountX") +
                                                   " when blank, so the card must hold it");
    if (card.partitionDirection != EulerMeshGeneratorPartition::User)
        return std::nullopt;
    if (!card.partitionCountX)
        return missingField("partitionCountX", std::string(CardProblem::FIELD) + " is required for " +
                                                   CardProblem::fieldNamed("partitionDirection") + "=USER");
    if (const auto problem = checkMultiple(card.cubeCountX, card.partitionCountX, "cubeCountX", "partitionCountX"))
        return problem;
    if (const auto problem = checkMultiple(card.cubeCountY, card.partitionCountY, "cubeCountY", "partitionCountY"))
        return problem;
    return checkMultiple(card.cubeCountZ, card.partitionCountZ, "cubeCountZ", "partitionCountZ");
}

// The rules that tie the fields of a mesh to its TYPE and to each other.
std::optional<CardProblem> checkCombinations(const EulerMeshGeneratorCard& card)
{
    if (card.type == EulerMeshGeneratorType::Box)
    {
        // The origin defaults to the reference point, which the card holds resolved.
        if (!card.originX)
            return missingField("originX", std::string(CardProblem::FIELD) +
                                               " is required for a BOX mesh (it defaults to " +
                                               CardProblem::fieldNamed("xReference") + " when the card is read)");
        if (!card.originY)
            return missingField("originY", std::string(CardProblem::FIELD) +
                                               " is required for a BOX mesh (it defaults to " +
                                               CardProblem::fieldNamed("yReference") + " when the card is read)");
        if (!card.originZ)
            return missingField("originZ", std::string(CardProblem::FIELD) +
                                               " is required for a BOX mesh (it defaults to " +
                                               CardProblem::fieldNamed("zReference") + " when the card is read)");
    }
    if (const auto problem = checkSizes(card))
        return problem;
    if (card.resize != EulerMeshGeneratorResize::None)
    {
        if (card.type != EulerMeshGeneratorType::Adapt)
            return badValue("resize", std::string(CardProblem::FIELD) + " other than NONE is only valid for " +
                                          CardProblem::fieldNamed("meshType") + "=ADAPT");
        if (card.resizeTableIdX == 0)
            return missingField("resizeTableIdX", std::string(CardProblem::FIELD) + " is required when " +
                                                      CardProblem::fieldNamed("resize") + " is SCALE or LENGTH");
    }
    if (card.resizeTableIdX != 0 && card.resizeTableIdY == 0)
        return missingField("resizeTableIdY", std::string(CardProblem::FIELD) + " takes the value of " +
                                                  CardProblem::fieldNamed("resizeTableIdX") +
                                                  " when blank, so the card must hold it");
    if (card.resizeTableIdX != 0 && card.resizeTableIdZ == 0)
        return missingField("resizeTableIdZ", std::string(CardProblem::FIELD) + " takes the value of " +
                                                  CardProblem::fieldNamed("resizeTableIdX") +
                                                  " when blank, so the card must hold it");
    // A box placed with a coordinate system cannot be an adaptive mesh.
    if (card.frameId != 0 && card.type == EulerMeshGeneratorType::Adapt)
        return badValue("frameId", std::string(CardProblem::FIELD) + " cannot be used with " +
                                       CardProblem::fieldNamed("meshType") + "=ADAPT");
    if (const auto problem = checkBias(card))
        return problem;
    if (const auto problem = checkOutputBox(card))
        return problem;
    return checkCubes(card);
}

} // namespace

void EulerMeshGeneratorFormat::resolveDefaults(EulerMeshGeneratorCard& card)
{
    if (card.type == EulerMeshGeneratorType::Box)
    {
        if (!card.originX)
            card.originX = card.xReference;
        if (!card.originY)
            card.originY = card.yReference;
        if (!card.originZ)
            card.originZ = card.zReference;
    }
    if (card.resizeTableIdX != 0)
    {
        if (card.resizeTableIdY == 0)
            card.resizeTableIdY = card.resizeTableIdX;
        if (card.resizeTableIdZ == 0)
            card.resizeTableIdZ = card.resizeTableIdX;
    }
    if (card.cubeCountX)
    {
        if (!card.cubeCountY)
            card.cubeCountY = card.cubeCountX;
        if (!card.cubeCountZ)
            card.cubeCountZ = card.cubeCountX;
    }
    if (card.partitionCountX)
    {
        if (!card.partitionCountY)
            card.partitionCountY = card.partitionCountX;
        if (!card.partitionCountZ)
            card.partitionCountZ = card.partitionCountX;
    }
}

std::optional<EulerMeshGeneratorCard> EulerMeshGeneratorFormat::read(FieldReader& reader)
{
    EulerMeshGeneratorCard card;
    // Fields 1 to 8
    card.id = reader.requiredInteger(1, "id");
    card.type = readRequiredKeyword<EulerMeshGeneratorType>(reader, 2, "meshType", EULER_MESH_TYPE_WORDS);
    card.elemSizeX = reader.optionalReal(3, "elemSizeX");
    card.elemSizeY = reader.optionalReal(4, "elemSizeY");
    card.elemSizeZ = reader.optionalReal(5, "elemSizeZ");
    card.xReference = reader.real(6, "xReference", EULER_MESH_DEFAULT_REFERENCE);
    card.yReference = reader.real(7, "yReference", EULER_MESH_DEFAULT_REFERENCE);
    card.zReference = reader.real(8, "zReference", EULER_MESH_DEFAULT_REFERENCE);
    // Fields 9 to 16
    card.originX = reader.optionalReal(9, "originX");
    card.originY = reader.optionalReal(10, "originY");
    card.originZ = reader.optionalReal(11, "originZ");
    card.widthX = reader.optionalReal(12, "widthX");
    card.widthY = reader.optionalReal(13, "widthY");
    card.widthZ = reader.optionalReal(14, "widthZ");
    // Fields 17 to 24
    card.elemCountX = reader.optionalInteger(17, "elemCountX");
    card.elemCountY = reader.optionalInteger(18, "elemCountY");
    card.elemCountZ = reader.optionalInteger(19, "elemCountZ");
    card.subMeshId = reader.integer(20, "subMeshId", 0);
    card.firstNodeId = reader.integer(21, "firstNodeId", 0);
    card.firstElemId = reader.integer(22, "firstElemId", 0);
    card.propKind = readKeyword(reader, 23, "propKind", EULER_MESH_PROPERTY_WORDS, EulerMeshGeneratorProperty::Euler);
    card.propId = reader.requiredInteger(24, "propId");
    // Fields 25 to 32
    card.resize = readKeyword(reader, 25, "resize", EULER_MESH_RESIZE_WORDS, EulerMeshGeneratorResize::None);
    card.resizeTableIdX = reader.integer(26, "resizeTableIdX", 0);
    card.resizeTableIdY = reader.integer(27, "resizeTableIdY", 0);
    card.resizeTableIdZ = reader.integer(28, "resizeTableIdZ", 0);
    card.method = readKeyword(reader, 29, "remeshMethod", EULER_MESH_REMESH_WORDS, EulerMeshGeneratorRemesh::All);
    // Fields 33 to 40
    card.bias = readOptionalKeyword<EulerMeshGeneratorBiasStart>(reader, 33, "biasStart", EULER_MESH_BIAS_START_WORDS);
    card.growthX = reader.optionalReal(34, "growthX");
    card.growthY = reader.optionalReal(35, "growthY");
    card.growthZ = reader.optionalReal(36, "growthZ");
    card.biasIdX = reader.integer(37, "biasIdX", 0);
    card.biasIdY = reader.integer(38, "biasIdY", 0);
    card.biasIdZ = reader.integer(39, "biasIdZ", 0);
    // Fields 41 to 48
    card.boxOriginX = reader.optionalReal(41, "boxOriginX");
    card.boxOriginY = reader.optionalReal(42, "boxOriginY");
    card.boxOriginZ = reader.optionalReal(43, "boxOriginZ");
    card.boxWidthX = reader.optionalReal(44, "boxWidthX");
    card.boxWidthY = reader.optionalReal(45, "boxWidthY");
    card.boxWidthZ = reader.optionalReal(46, "boxWidthZ");
    // Fields 49 to 56
    card.frameId = reader.integer(49, "frameId", 0);
    // Fields 57 to 64
    card.elemsPerCube = reader.integer(57, "elemsPerCube", 1);
    card.cubeCountX = reader.optionalInteger(58, "cubeCountX");
    card.cubeCountY = reader.optionalInteger(59, "cubeCountY");
    card.cubeCountZ = reader.optionalInteger(60, "cubeCountZ");
    card.partitionDirection =
        readKeyword(reader, 61, "partitionDirection", EULER_MESH_PARTITION_WORDS, EulerMeshGeneratorPartition::X);
    card.partitionCountX = reader.optionalInteger(62, "partitionCountX");
    card.partitionCountY = reader.optionalInteger(63, "partitionCountY");
    card.partitionCountZ = reader.optionalInteger(64, "partitionCountZ");
    // The cells that the grid of the format reference leaves blank, and the end of the card.
    requireBlankFields(reader, {15, 16, 30, 31, 32, 40, 47, 48, 50, 51, 52, 53, 54, 55, 56}, LAST_FIELD);
    resolveDefaults(card);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> EulerMeshGeneratorFormat::check(const EulerMeshGeneratorCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkOptionalReal(card.elemSizeX, "elemSizeX", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalReal(card.elemSizeY, "elemSizeY", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalReal(card.elemSizeZ, "elemSizeZ", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.xReference, "xReference", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.yReference, "yReference", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.zReference, "zReference", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalReal(card.originX, "originX", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalReal(card.originY, "originY", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalReal(card.originZ, "originZ", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalReal(card.widthX, "widthX", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalReal(card.widthY, "widthY", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalReal(card.widthZ, "widthZ", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalCount(card.elemCountX, "elemCountX", 1))
        return problem;
    if (const auto problem = checkOptionalCount(card.elemCountY, "elemCountY", 1))
        return problem;
    if (const auto problem = checkOptionalCount(card.elemCountZ, "elemCountZ", 1))
        return problem;
    if (const auto problem = checkOptionalId(card.subMeshId, "subMeshId"))
        return problem;
    // A mesh cannot be replaced in part by itself.
    if (card.subMeshId == card.id)
        return badValue("subMeshId", std::string(CardProblem::FIELD) + " must be another mesh than the " +
                                         CardProblem::fieldNamed("id") + " of the card");
    if (const auto problem = checkOptionalId(card.firstNodeId, "firstNodeId"))
        return problem;
    if (const auto problem = checkOptionalId(card.firstElemId, "firstElemId"))
        return problem;
    if (const auto problem = checkRequiredId(card.propId, "propId"))
        return problem;
    if (const auto problem = checkOptionalId(card.resizeTableIdX, "resizeTableIdX"))
        return problem;
    if (const auto problem = checkOptionalId(card.resizeTableIdY, "resizeTableIdY"))
        return problem;
    if (const auto problem = checkOptionalId(card.resizeTableIdZ, "resizeTableIdZ"))
        return problem;
    if (const auto problem = checkOptionalReal(card.growthX, "growthX", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalReal(card.growthY, "growthY", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalReal(card.growthZ, "growthZ", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalId(card.biasIdX, "biasIdX"))
        return problem;
    if (const auto problem = checkOptionalId(card.biasIdY, "biasIdY"))
        return problem;
    if (const auto problem = checkOptionalId(card.biasIdZ, "biasIdZ"))
        return problem;
    if (const auto problem = checkOptionalReal(card.boxOriginX, "boxOriginX", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalReal(card.boxOriginY, "boxOriginY", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalReal(card.boxOriginZ, "boxOriginZ", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalReal(card.boxWidthX, "boxWidthX", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalReal(card.boxWidthY, "boxWidthY", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalReal(card.boxWidthZ, "boxWidthZ", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalId(card.frameId, "frameId"))
        return problem;
    if (card.elemsPerCube < 1)
        return badValue("elemsPerCube", std::string(CardProblem::FIELD) + " must be at least 1");
    if (const auto problem = checkOptionalCount(card.cubeCountX, "cubeCountX", 0))
        return problem;
    if (const auto problem = checkOptionalCount(card.cubeCountY, "cubeCountY", 0))
        return problem;
    if (const auto problem = checkOptionalCount(card.cubeCountZ, "cubeCountZ", 0))
        return problem;
    if (const auto problem = checkOptionalCount(card.partitionCountX, "partitionCountX", 1))
        return problem;
    if (const auto problem = checkOptionalCount(card.partitionCountY, "partitionCountY", 1))
        return problem;
    if (const auto problem = checkOptionalCount(card.partitionCountZ, "partitionCountZ", 1))
        return problem;
    return checkCombinations(card);
}

void EulerMeshGeneratorFormat::write(const EulerMeshGeneratorCard& card, FieldWriter& writer)
{
    // Fields 1 to 8
    writer.integer(1, card.id);
    writeKeyword(writer, 2, EULER_MESH_TYPE_WORDS, card.type);
    if (card.elemSizeX)
        writer.real(3, *card.elemSizeX);
    if (card.elemSizeY)
        writer.real(4, *card.elemSizeY);
    if (card.elemSizeZ)
        writer.real(5, *card.elemSizeZ);
    writer.realUnlessDefault(6, card.xReference, EULER_MESH_DEFAULT_REFERENCE);
    writer.realUnlessDefault(7, card.yReference, EULER_MESH_DEFAULT_REFERENCE);
    writer.realUnlessDefault(8, card.zReference, EULER_MESH_DEFAULT_REFERENCE);
    // Fields 9 to 16
    if (card.originX)
        writer.real(9, *card.originX);
    if (card.originY)
        writer.real(10, *card.originY);
    if (card.originZ)
        writer.real(11, *card.originZ);
    if (card.widthX)
        writer.real(12, *card.widthX);
    if (card.widthY)
        writer.real(13, *card.widthY);
    if (card.widthZ)
        writer.real(14, *card.widthZ);
    // Fields 17 to 24
    if (card.elemCountX)
        writer.integer(17, *card.elemCountX);
    if (card.elemCountY)
        writer.integer(18, *card.elemCountY);
    if (card.elemCountZ)
        writer.integer(19, *card.elemCountZ);
    writer.integerUnlessDefault(20, card.subMeshId, 0);
    writer.integerUnlessDefault(21, card.firstNodeId, 0);
    writer.integerUnlessDefault(22, card.firstElemId, 0);
    writeKeywordUnlessDefault(writer, 23, EULER_MESH_PROPERTY_WORDS, card.propKind, EulerMeshGeneratorProperty::Euler);
    writer.integer(24, card.propId);
    // Fields 25 to 32
    writeKeywordUnlessDefault(writer, 25, EULER_MESH_RESIZE_WORDS, card.resize, EulerMeshGeneratorResize::None);
    writer.integerUnlessDefault(26, card.resizeTableIdX, 0);
    writer.integerUnlessDefault(27, card.resizeTableIdY, 0);
    writer.integerUnlessDefault(28, card.resizeTableIdZ, 0);
    writeKeywordUnlessDefault(writer, 29, EULER_MESH_REMESH_WORDS, card.method, EulerMeshGeneratorRemesh::All);
    // Fields 33 to 40
    writeOptionalKeyword(writer, 33, EULER_MESH_BIAS_START_WORDS, card.bias);
    if (card.growthX)
        writer.real(34, *card.growthX);
    if (card.growthY)
        writer.real(35, *card.growthY);
    if (card.growthZ)
        writer.real(36, *card.growthZ);
    writer.integerUnlessDefault(37, card.biasIdX, 0);
    writer.integerUnlessDefault(38, card.biasIdY, 0);
    writer.integerUnlessDefault(39, card.biasIdZ, 0);
    // Fields 41 to 48
    if (card.boxOriginX)
        writer.real(41, *card.boxOriginX);
    if (card.boxOriginY)
        writer.real(42, *card.boxOriginY);
    if (card.boxOriginZ)
        writer.real(43, *card.boxOriginZ);
    if (card.boxWidthX)
        writer.real(44, *card.boxWidthX);
    if (card.boxWidthY)
        writer.real(45, *card.boxWidthY);
    if (card.boxWidthZ)
        writer.real(46, *card.boxWidthZ);
    // Fields 49 to 56
    writer.integerUnlessDefault(49, card.frameId, 0);
    // Fields 57 to 64
    writer.integerUnlessDefault(57, card.elemsPerCube, 1);
    if (card.cubeCountX)
        writer.integer(58, *card.cubeCountX);
    if (card.cubeCountY)
        writer.integer(59, *card.cubeCountY);
    if (card.cubeCountZ)
        writer.integer(60, *card.cubeCountZ);
    writeKeywordUnlessDefault(writer, 61, EULER_MESH_PARTITION_WORDS, card.partitionDirection,
                              EulerMeshGeneratorPartition::X);
    if (card.partitionCountX)
        writer.integer(62, *card.partitionCountX);
    if (card.partitionCountY)
        writer.integer(63, *card.partitionCountY);
    if (card.partitionCountZ)
        writer.integer(64, *card.partitionCountZ);
}

std::uint32_t EulerMeshGeneratorTable::append(const EulerMeshGeneratorCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void EulerMeshGeneratorTable::store(std::uint32_t row, const EulerMeshGeneratorCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void EulerMeshGeneratorTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void EulerMeshGeneratorTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void EulerMeshGeneratorTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    EulerMeshGeneratorCard& card = m_cards.edit(row);
    visitId(visitor, "subMeshId", ReferenceTarget::EulerMesh, card.subMeshId);
    visitId(visitor, "propId", ReferenceTarget::Property, card.propId);
    visitId(visitor, "resizeTableIdX", ReferenceTarget::DynamicTable, card.resizeTableIdX);
    visitId(visitor, "resizeTableIdY", ReferenceTarget::DynamicTable, card.resizeTableIdY);
    visitId(visitor, "resizeTableIdZ", ReferenceTarget::DynamicTable, card.resizeTableIdZ);
    visitId(visitor, "biasIdX", ReferenceTarget::MeshBias, card.biasIdX);
    visitId(visitor, "biasIdY", ReferenceTarget::MeshBias, card.biasIdY);
    visitId(visitor, "biasIdZ", ReferenceTarget::MeshBias, card.biasIdZ);
    visitId(visitor, "frameId", ReferenceTarget::Coordinate, card.frameId);
}

} // namespace oor::model
