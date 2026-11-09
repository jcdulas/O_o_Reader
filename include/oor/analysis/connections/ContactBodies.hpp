#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/Model.hpp"

#include <cstdint>
#include <string_view>
#include <vector>

namespace oor::analysis
{

// The card type a contact body comes from.
enum class ContactBodySource : std::uint8_t
{
    ContactBody,        // over a ContactElemSurface, a ContactPropertyRegion or (SOL 700) a ContactMaterialRegion
    ContactBody1,       // over a ContactElemSurface or a ContactPropertyRegion
    ContactFaceRegion,  // a list of elements (FACE and GRID forms)
    ContactNodeRegion,  // grid points only, a secondary body (node-to-segment contact)
    ContactElemSurface, // a region named by a ContactSet
    SlidelineSegments   // a line named by a ContactSlideline or a ContactSet: grid points joined by straight segments
};

// The card type of a body (a neutral name: a message shows it through model::cardLabel).
[[nodiscard]] std::string_view contactBodySourceName(ContactBodySource source) noexcept;

// One contact body: what the contact search reads of it.
struct ContactBody
{
    base::EntityId id = 0; // the BID of a body, the ID of a BSURF or BLSEG region
    ContactBodySource source = ContactBodySource::ContactBody;
    model::CardLocation card;
    // A rigid body has no structural element and plays no part in the components: BEHAV RIGID, SYMM or HEAT (format
    // reference BCBODY, BCBODY1: a symmetry body and a heat-rigid body are rigid surfaces), the RIGID form of a BCSURF,
    // or a BCBODY whose BSID is not a surface made of elements (a BCBOX, a BCSEG, a BCELIPS, ...).
    bool isRigid = false;
    // The shells and solids of the body, in increasing order of the location; the other elements a list names (beams,
    // bars) are left out: the search is between surfaces.
    std::vector<model::CardLocation> elements;
    // The grid points of a BCGRID body, and of a BLSEG line in the order of the curve.
    std::vector<base::EntityId> grids;
    // The cards the body is made of (its BSURF, BCPROP or BCMATL): the cards of its revision stamp besides its own.
    std::vector<model::CardLocation> definitionCards;
};

// The contact bodies of a model (BCBODY, BCBODY1, BCSURF, BCGRID: one ID space, the BID) and the regions that the
// legacy tables name instead of bodies (BSURF for a BCTSET, BLSEG for a BCONP or a BCTSET). Read once; immutable.
//
// format reference BCSURF: when a BCONECT names a BCSURF, its FACEIDs are ignored, so a BCSURF is the
// elements it lists and the search uses their outer faces. An entry of IDTYPE SET (a SET3 of elements) is not read yet:
// the body has the elements it lists by ID.
class ContactBodies
{
public:
    [[nodiscard]] static ContactBodies read(const model::Model& model);

    // The bodies, by increasing BID.
    [[nodiscard]] const std::vector<ContactBody>& bodies() const noexcept { return m_bodies; }
    // The body of a BID; null when no body has it.
    [[nodiscard]] const ContactBody* findBody(base::EntityId id) const noexcept;
    // The BSURF region of an ID (else its BLSEG line when `orLine`); null when there is none.
    [[nodiscard]] const ContactBody* findRegion(base::EntityId id, bool orLine) const noexcept;
    // The BLSEG line of an ID; null when there is none.
    [[nodiscard]] const ContactBody* findLine(base::EntityId id) const noexcept;

private:
    std::vector<ContactBody> m_bodies;  // by BID
    std::vector<ContactBody> m_regions; // the BSURF regions, by ID
    std::vector<ContactBody> m_lines;   // the BLSEG lines, by ID
};

} // namespace oor::analysis
