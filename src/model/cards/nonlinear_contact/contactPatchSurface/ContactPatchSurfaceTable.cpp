#include "oor/model/cards/nonlinear_contact/contactPatchSurface/ContactPatchSurfaceTable.hpp"

#include "oor/model/cards/nonlinear_contact/ContactSections.hpp"

namespace oor::model
{

std::optional<ContactPatchSurfaceCard> ContactPatchSurfaceFormat::read(FieldReader& reader)
{
    ContactPatchSurfaceCard card;
    card.id = reader.requiredInteger(1, "id");
    const int lines = (reader.lastUsedField() + CONTACT_LINE_FIELDS - 1) / CONTACT_LINE_FIELDS;
    for (int line = 1; line < lines && !reader.failed(); ++line)
    {
        const int start = firstFieldOfLine(line);
        bool blank = true;
        for (int field = start; field < start + CONTACT_LINE_FIELDS; ++field)
            blank = blank && reader.isBlank(field);
        if (blank)
            continue;
        ContactPatch patch;
        patch.patchId = reader.requiredInteger(start, "patchId");
        for (int corner = 0; corner < 4; ++corner)
            patch.grids[static_cast<std::size_t>(corner)] = reader.requiredInteger(start + 1 + corner, "node");
        if (reader.lastUsedField() >= start + 5 && !reader.isBlank(start + 5))
            reader.error("card.bad-value", "a patch of a " + std::string(reader.cardName()) + " has four grid points",
                         "node4");
        card.patches.push_back(patch);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ContactPatchSurfaceFormat::check(const ContactPatchSurfaceCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "id"};
    if (card.patches.empty())
        return CardProblem{"card.missing-field", "the surface needs at least one patch", "patchId"};
    for (const ContactPatch& patch : card.patches)
    {
        if (patch.patchId <= 0)
            return CardProblem{"card.bad-id",
                               "the patch number " + std::string(CardProblem::FIELD) + " must be a positive integer",
                               "patchId"};
        for (std::size_t corner = 0; corner < 4; ++corner)
        {
            if (patch.grids[corner] <= 0)
                return CardProblem{"card.bad-id", "a grid point ID must be a positive integer", "node"};
            for (std::size_t other = 0; other < corner; ++other)
            {
                if (patch.grids[corner] == patch.grids[other])
                    return CardProblem{"card.bad-value", "the four grid points of a patch must be different", "node"};
            }
        }
    }
    return std::nullopt;
}

void ContactPatchSurfaceFormat::write(const ContactPatchSurfaceCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    int line = 1;
    for (const ContactPatch& patch : card.patches)
    {
        const int start = firstFieldOfLine(line++);
        writer.integer(start, patch.patchId);
        for (int corner = 0; corner < 4; ++corner)
            writer.integer(start + 1 + corner, patch.grids[static_cast<std::size_t>(corner)]);
    }
}

std::uint32_t ContactPatchSurfaceTable::append(const ContactPatchSurfaceCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    std::vector<std::array<base::EntityId, 5>> patches;
    for (const ContactPatch& patch : card.patches)
        patches.push_back({patch.patchId, patch.grids[0], patch.grids[1], patch.grids[2], patch.grids[3]});
    m_patches.push(patches);
    return row;
}

ContactPatchSurfaceCard ContactPatchSurfaceTable::card(std::uint32_t row) const
{
    ContactPatchSurfaceCard card;
    card.id = m_id.get(row);
    for (const auto& entry : m_patches.get(row))
        card.patches.push_back(ContactPatch{entry[0], {entry[1], entry[2], entry[3], entry[4]}});
    return card;
}

void ContactPatchSurfaceTable::store(std::uint32_t row, const ContactPatchSurfaceCard& card)
{
    m_id.set(row, card.id);
    std::vector<std::array<base::EntityId, 5>> patches;
    for (const ContactPatch& patch : card.patches)
        patches.push_back({patch.patchId, patch.grids[0], patch.grids[1], patch.grids[2], patch.grids[3]});
    m_patches.set(row, patches);
}

void ContactPatchSurfaceTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_patches.keepRows(rows);
}

void ContactPatchSurfaceTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    const auto current = m_patches.get(row);
    std::vector<std::array<base::EntityId, 5>> patches(current.begin(), current.end());
    bool changed = false;
    for (auto& entry : patches)
    {
        for (std::size_t corner = 1; corner < 5; ++corner)
        {
            const base::EntityId updated = visitor.visit("node", ReferenceTarget::Node, entry[corner]);
            changed = changed || updated != entry[corner];
            entry[corner] = updated;
        }
    }
    if (changed)
        m_patches.set(row, patches);
}

} // namespace oor::model
