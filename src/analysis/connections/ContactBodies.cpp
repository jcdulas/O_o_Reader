#include "oor/analysis/connections/ContactBodies.hpp"

#include "oor/analysis/adjacency/ElementTopology.hpp"
#include "oor/analysis/components/ComponentCache.hpp"
#include "oor/analysis/frames/StoredFrames.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/nonlinear_contact/contactBody/ContactBodyTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactBody1/ContactBody1Table.hpp"
#include "oor/model/cards/nonlinear_contact/contactElemSurface/ContactElemSurfaceTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactFaceRegion/ContactFaceRegionTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactMaterialRegion/ContactMaterialRegionTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactNodeRegion/ContactNodeRegionTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactPropertyRegion/ContactPropertyRegionTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactSet/ContactSetTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactSlideline/ContactSlidelineTable.hpp"
#include "oor/model/cards/nonlinear_contact/slidelineSegments/SlidelineSegmentsTable.hpp"

#include <algorithm>
#include <map>
#include <optional>
#include <set>

namespace oor::analysis
{

namespace
{

bool locationLess(const model::CardLocation& left, const model::CardLocation& right) noexcept
{
    return left.table != right.table ? left.table < right.table : left.row < right.row;
}

void sortUnique(std::vector<model::CardLocation>& locations)
{
    std::sort(locations.begin(), locations.end(), locationLess);
    locations.erase(std::unique(locations.begin(), locations.end()), locations.end());
}

// The elements a contact surface is made of: shells and solids (the search is between the outer faces of
// the bodies).
bool isSurfaceType(std::uint16_t type) noexcept
{
    const int dimension = dimensionOf(ElementTopology::shapeOf(type));
    return dimension == 2 || dimension == 3;
}

// format reference BCBODY and BCBODY1, BEHAV: Deformable (DEFORM) is a deformable body; Rigid, Symmetric (a symmetry
// rigid body) and HeatTransfer (a heat-rigid body) are rigid surfaces.
bool isRigidBehaviour(const std::string& behav) noexcept
{
    return behav != "Deformable";
}

bool byId(const ContactBody& left, const ContactBody& right) noexcept
{
    return left.id < right.id;
}

const ContactBody* findIn(const std::vector<ContactBody>& bodies, base::EntityId id) noexcept
{
    const auto found =
        std::lower_bound(bodies.begin(), bodies.end(), id,
                         [](const ContactBody& body, base::EntityId wanted) { return body.id < wanted; });
    return found != bodies.end() && found->id == id ? &*found : nullptr;
}

// Reads the element sets the BSID of a body names (BSURF, BCPROP, BCMATL), each once.
class SurfaceReader
{
public:
    explicit SurfaceReader(const model::Model& model) : m_model(model) {}

    // The elements of a BSURF, a BCPROP or a BCMATL with this ID (one ID space: format reference BSURF, BCPROP,
    // BCMATL), with the card; nothing when the ID is none of them (a BCBOX, a BCSEG, a rigid surface, or nothing).
    [[nodiscard]] std::optional<std::pair<model::CardLocation, std::vector<model::CardLocation>>> read(
        base::EntityId id)
    {
        if (const auto location = m_model.locateInType(model::typeIndexOf<model::ContactElemSurfaceTable>(), id))
            return std::make_pair(*location, bsurfElements(*location));
        if (const auto location = m_model.locateInType(model::typeIndexOf<model::ContactPropertyRegionTable>(), id))
        {
            const model::ContactPropertyRegionCard card =
                m_model.findTable<model::ContactPropertyRegionTable>()->card(location->row);
            const std::vector<base::EntityId> wanted = model::expandContactRuns(card.properties);
            return std::make_pair(*location,
                                  elementsWithProperty(std::set<base::EntityId>(wanted.begin(), wanted.end())));
        }
        if (const auto location = m_model.locateInType(model::typeIndexOf<model::ContactMaterialRegionTable>(), id))
        {
            const model::ContactMaterialRegionCard card =
                m_model.findTable<model::ContactMaterialRegionTable>()->card(location->row);
            return std::make_pair(*location, elementsWithMaterial(card.materials));
        }
        return std::nullopt;
    }

    [[nodiscard]] std::vector<model::CardLocation> bsurfElements(model::CardLocation location) const
    {
        const model::ContactElemSurfaceCard card =
            m_model.findTable<model::ContactElemSurfaceTable>()->card(location.row);
        return elementsById(model::expandContactRuns(card.elements));
    }

    [[nodiscard]] std::vector<model::CardLocation> elementsById(const std::vector<base::EntityId>& ids) const
    {
        std::vector<model::CardLocation> elements;
        for (const base::EntityId eid : ids)
        {
            const auto location = m_model.locate(base::IdSpace::Element, eid);
            if (location && isSurfaceType(location->table))
                elements.push_back(*location);
        }
        sortUnique(elements);
        return elements;
    }

private:
    // format reference BCPROP: every element that has one of the listed properties.
    [[nodiscard]] std::vector<model::CardLocation> elementsWithProperty(const std::set<base::EntityId>& properties)
    {
        prepareProperties();
        std::vector<model::CardLocation> elements;
        for (const base::EntityId pid : properties)
        {
            const auto [first, last] = m_byProperty.equal_range(pid);
            for (auto at = first; at != last; ++at)
                elements.push_back(at->second);
        }
        sortUnique(elements);
        return elements;
    }

    // format reference BCMATL: every element whose property has one of the listed materials.
    [[nodiscard]] std::vector<model::CardLocation> elementsWithMaterial(const std::vector<base::EntityId>& materials)
    {
        prepareProperties();
        const std::shared_ptr<const ReferenceIndex> references = ComponentCache::of(m_model).references(m_model);
        std::set<base::EntityId> properties;
        for (const auto& [pid, element] : m_byProperty)
        {
            if (properties.contains(pid))
                continue;
            const auto property = m_model.locate(base::IdSpace::Property, pid);
            if (!property)
                continue;
            for (const ReferenceEdge& edge : references->from(*property))
            {
                if (edge.target == model::ReferenceTarget::Material &&
                    std::find(materials.begin(), materials.end(), edge.id) != materials.end())
                    properties.insert(pid);
            }
        }
        return elementsWithProperty(properties);
    }

    void prepareProperties()
    {
        if (m_propertiesRead)
            return;
        m_propertiesRead = true;
        const model::CardRegistry& registry = model::CardRegistry::instance();
        for (std::uint16_t type = 1; type < registry.size(); ++type)
        {
            const model::CardTable* table = m_model.findTable(type);
            if (table == nullptr || !isSurfaceType(type))
                continue;
            for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
            {
                if (!table->rows().isAlive(row))
                    continue;
                const model::CardLocation location{type, row};
                if (const auto pid = StoredFrames::propertyOf(m_model, location))
                    m_byProperty.emplace(*pid, location);
            }
        }
    }

    const model::Model& m_model;
    bool m_propertiesRead = false;
    std::multimap<base::EntityId, model::CardLocation> m_byProperty;
};

} // namespace

std::string_view contactBodySourceName(ContactBodySource source) noexcept
{
    switch (source)
    {
    case ContactBodySource::ContactBody:
        return "ContactBody";
    case ContactBodySource::ContactBody1:
        return "ContactBody1";
    case ContactBodySource::ContactFaceRegion:
        return "ContactFaceRegion";
    case ContactBodySource::ContactNodeRegion:
        return "ContactNodeRegion";
    case ContactBodySource::ContactElemSurface:
        return "ContactElemSurface";
    case ContactBodySource::SlidelineSegments:
        return "SlidelineSegments";
    }
    return "";
}

ContactBodies ContactBodies::read(const model::Model& model)
{
    ContactBodies result;
    SurfaceReader surfaces(model);
    const auto bodyOver = [&](ContactBody body, base::EntityId bsid)
    {
        if (!body.isRigid)
        {
            if (auto found = surfaces.read(bsid))
            {
                body.definitionCards.push_back(found->first);
                body.elements = std::move(found->second);
            }
            else
                body.isRigid = true; // a BCBOX, a BCSEG, a BCELIPS or a rigid surface: no element of the structure
        }
        result.m_bodies.push_back(std::move(body));
    };
    forEachCard<model::ContactBodyTable>(model,
                                         [&](const model::ContactBodyCard& card, model::CardLocation where)
                                         {
                                             ContactBody body;
                                             body.id = card.id;
                                             body.source = ContactBodySource::ContactBody;
                                             body.card = where;
                                             body.isRigid = isRigidBehaviour(card.behavior);
                                             bodyOver(std::move(body), card.surfaceId);
                                         });
    forEachCard<model::ContactBody1Table>(model,
                                          [&](const model::ContactBody1Card& card, model::CardLocation where)
                                          {
                                              ContactBody body;
                                              body.id = card.id;
                                              body.source = ContactBodySource::ContactBody1;
                                              body.card = where;
                                              body.isRigid = isRigidBehaviour(card.behavior);
                                              bodyOver(std::move(body), card.surfaceId);
                                          });
    forEachCard<model::ContactFaceRegionTable>(model,
                                               [&](const model::ContactFaceRegionCard& card, model::CardLocation where)
                                               {
                                                   ContactBody body;
                                                   body.id = card.id;
                                                   body.source = ContactBodySource::ContactFaceRegion;
                                                   body.card = where;
                                                   body.isRigid = card.form == "Rigid";
                                                   std::vector<base::EntityId> ids;
                                                   for (const model::ContactFaceRegionFace& face : card.faces)
                                                   {
                                                       if (face.idType == "Elements")
                                                           ids.push_back(face.id);
                                                   }
                                                   for (const model::ContactFaceRegionGridFace& face : card.gridFaces)
                                                       ids.push_back(face.elemId);
                                                   body.elements = surfaces.elementsById(ids);
                                                   result.m_bodies.push_back(std::move(body));
                                               });
    forEachCard<model::ContactNodeRegionTable>(model,
                                               [&](const model::ContactNodeRegionCard& card, model::CardLocation where)
                                               {
                                                   ContactBody body;
                                                   body.id = card.id;
                                                   body.source = ContactBodySource::ContactNodeRegion;
                                                   body.card = where;
                                                   body.grids = model::expandContactRuns(card.grids);
                                                   result.m_bodies.push_back(std::move(body));
                                               });
    std::stable_sort(result.m_bodies.begin(), result.m_bodies.end(), byId);

    // The regions of the legacy tables, read only when a table names them.
    const bool hasBctset = model.findTable<model::ContactSetTable>() != nullptr;
    const bool hasBconp = model.findTable<model::ContactSlidelineTable>() != nullptr;
    if (hasBctset)
    {
        forEachCard<model::ContactElemSurfaceTable>(
            model,
            [&](const model::ContactElemSurfaceCard& card, model::CardLocation where)
            {
                ContactBody region;
                region.id = card.id;
                region.source = ContactBodySource::ContactElemSurface;
                region.card = where;
                region.elements = surfaces.bsurfElements(where);
                result.m_regions.push_back(std::move(region));
            });
        std::stable_sort(result.m_regions.begin(), result.m_regions.end(), byId);
    }
    if (hasBctset || hasBconp)
    {
        forEachCard<model::SlidelineSegmentsTable>(
            model,
            [&](const model::SlidelineSegmentsCard& card, model::CardLocation where)
            {
                ContactBody line;
                line.id = card.id;
                line.source = ContactBodySource::SlidelineSegments;
                line.card = where;
                line.grids = model::expandContactRuns(card.nodes);
                result.m_lines.push_back(std::move(line));
            });
        std::stable_sort(result.m_lines.begin(), result.m_lines.end(), byId);
    }
    return result;
}

const ContactBody* ContactBodies::findBody(base::EntityId id) const noexcept
{
    return findIn(m_bodies, id);
}

const ContactBody* ContactBodies::findRegion(base::EntityId id, bool orLine) const noexcept
{
    if (const ContactBody* region = findIn(m_regions, id))
        return region;
    return orLine ? findIn(m_lines, id) : nullptr;
}

const ContactBody* ContactBodies::findLine(base::EntityId id) const noexcept
{
    return findIn(m_lines, id);
}

} // namespace oor::analysis
