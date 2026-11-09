#include "oor/model/cards/solution_control/cosimulationArea/CosimulationAreaTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

// Every form lists its items from the first field of the first continuation line.
constexpr int FIRST_ITEM_FIELD = 9;
constexpr std::int32_t FIRST_FACE = 1; // the sides of an element are numbered as those of a CHBDYE (1 to 6)
constexpr std::int32_t LAST_FACE = 6;

// A list of grid points or elements goes upwards and has no increment: the format draws neither BY nor a descending
// list.
constexpr ContactRunRules RUN_RULES{false, false};

// The pairs of element and face from field 9 on. A pair is two fields and a line holds four, so the pairs always start
// in odd fields; a pair with both fields blank is the padding of a line.
std::vector<CosimulationFace> readFaces(FieldReader& reader, int last)
{
    std::vector<CosimulationFace> faces;
    for (int field = FIRST_ITEM_FIELD; field <= last && !reader.failed(); field += 2)
    {
        if (reader.isBlank(field) && reader.isBlank(field + 1))
            continue;
        CosimulationFace face;
        face.elemId = reader.requiredInteger(field, "elemIds");
        face.face = reader.requiredInteger(field + 1, "faceIds");
        faces.push_back(face);
    }
    return faces;
}

// The name a list of the form gives its items in the messages.
const char* itemName(CosimulationAreaType type) noexcept
{
    return type == CosimulationAreaType::Point ? "GridID" : "elemIds";
}

} // namespace

std::optional<CosimulationAreaCard> CosimulationAreaFormat::read(FieldReader& reader)
{
    CosimulationAreaCard card;
    card.id = reader.requiredInteger(1, "id");
    card.kind = readKeyword(reader, 2, "kind", COSIMULATION_AREA_WORDS, CosimulationAreaType::Surface);
    requireBlankFields(reader, {3, 4, 5, 6, 7, 8}, reader.lastUsedField());
    if (reader.failed())
        return card;
    const int last = reader.lastUsedField();
    if (card.kind == CosimulationAreaType::Surface)
        card.faces = readFaces(reader, last);
    else
        card.runs = readContactRuns(reader, FIRST_ITEM_FIELD, last, itemName(card.kind), RUN_RULES);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> CosimulationAreaFormat::check(const CosimulationAreaCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.kind == CosimulationAreaType::Surface)
    {
        if (!card.runs.empty())
            return CardProblem{"card.bad-value", "a SURFACE area lists elements and faces, not a run of IDs", "kind"};
        if (card.faces.empty())
            return CardProblem{"card.missing-field", "a SURFACE area needs at least one element and face", "elemIds"};
        for (const CosimulationFace& face : card.faces)
        {
            if (const auto problem = checkRequiredId(face.elemId, "elemIds"))
                return problem;
            if (face.face < FIRST_FACE || face.face > LAST_FACE)
                return CardProblem{"card.bad-value", "FaceID must be between 1 and 6", "faceIds"};
        }
        return std::nullopt;
    }
    const bool isPoint = card.kind == CosimulationAreaType::Point;
    if (!card.faces.empty())
        return CardProblem{"card.bad-value", "only a SURFACE area lists elements and faces", "kind"};
    if (card.runs.empty())
    {
        if (isPoint)
            return CardProblem{"card.missing-field", "a POINT area needs at least one grid point", "nodeIds"};
        return CardProblem{"card.missing-field", "a VOLUME area needs at least one element", "elemIds"};
    }
    return checkContactRuns(card.runs, itemName(card.kind), RUN_RULES);
}

void CosimulationAreaFormat::write(const CosimulationAreaCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeKeywordUnlessDefault(writer, 2, COSIMULATION_AREA_WORDS, card.kind, CosimulationAreaType::Surface);
    if (card.kind != CosimulationAreaType::Surface)
    {
        writeContactRuns(writer, FIRST_ITEM_FIELD, card.runs);
        return;
    }
    int field = FIRST_ITEM_FIELD;
    for (const CosimulationFace& face : card.faces)
    {
        writer.integer(field++, face.elemId);
        writer.integer(field++, face.face);
    }
}

std::uint32_t CosimulationAreaTable::append(const CosimulationAreaCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void CosimulationAreaTable::store(std::uint32_t row, const CosimulationAreaCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void CosimulationAreaTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void CosimulationAreaTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void CosimulationAreaTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    CosimulationAreaCard& card = m_cards.edit(row);
    switch (card.kind)
    {
    case CosimulationAreaType::Surface:
        for (CosimulationFace& face : card.faces)
            visitId(visitor, "elemIds", ReferenceTarget::Element, face.elemId);
        break;
    case CosimulationAreaType::Point:
        visitContactRuns(card.runs, visitor, "nodeIds", ReferenceTarget::Node, ReferenceTarget::NodeInRange);
        break;
    case CosimulationAreaType::Volume:
        visitContactRuns(card.runs, visitor, "elemIds", ReferenceTarget::Element, ReferenceTarget::ElementInRange);
        break;
    }
}

} // namespace oor::model
