#include "oor/model/cards/nonlinear_contact/contactFaceRegion/ContactFaceRegionTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/nonlinear_contact/ContactSections.hpp"
#include "oor/model/cards/thermal/ThermalSupport.hpp"

namespace oor::model
{

namespace
{

// An entry of a line is three or four fields; two entries go on a line, the second after a gap of one field.
constexpr int SECOND_ENTRY_OFFSET = 4;

// The faces the format reference lists for the element types: S1 to S6 (solids), E1 to E4 (edges of a shell), TOP and
// BTM (the sides of a shell), BOTH, ENDA and ENDB (the ends of a bar or beam); blank means both sides of a shell or the
// whole beam.
bool isFaceName(const std::string& face)
{
    if (face.empty() || face == "Top" || face == "Bottom" || face == "Both" || face == "EndA" || face == "EndB")
        return true;
    if (face.size() != 2)
        return false;
    const char number = face[1];
    return (face[0] == 'S' && number >= '1' && number <= '6') || (face[0] == 'E' && number >= '1' && number <= '4');
}

bool blankFields(const FieldReader& reader, int first, int last)
{
    for (int field = first; field <= last; ++field)
    {
        if (!reader.isBlank(field))
            return false;
    }
    return true;
}

void readFaces(FieldReader& reader, ContactFaceRegionCard& card, int lines)
{
    for (int line = 1; line < lines && !reader.failed(); ++line)
    {
        const int start = firstFieldOfLine(line);
        if (!reader.isBlank(start + 3) || !reader.isBlank(start + 7))
            reader.error(
                "card.bad-value",
                "the entries of a FACE line of a " + std::string(reader.cardName()) + " use fields 1 to 3 and 5 to 7",
                "faces");
        for (int slot = 0; slot < 2; ++slot)
        {
            const int field = start + slot * SECOND_ENTRY_OFFSET;
            if (reader.isBlank(field))
            {
                if (!blankFields(reader, field + 1, field + 2))
                    reader.error("card.missing-field", "a face needs its element or set ID", "faceElemId");
                continue;
            }
            ContactFaceRegionFace face;
            face.id = reader.requiredInteger(field, "faceElemId");
            // A face is a word (Top, Bottom, ...) or a code of a side (S1 to S6, E1 to E4), using exact case.
            const std::string_view faceText = reader.text(field + 1);
            face.face = std::string(faceText);
            face.idType = readKeyword(reader, field + 2, "idType", {"Elements", "Set"}, "Elements");
            card.faces.push_back(std::move(face));
        }
    }
}

void readGridFaces(FieldReader& reader, ContactFaceRegionCard& card, int lines)
{
    for (int line = 1; line < lines && !reader.failed(); ++line)
    {
        const int start = firstFieldOfLine(line);
        for (int slot = 0; slot < 2; ++slot)
        {
            const int field = start + slot * SECOND_ENTRY_OFFSET;
            if (reader.isBlank(field))
            {
                if (!blankFields(reader, field + 1, field + 3))
                    reader.error("card.missing-field", "a face needs its element ID", "elemId");
                continue;
            }
            ContactFaceRegionGridFace face;
            face.elemId = reader.requiredInteger(field, "elemId");
            for (int corner = 0; corner < 3; ++corner)
                face.grids[static_cast<std::size_t>(corner)] = reader.integer(field + 1 + corner, "node", 0);
            card.gridFaces.push_back(face);
        }
    }
}

} // namespace

std::optional<ContactFaceRegionCard> ContactFaceRegionFormat::read(FieldReader& reader)
{
    ContactFaceRegionCard card;
    card.id = reader.requiredInteger(1, "id");
    card.bodyParamsId = reader.integer(2, "bodyParamsId", 0);
    card.dimension = readKeyword(reader, 3, "dimension", {"TwoDimensional", "ThreeDimensional"}, "ThreeDimensional");
    card.form = readKeyword(reader, 5, "form", {"Face", "Node", "Rigid"}, "Face");
    card.includeThickness = readKeyword(reader, 6, "includeThickness", {"Yes", "No"}, "Yes");
    card.edgeContact = reader.integer(7, "edgeContact", 1);
    requireUnusedBlank(reader, {4, 8}, reader.lastUsedField());
    const int lines = (reader.lastUsedField() + CONTACT_LINE_FIELDS - 1) / CONTACT_LINE_FIELDS;
    if (card.form == "Face")
    {
        readFaces(reader, card, lines);
    }
    else if (card.form == "Node")
    {
        readGridFaces(reader, card, lines);
    }
    else
    {
        const int start = firstFieldOfLine(1);
        card.surfaceId = reader.integer(start, "surfaceId", 0);
        card.rigidMotionId = reader.integer(start + 1, "rigidMotionId", 0);
        if (lines > 2 || !blankFields(reader, start + 2, start + 7))
            reader.error("card.bad-value",
                         "the RIGID form of a " + std::string(reader.cardName()) + " has one line, with " +
                             std::string("surfaceId") + " and " + std::string("rigidMotionId"),
                         "surfaceId");
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ContactFaceRegionFormat::check(const ContactFaceRegionCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "id"};
    if (card.bodyParamsId < 0)
        return CardProblem{"card.bad-id",
                           std::string(CardProblem::FIELD) + " must be the ID of body parameters, or blank",
                           "bodyParamsId"};
    if (auto problem = checkKeyword(card.dimension, "dimension", {"TwoDimensional", "ThreeDimensional"}))
        return problem;
    if (auto problem = checkKeyword(card.form, "form", {"Face", "Node", "Rigid"}))
        return problem;
    if (auto problem = checkKeyword(card.includeThickness, "includeThickness", {"Yes", "No"}))
        return problem;
    if (card.edgeContact != 1 && card.edgeContact != 10 && card.edgeContact != 11)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 1, 10 or 11", "edgeContact"};
    const bool face = card.form == "Face";
    const bool grid = card.form == "Node";
    if ((!face && !card.faces.empty()) || (!grid && !card.gridFaces.empty()))
        return CardProblem{"card.bad-value",
                           "the entries do not belong to the " + std::string(CardProblem::FIELD) + " of the card",
                           "form"};
    if (card.form != "Rigid" && (card.surfaceId != 0 || card.rigidMotionId != 0))
        return CardProblem{"card.bad-value",
                           CardProblem::fieldNamed("surfaceId") + " and " + CardProblem::fieldNamed("rigidMotionId") +
                               " are for the RIGID form",
                           "form"};
    if (face)
    {
        if (card.faces.empty())
            return CardProblem{"card.missing-field", "the surface needs at least one face", "faces"};
        for (const ContactFaceRegionFace& entry : card.faces)
        {
            if (entry.id <= 0)
                return CardProblem{"card.bad-id", "an element or set ID must be a positive integer", "faces"};
            if (!isFaceName(entry.face))
                return CardProblem{"card.bad-value",
                                   CardProblem::fieldNamed("faceId") +
                                       " must be S1 to S6, E1 to E4, Top, Bottom, Both, EndA, EndB or blank",
                                   "faces"};
            if (entry.idType != "Elements" && entry.idType != "Set")
                return CardProblem{"card.bad-value", CardProblem::fieldNamed("idType") + " must be ELEM or SET",
                                   "faces"};
        }
    }
    else if (grid)
    {
        if (card.gridFaces.empty())
            return CardProblem{"card.missing-field", "the surface needs at least one face", "elemId"};
        for (const ContactFaceRegionGridFace& entry : card.gridFaces)
        {
            if (entry.elemId <= 0)
                return CardProblem{"card.bad-id", "an element ID must be a positive integer", "elemId"};
            int given = 0;
            for (const base::EntityId corner : entry.grids)
            {
                if (corner < 0)
                    return CardProblem{"card.bad-id", "a grid point ID must not be negative", "elemId"};
                given += corner > 0 ? 1 : 0;
            }
            // three corners fix a face of a 3D body, two (G1 and G2, G3 blank) an
            // edge of a 2D body; none means both sides of a shell or the whole beam.
            if (card.dimension == "TwoDimensional")
            {
                if (given != 0 && (given != 2 || entry.grids[2] != 0))
                    return CardProblem{
                        "card.bad-value",
                        "the two corners of an edge of a 2D body are both given (the third blank) or both blank",
                        "elemId"};
            }
            else if (given != 0 && given != 3)
            {
                return CardProblem{"card.bad-value", "the three corners of a face are all given or all blank",
                                   "elemId"};
            }
        }
    }
    else
    {
        if (card.surfaceId <= 0)
            return CardProblem{"card.missing-field", "the RIGID form needs its " + std::string(CardProblem::FIELD),
                               "surfaceId"};
        if (card.rigidMotionId < 0)
            return CardProblem{"card.bad-id",
                               std::string(CardProblem::FIELD) + " must be the ID of a rigid body motion, or blank",
                               "rigidMotionId"};
    }
    return std::nullopt;
}

void ContactFaceRegionFormat::write(const ContactFaceRegionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.bodyParamsId, 0);
    writer.textUnlessDefault(3, card.dimension, "ThreeDimensional");
    writer.textUnlessDefault(5, card.form, "Face");
    writer.textUnlessDefault(6, card.includeThickness, "Yes");
    writer.integerUnlessDefault(7, card.edgeContact, 1);
    if (card.form == "Rigid")
    {
        const int start = firstFieldOfLine(1);
        writer.integer(start, card.surfaceId);
        writer.integerUnlessDefault(start + 1, card.rigidMotionId, 0);
        return;
    }
    const std::size_t count = card.form == "Face" ? card.faces.size() : card.gridFaces.size();
    for (std::size_t index = 0; index < count; ++index)
    {
        const int start = firstFieldOfLine(1 + static_cast<int>(index / 2));
        const int field = start + static_cast<int>(index % 2) * SECOND_ENTRY_OFFSET;
        if (card.form == "Face")
        {
            const ContactFaceRegionFace& entry = card.faces[index];
            writer.integer(field, entry.id);
            if (!entry.face.empty())
                writer.text(field + 1, entry.face);
            writer.textUnlessDefault(field + 2, entry.idType, "Elements");
        }
        else
        {
            const ContactFaceRegionGridFace& entry = card.gridFaces[index];
            writer.integer(field, entry.elemId);
            for (std::size_t corner = 0; corner < 3; ++corner)
                writer.integerUnlessDefault(field + 1 + static_cast<int>(corner), entry.grids[corner], 0);
        }
    }
}

std::uint32_t ContactFaceRegionTable::append(const ContactFaceRegionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ContactFaceRegionTable::store(std::uint32_t row, const ContactFaceRegionCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ContactFaceRegionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ContactFaceRegionTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ContactFaceRegionTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    ContactFaceRegionCard& card = m_cards.edit(row);
    visitContactId(card.bodyParamsId, visitor, "bodyParamsId", ReferenceTarget::ContactBodyParameters);
    for (ContactFaceRegionFace& face : card.faces)
        visitContactId(face.id, visitor, "faceElemId",
                       face.idType == "Set" ? ReferenceTarget::Set : ReferenceTarget::Element);
    for (ContactFaceRegionGridFace& face : card.gridFaces)
    {
        visitContactId(face.elemId, visitor, "elemId", ReferenceTarget::Element);
        for (base::EntityId& grid : face.grids)
            visitContactId(grid, visitor, "node", ReferenceTarget::Node);
    }
    visitContactId(card.surfaceId, visitor, "surfaceId", ReferenceTarget::ContactSurface);
    visitContactId(card.rigidMotionId, visitor, "rigidMotionId", ReferenceTarget::ContactRigid);
}

} // namespace oor::model
