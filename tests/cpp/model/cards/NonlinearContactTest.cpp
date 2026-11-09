// Checks of the non-linear and contact cards: field positions of every layout, defaults, references, ID
// spaces, renumbering and refused values of BFRIC, BSURF, BLSEG, BOUTPUT, BWIDTH, BCONP, BCPROP, BCMATL, BCRGSRF,
// BCELIPS, BCSEG, BCBOX, BCPATCH, BCBZIER, BCNURB2, BCNURBS, BCTRIM, BCRIGID, BCBODY, BCBODY1, BCSURF, BCGRID, BCONECT,
// BCTABL1, BCTABLE, BCMOVE, BCHANGE, BCPARA, BCONPRG, BCONPRP, BCBDPRP, BCAUTOP, BCSCAP, BCPFLG, BCBMRAD, BOLT, BOLT1,
// BCONUDS and the NLSTEP, NLADAPT, NLHARM, NLFREQ, NLFREQ1, NLFREQC, NLOUT, NLOUTUD, NLRGAP, NLRSFD cards.

#include "oor/model/cards/nonlinear_contact/boltConnection/BoltConnectionTable.hpp"
#include "oor/model/cards/nonlinear_contact/boltPretension/BoltPretensionTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactAutoParams/ContactAutoParamsTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactBeamCap/ContactBeamCapTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactBeamRadius/ContactBeamRadiusTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactBeamSectionFlags/ContactBeamSectionFlagsTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactBezierSurface/ContactBezierSurfaceTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactBody/ContactBodyTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactBody1/ContactBody1Table.hpp"
#include "oor/model/cards/nonlinear_contact/contactBodyChange/ContactBodyChangeTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactBodyMove/ContactBodyMoveTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactBodyParams/ContactBodyParamsTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactBoxRegion/ContactBoxRegionTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactElemSurface/ContactElemSurfaceTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactEllipsoidList/ContactEllipsoidListTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactFaceRegion/ContactFaceRegionTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactMaterialRegion/ContactMaterialRegionTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactNodeRegion/ContactNodeRegionTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactNurbsCurve/ContactNurbsCurveTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactNurbsSurface/ContactNurbsSurfaceTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactPair/ContactPairTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactPairGeomParams/ContactPairGeomParamsTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactPairPhysicalParams/ContactPairPhysicalParamsTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactParams/ContactParamsTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactPatchSurface/ContactPatchSurfaceTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactPropertyRegion/ContactPropertyRegionTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactRigidMotion/ContactRigidMotionTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactRigidSurfaceList/ContactRigidSurfaceListTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactSegment/ContactSegmentTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactSetUnion/ContactSetUnionTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactSlideline/ContactSlidelineTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactTable/ContactTableTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactTable1/ContactTable1Table.hpp"
#include "oor/model/cards/nonlinear_contact/contactTrimCurve/ContactTrimCurveTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactUserService/ContactUserServiceTable.hpp"
#include "oor/model/cards/nonlinear_contact/nonlinearAdaptiveStepping/NonlinearAdaptiveSteppingTable.hpp"
#include "oor/model/cards/nonlinear_contact/nonlinearHarmonicFreqContinuation/NonlinearHarmonicFreqContinuationTable.hpp"
#include "oor/model/cards/nonlinear_contact/nonlinearHarmonicFreqList/NonlinearHarmonicFreqListTable.hpp"
#include "oor/model/cards/nonlinear_contact/nonlinearHarmonicFreqSteps/NonlinearHarmonicFreqStepsTable.hpp"
#include "oor/model/cards/nonlinear_contact/nonlinearHarmonicParams/NonlinearHarmonicParamsTable.hpp"
#include "oor/model/cards/nonlinear_contact/nonlinearOutputItems/NonlinearOutputItemsTable.hpp"
#include "oor/model/cards/nonlinear_contact/nonlinearRadialGap/NonlinearRadialGapTable.hpp"
#include "oor/model/cards/nonlinear_contact/nonlinearSqueezeFilmDamper/NonlinearSqueezeFilmDamperTable.hpp"
#include "oor/model/cards/nonlinear_contact/nonlinearStepControl/NonlinearStepControlTable.hpp"
#include "oor/model/cards/nonlinear_contact/nonlinearUserOutput/NonlinearUserOutputTable.hpp"
#include "oor/model/cards/nonlinear_contact/slidelineFriction/SlidelineFrictionTable.hpp"
#include "oor/model/cards/nonlinear_contact/slidelineOutput/SlidelineOutputTable.hpp"
#include "oor/model/cards/nonlinear_contact/slidelineSegments/SlidelineSegmentsTable.hpp"
#include "oor/model/cards/nonlinear_contact/slidelineWidth/SlidelineWidthTable.hpp"
#include "oor/model/cards/solution_control/nonlinearStaticParams/NonlinearStaticParamsTable.hpp"
#include "oor/model/control/case_control/CaseControl.hpp"
#include "oor/model/references/ReferenceChecker.hpp"
#include "oor/model/references/Renumbering.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
using Catch::Approx;
using oor::testing::readModelText;
using oor::testing::writeModelText;

namespace
{

Model readBulk(const std::string& cards)
{
    return readModelText(cards, true);
}

template <typename TableT>
typename TableT::CardType cardAt(const Model& model, std::uint32_t row = 0)
{
    return model.findTable<TableT>()->card(row);
}

template <typename TableT>
std::size_t rowCount(const Model& model)
{
    const TableT* table = model.findTable<TableT>();
    return table == nullptr ? 0 : table->rows().rowCount();
}

std::size_t unresolvedIn(Model& model)
{
    base::DiagnosticSink sink(base::ReadMode::Tolerant);
    return ReferenceChecker::check(model, sink);
}

// The card record is an error in strict mode and in tolerant mode, where the first error names the field (when given).
void expectInvalid(const std::string& card, const std::string& field = "")
{
    INFO(card);
    REQUIRE_THROWS_AS(readModelText(card, true), base::DeckError);
    const Model tolerant = readModelText(card, false);
    REQUIRE(tolerant.diagnostics().hasErrors());
    if (!field.empty())
    {
        const auto error = tolerant.diagnostics().firstError();
        REQUIRE(error.has_value());
        INFO(error->message);
        REQUIRE(error->location.fieldName == field);
    }
}

// Records the references a card lists: the field names and what each points to.
struct RecordingVisitor final : ReferenceVisitor
{
    std::vector<std::string> fields;
    std::vector<ReferenceTarget> targets;
    base::EntityId visit(std::string_view field, ReferenceTarget target, base::EntityId id) override
    {
        fields.emplace_back(field);
        targets.push_back(target);
        return id;
    }
};

// Writes the model as a model file and reads that again: the same cards.
void requireRoundTrip(const Model& model)
{
    oor::testing::requireModelFileRoundTrip(model);
}

// The value of a named parameter of a list, or a blank one when the list does not have it.
ContactValue valueOf(const std::vector<ContactParameter>& parameters, std::string_view name)
{
    for (const ContactParameter& parameter : parameters)
    {
        if (parameter.name == name)
            return parameter.value;
    }
    return ContactValue{};
}

double realOf(const std::vector<ContactParameter>& parameters, std::string_view name)
{
    const ContactValue value = valueOf(parameters, name);
    if (const auto* real = std::get_if<double>(&value))
        return *real;
    if (const auto* integer = std::get_if<std::int64_t>(&value))
        return static_cast<double>(*integer);
    return std::nan("");
}

std::int64_t integerOf(const std::vector<ContactParameter>& parameters, std::string_view name)
{
    const ContactValue value = valueOf(parameters, name);
    const auto* integer = std::get_if<std::int64_t>(&value);
    return integer == nullptr ? -999999 : *integer;
}

std::string wordOf(const std::vector<ContactParameter>& parameters, std::string_view name)
{
    const ContactValue value = valueOf(parameters, name);
    const auto* word = std::get_if<std::string>(&value);
    return word == nullptr ? std::string("?") : *word;
}

// The section of an NLSTEP or NLADAPT with that keyword, or an empty one.
std::vector<ContactParameter> sectionOf(const std::vector<ContactNamedSection>& sections, std::string_view keyword)
{
    for (const ContactNamedSection& section : sections)
    {
        if (section.keyword == keyword)
            return section.params;
    }
    return {};
}

// The number of a Case Control command of the global section.
std::int64_t globalCommand(const Model& model, const char* name)
{
    const CaseEntry* entry = CaseControl::ownCommand(model.control().caseControl.global(), name);
    REQUIRE(entry != nullptr);
    return entry->value.integer;
}

// A hexahedron (element 1) on the grid points 1 to 8 with its property and material: what the contact cards point to.
std::string solidBlock()
{
    return "Node;1;;0.;0.;0.\nNode;2;;1.;0.;0.\nNode;3;;1.;1.;0.\nNode;4;;0.;1.;0.\n"
           "Node;5;;0.;0.;1.\nNode;6;;1.;0.;1.\nNode;7;;1.;1.;1.\nNode;8;;0.;1.;1.\n"
           "SolidHexa;1;1;1;2;3;4;5;6;7;8\nSolidProp;1;1\nIsoMat;1;2.1E+11;;0.3\n";
}

} // namespace

// ------------------------------------------------------------------------------------------ slideline contact

TEST_CASE("the slideline entries have their fields in the order of the format reference", "[model][cards][contact]")
{
    const Model model = readBulk(
        "SlidelineFriction;37;2.E+5;0.35\n"
        "SlidelineFriction;34;;0.3\n"
        "ContactElemSurface;200;1;2;3;4;5;6;7;8;9;10\n"
        "ContactElemSurface;201;100;Through;200;300;Through;350;By;10\n"
        "ContactElemSurface;202;20;Through;10\n"
        "SlidelineSegments;400;1;2;3\n"
        "SlidelineSegments;401;1;Through;6;By;1\n"
        "SlidelineSegments;402;9;Through;3;By;-2\n"
        "SlidelineOutput;410;1;2;3\n"
        "SlidelineOutput;411;All\n"
        "SlidelineOutput;412;1;Through;9;By;2\n"
        "SlidelineWidth;400;0.5;0.6\n"
        "SlidelineWidth;401;1.;Through;2.;By;0.5\n"
        "SlidelineWidth;402;3.\n"
        "ContactSlideline;420;400;401;1.5;37;2;3\n"
        "ContactSlideline;421;400;401");
    const SlidelineFrictionCard friction = cardAt<SlidelineFrictionTable>(model, 0);
    REQUIRE((friction.id == 37 && friction.stickStiffness == 2.0E5 && friction.staticFriction == 0.35));
    REQUIRE(cardAt<SlidelineFrictionTable>(model, 1).stickStiffness == 0.0);

    const ContactElemSurfaceCard list = cardAt<ContactElemSurfaceTable>(model, 0);
    REQUIRE(list.id == 200);
    REQUIRE(expandContactRuns(list.elements) == std::vector<base::EntityId>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
    REQUIRE(cardAt<ContactElemSurfaceTable>(model, 1).elements ==
            std::vector<ContactRun>{{100, 200, 1}, {300, 350, 10}});
    // A range goes down when its end is below its start: the increment BY is -1 when left out, stored resolved.
    REQUIRE(cardAt<ContactElemSurfaceTable>(model, 2).elements == std::vector<ContactRun>{{20, 10, -1}});
    REQUIRE(cardAt<SlidelineSegmentsTable>(model, 0).nodes == std::vector<ContactRun>{{1, 1, 1}, {2, 2, 1}, {3, 3, 1}});
    REQUIRE(cardAt<SlidelineSegmentsTable>(model, 1).nodes == std::vector<ContactRun>{{1, 6, 1}});
    REQUIRE(cardAt<SlidelineSegmentsTable>(model, 2).nodes == std::vector<ContactRun>{{9, 3, -2}});
    REQUIRE(expandContactRuns(cardAt<SlidelineSegmentsTable>(model, 2).nodes) ==
            std::vector<base::EntityId>{9, 7, 5, 3});

    REQUIRE_FALSE(cardAt<SlidelineOutputTable>(model, 0).all);
    REQUIRE(expandContactRuns(cardAt<SlidelineOutputTable>(model, 0).nodes) == std::vector<base::EntityId>{1, 2, 3});
    REQUIRE(cardAt<SlidelineOutputTable>(model, 1).all);
    REQUIRE(cardAt<SlidelineOutputTable>(model, 1).nodes.empty());
    REQUIRE(cardAt<SlidelineOutputTable>(model, 2).nodes == std::vector<ContactRun>{{1, 9, 2}});

    const SlidelineWidthCard widths = cardAt<SlidelineWidthTable>(model, 1);
    REQUIRE(widths.widths.size() == 1);
    REQUIRE((widths.widths[0].first == 1.0 && widths.widths[0].last == 2.0 && widths.widths[0].increment == 0.5));
    REQUIRE_FALSE(cardAt<SlidelineWidthTable>(model, 2).widths[0].isRange());
    REQUIRE(cardAt<SlidelineWidthTable>(model, 0).widths.size() == 2);

    const ContactSlidelineCard region = cardAt<ContactSlidelineTable>(model, 0);
    REQUIRE((region.id == 420 && region.secondaryLineId == 400 && region.primaryLineId == 401 &&
             region.penaltyScale == 1.5 && region.frictionId == 37 && region.penetrationKind == 2 &&
             region.frameId == 3));
    const ContactSlidelineCard plain = cardAt<ContactSlidelineTable>(model, 1);
    REQUIRE((plain.penaltyScale == 1.0 && plain.frictionId == 0 && plain.penetrationKind == 1 && plain.frameId == 0));
    requireRoundTrip(model);
}

TEST_CASE("the slideline entries refer to each other and to the grid points, and renumber with them",
          "[model][cards][contact]")
{
    Model model = readBulk(solidBlock() +
                           "SlidelineFriction;37;2.E+5;0.35\n"
                           "SlidelineSegments;400;1;2;3\n"
                           "SlidelineSegments;401;4;Through;6\n"
                           "SlidelineWidth;400;0.5\n"
                           "ContactElemSurface;200;1;Through;5\n"
                           "ContactSlideline;420;400;401;1.5;37;2\n"
                           "SlidelineOutput;410;1;2");
    REQUIRE(unresolvedIn(model) == 0);
    RecordingVisitor visitor;
    model.table<ContactSlidelineTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"secondaryLineId", "primaryLineId", "frictionId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::SlidelineSegments,
                                                            ReferenceTarget::SlidelineSegments,
                                                            ReferenceTarget::SlidelineFriction});
    RecordingVisitor widthVisitor;
    model.table<SlidelineWidthTable>().visitReferences(0, widthVisitor);
    REQUIRE(widthVisitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::SlidelineSegments});
    // The IDs of a THRU range of grid points may have gaps, a single missing one is reported; BSURF too.
    Model lacking =
        readBulk(solidBlock() +
                 "SlidelineSegments;400;1;2;99\nSlidelineSegments;401;1;Through;50\nContactElemSurface;200;1;"
                 "Through;5;50\nContactSlideline;420;400;401;1.;77");
    REQUIRE(unresolvedIn(lacking) == 3); // the grid 99, the element 50 and the BFRIC 77

    Renumbering::apply(model, RenumberSpace::SlidelineSegments, Renumbering::offset(1000));
    REQUIRE(cardAt<SlidelineSegmentsTable>(model, 0).id == 1400);
    REQUIRE(cardAt<SlidelineWidthTable>(model).segmentsId == 1400); // a BWIDTH has the ID of the BLSEG it widens
    REQUIRE((cardAt<ContactSlidelineTable>(model).secondaryLineId == 1400 &&
             cardAt<ContactSlidelineTable>(model).primaryLineId == 1401));
    Renumbering::apply(model, RenumberSpace::SlidelineFriction, Renumbering::offset(10));
    REQUIRE(cardAt<SlidelineFrictionTable>(model).id == 47);
    REQUIRE(cardAt<ContactSlidelineTable>(model).frictionId == 47);
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE(cardAt<SlidelineSegmentsTable>(model, 0).nodes ==
            std::vector<ContactRun>{{101, 101, 1}, {102, 102, 1}, {103, 103, 1}});
    REQUIRE(cardAt<SlidelineSegmentsTable>(model, 1).nodes == std::vector<ContactRun>{{104, 106, 1}});
    REQUIRE(cardAt<SlidelineOutputTable>(model).nodes == std::vector<ContactRun>{{101, 101, 1}, {102, 102, 1}});
    REQUIRE(cardAt<SlidelineOutputTable>(model).id ==
            410); // the number of a contact region, not a card: not renumbered
    Renumbering::apply(model, RenumberSpace::Element, Renumbering::offset(100));
    REQUIRE(cardAt<ContactElemSurfaceTable>(model).elements == std::vector<ContactRun>{{101, 105, 1}});
    REQUIRE(unresolvedIn(model) == 0);
}

TEST_CASE("an invalid slideline entry is reported by field", "[model][cards][contact]")
{
    expectInvalid("SlidelineFriction;0;;0.3", "id");
    expectInvalid("SlidelineFriction;1;;-0.3", "staticFriction");
    expectInvalid("SlidelineFriction;1;-1.;0.3", "stickStiffness");
    expectInvalid("ContactElemSurface;1", "");
    expectInvalid("ContactElemSurface;1;0", "");
    expectInvalid("ContactElemSurface;1;5;Through", "");
    expectInvalid("ContactElemSurface;1;5;Through;9;By;0", "");
    expectInvalid("ContactElemSurface;1;5;Through;9;By;-1", "");
    expectInvalid("SlidelineSegments;1;1;Through;6;By;-1", "");
    expectInvalid("SlidelineSegments;1", "");
    expectInvalid("SlidelineOutput;1;All;3", "");
    expectInvalid("SlidelineOutput;1", "");
    expectInvalid("SlidelineWidth;1;-1.", "");
    expectInvalid("SlidelineWidth;1", "");
    expectInvalid("ContactSlideline;1;2;3;1.;;3", "penetrationKind");
    expectInvalid("ContactSlideline;1;2;3;-1.", "penaltyScale");
    expectInvalid("ContactSlideline;1;2", "primaryLineId");
}

// ------------------------------------------------------------------------------------------------- contact bodies

TEST_CASE("the regions of a contact body have their fields in the order of the format reference",
          "[model][cards][contact]")
{
    const Model model = readBulk(
        "ContactPropertyRegion;210;1;2;3;4\n"
        "ContactPropertyRegion;211;10;Through;20;30\n"
        "ContactMaterialRegion;220;1;2;3\n"
        "ContactRigidSurfaceList;250;240;241;242\n"
        "ContactEllipsoidList;260;UPPER;MIDDLE;LOWER\n"
        "ContactSegment;1;5;10;11;12;13\n"
        "ContactSegment;2;5;10;11;12\n"
        "ContactBoxRegion;230;1;;1;2;;3;;4;;5;;6;;7;;8\n"
        "ContactBoxRegion;231;;Coordinates;0;;0.;;0.;;0.;;1.;;0.;;0.;;1.;1.;0.;0.;1.;0.;0.;0.;1.;1.;0.;1.;1.;1.;1.;0.;"
        "1.;1.\n"
        "ContactPatchSurface;240;1;1;2;3;4;2;2;5;6;3\n"
        "ContactBezierSurface;241;2;2;3;3;1;2;3;4\n"
        "ContactNurbsCurve;242;3;3;2;Node;3;1;2;3;HomogeneousCoordinates;3;1.;1.;1.;KnotVector;6;0.;0.;0.;1.;1.;1.\n"
        "ContactNurbsSurface;243;2;2;2;2;1;1;Node;4;1;2;3;4;HomogeneousCoordinates;4;1.;1.;1.;1.;KnotVector;8;0.;0.;1.;"
        "1.;0.;0.;1.;1.;Trimming;2;5;"
        "6\n"
        "ContactNurbsSurface;244;-2;2;2;2;1;1;Coordinates;12;0.;0.;0.;1.;0.;0.;0.;1.;0.;1.;1.;0.;"
        "HomogeneousCoordinates;4;1.;1.;1.;1.;KnotVector;8;0."
        ";0.;1.;1.;0.;0.;1.;1.\n"
        "ContactTrimCurve;5;3;3;2;Coordinates;6;0.;0.;0.5;1.;1.;0.;HomogeneousCoordinates;3;1.;1.;1.;KnotVector;6;0.;0."
        ";0.;1.;1.;1.");
    REQUIRE(cardAt<ContactPropertyRegionTable>(model, 0).properties ==
            std::vector<ContactRun>{{1, 1, 1}, {2, 2, 1}, {3, 3, 1}, {4, 4, 1}});
    REQUIRE(cardAt<ContactPropertyRegionTable>(model, 1).properties ==
            std::vector<ContactRun>{{10, 20, 1}, {30, 30, 1}});
    REQUIRE(cardAt<ContactMaterialRegionTable>(model).materials == std::vector<base::EntityId>{1, 2, 3});
    REQUIRE(cardAt<ContactRigidSurfaceListTable>(model).surfaces == std::vector<base::EntityId>{240, 241, 242});
    REQUIRE(cardAt<ContactEllipsoidListTable>(model).ellipsoidNames ==
            std::vector<std::string>{"UPPER", "MIDDLE", "LOWER"});
    const ContactSegmentCard quadrilateral = cardAt<ContactSegmentTable>(model, 0);
    REQUIRE((quadrilateral.id == 1 && quadrilateral.surfaceId == 5));
    REQUIRE(quadrilateral.nodes == std::array<base::EntityId, 4>{10, 11, 12, 13});
    REQUIRE(cardAt<ContactSegmentTable>(model, 1).nodes == std::array<base::EntityId, 4>{10, 11, 12, 0}); // a triangle

    const ContactBoxRegionCard grids = cardAt<ContactBoxRegionTable>(model, 0);
    REQUIRE((grids.inclusionRule == 1 && !grids.byCoordinates));
    REQUIRE(grids.cornerNodes == std::array<base::EntityId, 8>{1, 2, 3, 4, 5, 6, 7, 8});
    const ContactBoxRegionCard points = cardAt<ContactBoxRegionTable>(model, 1);
    REQUIRE((points.inclusionRule == 0 && points.byCoordinates));
    REQUIRE((points.cornerCoordinates[3] == 1.0 && points.cornerCoordinates[7] == 1.0 &&
             points.cornerCoordinates[14] == 1.0 && points.cornerCoordinates[17] == 1.0 &&
             points.cornerCoordinates[23] == 1.0 && points.cornerCoordinates[22] == 1.0 &&
             points.cornerCoordinates[20] == 1.0));

    const ContactPatchSurfaceCard patches = cardAt<ContactPatchSurfaceTable>(model);
    REQUIRE(patches.id == 240);
    REQUIRE(patches.patches.size() == 2);
    REQUIRE((patches.patches[1].patchId == 2 && patches.patches[1].grids == std::array<base::EntityId, 4>{2, 5, 6, 3}));
    const ContactBezierSurfaceCard bezier = cardAt<ContactBezierSurfaceTable>(model);
    REQUIRE(
        (bezier.pointCount1 == 2 && bezier.pointCount2 == 2 && bezier.subdivisions1 == 3 && bezier.subdivisions2 == 3));
    REQUIRE(bezier.nodes == std::vector<base::EntityId>{1, 2, 3, 4});

    const ContactNurbsCurveCard curve = cardAt<ContactNurbsCurveTable>(model);
    REQUIRE((curve.pointCountU == 3 && curve.orderU == 3 && curve.subdivisions == 2 && !curve.data.byCoordinates));
    REQUIRE(curve.data.grids == std::vector<base::EntityId>{1, 2, 3});
    REQUIRE(curve.data.homogeneous == std::vector<double>{1., 1., 1.});
    REQUIRE(curve.data.knots == std::vector<double>{0., 0., 0., 1., 1., 1.});
    const ContactNurbsSurfaceCard surface = cardAt<ContactNurbsSurfaceTable>(model, 0);
    REQUIRE((surface.pointCountU == 2 && surface.pointCountV == 2 && surface.orderU == 2 && surface.orderV == 2 &&
             surface.subdivisionsU == 1 && surface.subdivisionsV == 1));
    REQUIRE(surface.data.knots.size() == 8);
    REQUIRE(surface.trimCurveIds == std::vector<base::EntityId>{5, 6});
    const ContactNurbsSurfaceCard byCoordinates = cardAt<ContactNurbsSurfaceTable>(model, 1);
    REQUIRE((byCoordinates.pointCountU == -2 && byCoordinates.data.byCoordinates && byCoordinates.data.grids.empty()));
    REQUIRE(byCoordinates.data.coordinates.size() == 12);
    REQUIRE(byCoordinates.data.coordinates[3] == 1.0);
    const ContactTrimCurveCard trim = cardAt<ContactTrimCurveTable>(model);
    REQUIRE((trim.id == 5 && trim.pointCount == 3 && trim.order == 3 && trim.subdivisions == 2));
    REQUIRE(trim.coordinates == std::vector<double>{0., 0., 0.5, 1., 1., 0.});
    REQUIRE(trim.knots.size() == 6);
    requireRoundTrip(model);
}

TEST_CASE("BCBODY reads the first line, the line of the motion and every keyword line", "[model][cards][contact]")
{
    const Model model = readBulk(
        "ContactBody;100;ThreeDimensional;Deformable;200\n"
        "ContactBody;101;TwoDimensional;Rigid;0;0;0.3;5;-1;3;1.5;1.;0.;0.;10.;0.;0.;Advance;45.;3;1;Rigid;100;1;PUSHER;"
        "ApproachVelocity;1.;0."
        ";0.;1.;0.;0.;-1.;Growth;1.1;1.1;1.1;5;6;7;PatchSurface;2;2;1;1;2;3;4;2;2;5;6;3\n"
        "ContactBody;102;ThreeDimensional;HeatTransfer;200;;;;;HeatTransfer;1.E+3;20.;5.;20.;1.;2.;1;3.;0.8;4.;5.;6.;7."
        ";8.;9.;10.;0.5;0.5\n"
        "ContactBody;103;ThreeDimensional;Rigid;0;;;;;BezierSurface;2;2;3;3;4;1;2;3;4\n"
        "ContactBody;104;TwoDimensional;Rigid;0;;;;;NurbsCurve;27;3;3;2;;;;;1;2;3;;;;;1.;1.;1.;;;;;0.;0.;0.;1.;1.;1.\n"
        "ContactBody;105;ThreeDimensional;Rigid;0;;;;;NurbsSurface;29;2;2;2;2;1;1;0;1;2;3;4;;;;1.;1.;1.;1.;;;;0.;0.;1.;"
        "1.;0.;0.;1.;1.\n"
        "ContactBody;106;;;300;2;;17\n");
    const ContactBodyCard deformable = cardAt<ContactBodyTable>(model, 0);
    REQUIRE((deformable.id == 100 && deformable.dimension == "ThreeDimensional" &&
             deformable.behavior == "Deformable" && deformable.surfaceId == 200));
    REQUIRE((deformable.contactKind == 0 && deformable.splineSegmentsId == 0 && deformable.control == 0));
    REQUIRE(std::holds_alternative<std::monostate>(deformable.friction));
    REQUIRE(deformable.motion.isDefault());
    REQUIRE_FALSE(
        (deformable.advance || deformable.rigid || deformable.approach || deformable.growth || deformable.heat));

    const ContactBodyCard rigid = cardAt<ContactBodyTable>(model, 1);
    REQUIRE((rigid.dimension == "TwoDimensional" && rigid.behavior == "Rigid" && rigid.surfaceId == 0 &&
             rigid.splineSegmentsId == 5 && rigid.control == -1));
    REQUIRE(std::get<double>(rigid.friction) == 0.3);
    REQUIRE(rigid.motion.loadNode == 3);
    REQUIRE(std::get<double>(rigid.motion.angularVelocity) == 1.5);
    REQUIRE(std::get<double>(rigid.motion.axisCosines[0]) == 1.0);
    REQUIRE(std::get<double>(rigid.motion.rigidVelocities[0]) == 10.0);
    REQUIRE(rigid.advance.has_value());
    REQUIRE((rigid.advance->smoothingAngle == 45.0 && rigid.advance->contactOption == 3 &&
             rigid.advance->midsideProjection == 1));
    REQUIRE((rigid.rigid->referenceNode == 100 && rigid.rigid->entityCount == 1 && rigid.rigid->name == "PUSHER"));
    REQUIRE((rigid.approach->angularVelocity == 1.0 && rigid.approach->axis[2] == 1.0 &&
             rigid.approach->velocity[2] == -1.0));
    REQUIRE((rigid.growth->factors[0] == 1.1 && rigid.growth->tables == std::array<base::EntityId, 3>{5, 6, 7}));
    REQUIRE(rigid.patches->size() == 2);
    REQUIRE((*rigid.patches)[0].grids == std::array<base::EntityId, 4>{1, 2, 3, 4});

    const ContactBodyCard heat = cardAt<ContactBodyTable>(model, 2);
    REQUIRE(heat.behavior == "HeatTransfer");
    REQUIRE(heat.heat.has_value());
    REQUIRE((realOf(*heat.heat, "EnvironmentFilmCoefficient") == 1000.0 &&
             realOf(*heat.heat, "SinkTemperature") == 20.0 &&
             realOf(*heat.heat, "ContactHeatTransferCoefficient") == 5.0));
    REQUIRE((integerOf(*heat.heat, "ThermalInterfaceKind") == 1 &&
             realOf(*heat.heat, "NearContactNaturalConvectionExponent") == 3.0 &&
             realOf(*heat.heat, "EnvironmentNonlinearConvectionCoefficient") == 7.0));
    REQUIRE((realOf(*heat.heat, "EnvironmentNaturalConvectionCoefficient") == 9.0 &&
             realOf(*heat.heat, "AdditionalHeatCoefficient2") == 0.5));
    REQUIRE(heat.heat->size() == 18);

    const ContactBodyCard bezier = cardAt<ContactBodyTable>(model, 3);
    REQUIRE((bezier.bezier->pointCount1 == 2 && bezier.bezier->subdivisions2 == 3 &&
             bezier.bezier->grids == std::vector<base::EntityId>{1, 2, 3, 4}));
    REQUIRE(cardAt<ContactBodyTable>(model, 4).nurbs2d->lines.size() == 4);
    REQUIRE(std::get<std::int64_t>(cardAt<ContactBodyTable>(model, 4).nurbs2d->lines[0][1]) == 3);
    const ContactBodyCard nurbs = cardAt<ContactBodyTable>(model, 5);
    REQUIRE(nurbs.nurbs->lines.size() == 5);
    REQUIRE(std::get<double>(nurbs.nurbs->lines[3][6]) == 1.0);
    // The first line alone, with its defaults: DIM 3D, BEHAV DEFORM; blanks keep them.
    const ContactBodyCard blanks = cardAt<ContactBodyTable>(model, 6);
    REQUIRE((blanks.dimension == "ThreeDimensional" && blanks.behavior == "Deformable" && blanks.surfaceId == 300 &&
             blanks.contactKind == 2 && blanks.friction.index() == 0));
    REQUIRE(blanks.splineSegmentsId == 17);
    requireRoundTrip(model);
}

TEST_CASE("BCBODY1, BCSURF, BCGRID and BCRIGID have their layouts, the forms of BCSURF and of BCGRID included",
          "[model][cards][contact]")
{
    const Model model = readBulk(
        "ContactBody1;110;;ThreeDimensional;Deformable;200\n"
        "ContactBody1;111;300;TwoDimensional;Rigid;250;7;-1\n"
        "ContactBody1;112;;;;5\n"
        "ContactFaceRegion;120;;ThreeDimensional;Face;No;10;1;S1;Elements;;2;S2;Elements;;5;Top;Set\n"
        "ContactFaceRegion;121;;ThreeDimensional;Node;;;1;10;11;12;2;13;14;15\n"
        "ContactFaceRegion;122;;ThreeDimensional;Rigid;;;250;7\n"
        "ContactFaceRegion;123;5;TwoDimensional;;;;7\n"
        "ContactNodeRegion;130;300;ThreeDimensional;1;2;3;Through;10;By;2\n"
        "ContactNodeRegion;131;5;6;7;Through;12\n"
        "ContactNodeRegion;132;;TwoDimensional;9\n"
        "ContactRigidMotion;7;100;0;;1.5;1.;0.;0.;10.;0.;0.;ApproachVelocity;1.;0.;0.;1.;0.;0.;-1.;Growth;1.1;1.1;1.1;"
        "5;6;7\n"
        "ContactRigidMotion;8;101;-1\n"
        "ContactRigidMotion;9;;5;4");
    const ContactBody1Card deformable = cardAt<ContactBody1Table>(model, 0);
    REQUIRE((deformable.id == 110 && deformable.bodyParamsId == 0 && deformable.dimension == "ThreeDimensional" &&
             deformable.behavior == "Deformable" && deformable.surfaceId == 200 && deformable.rigidMotionId == 0 &&
             deformable.forceReferencePoint == 0));
    const ContactBody1Card rigid = cardAt<ContactBody1Table>(model, 1);
    REQUIRE((rigid.bodyParamsId == 300 && rigid.dimension == "TwoDimensional" && rigid.behavior == "Rigid" &&
             rigid.surfaceId == 250 && rigid.rigidMotionId == 7 && rigid.forceReferencePoint == -1));
    REQUIRE((cardAt<ContactBody1Table>(model, 2).dimension == "ThreeDimensional" &&
             cardAt<ContactBody1Table>(model, 2).behavior == "Deformable" &&
             cardAt<ContactBody1Table>(model, 2).surfaceId == 5));

    const ContactFaceRegionCard faces = cardAt<ContactFaceRegionTable>(model, 0);
    REQUIRE((faces.form == "Face" && faces.dimension == "ThreeDimensional" && faces.includeThickness == "No" &&
             faces.edgeContact == 10));
    REQUIRE(faces.faces.size() == 3);
    REQUIRE((faces.faces[0].id == 1 && faces.faces[0].face == "S1" && faces.faces[0].idType == "Elements"));
    REQUIRE((faces.faces[2].id == 5 && faces.faces[2].face == "Top" && faces.faces[2].idType == "Set"));
    const ContactFaceRegionCard grids = cardAt<ContactFaceRegionTable>(model, 1);
    REQUIRE((grids.form == "Node" && grids.dimension == "ThreeDimensional" && grids.gridFaces.size() == 2));
    REQUIRE((grids.gridFaces[1].elemId == 2 && grids.gridFaces[1].grids == std::array<base::EntityId, 3>{13, 14, 15}));
    const ContactFaceRegionCard rigidSurface = cardAt<ContactFaceRegionTable>(model, 2);
    REQUIRE((rigidSurface.form == "Rigid" && rigidSurface.surfaceId == 250 && rigidSurface.rigidMotionId == 7));
    const ContactFaceRegionCard blank = cardAt<ContactFaceRegionTable>(model, 3);
    REQUIRE((blank.form == "Face" && blank.bodyParamsId == 5 && blank.dimension == "TwoDimensional" &&
             blank.includeThickness == "Yes" && blank.edgeContact == 1));
    REQUIRE(blank.faces.size() == 1);
    REQUIRE((blank.faces[0].id == 7 && blank.faces[0].face.empty() && blank.faces[0].idType == "Elements"));

    const ContactNodeRegionCard region = cardAt<ContactNodeRegionTable>(model, 0);
    REQUIRE((!region.sol700 && region.bodyParamsId == 300 && region.dimension == "ThreeDimensional"));
    REQUIRE(region.grids == std::vector<ContactRun>{{1, 1, 1}, {2, 2, 1}, {3, 10, 2}});
    const ContactNodeRegionCard cloud = cardAt<ContactNodeRegionTable>(model, 1);
    REQUIRE((cloud.sol700 && cloud.bodyParamsId == 0 && cloud.dimension == "ThreeDimensional"));
    REQUIRE(cloud.grids == std::vector<ContactRun>{{5, 5, 1}, {6, 6, 1}, {7, 12, 1}});
    // One grid point and a blank field 3 is the first form (the format reference has no keyword to tell the two apart).
    const ContactNodeRegionCard single = cardAt<ContactNodeRegionTable>(model, 2);
    REQUIRE(
        (!single.sol700 && single.dimension == "TwoDimensional" && single.grids == std::vector<ContactRun>{{9, 9, 1}}));

    const ContactRigidMotionCard motion = cardAt<ContactRigidMotionTable>(model, 0);
    REQUIRE((motion.id == 7 && motion.referenceNode == 100 && motion.control == 0 && motion.motion.loadNode == 0));
    REQUIRE((std::get<double>(motion.motion.angularVelocity) == 1.5 &&
             std::get<double>(motion.motion.rigidVelocities[0]) == 10.0));
    REQUIRE((motion.approach->angularVelocity == 1.0 && motion.growth->tables[2] == 7));
    REQUIRE((cardAt<ContactRigidMotionTable>(model, 1).referenceNode == 101 &&
             cardAt<ContactRigidMotionTable>(model, 1).control == -1));
    REQUIRE(cardAt<ContactRigidMotionTable>(model, 2).control == 5);
    requireRoundTrip(model);
}

TEST_CASE("the contact bodies, regions and rigid surfaces refer to each other, to grid points and to elements",
          "[model][cards][contact]")
{
    Model model =
        readBulk(solidBlock() +
                 "ContactElemSurface;10;1\n"
                 "ContactPropertyRegion;11;1\n"
                 "ContactMaterialRegion;12;1\n"
                 "ContactBoxRegion;13;0;;1;2;;3;;4;;5;;6;;7;;8\n"
                 "ContactPatchSurface;20;1;1;2;3;4\n"
                 "ContactBezierSurface;21;2;2;1;1;5;6;7;8\n"
                 "ContactNurbsCurve;22;2;2;1;Node;2;1;2;HomogeneousCoordinates;2;1.;1.;KnotVector;4;0.;0.;1.;1.\n"
                 "ContactNurbsSurface;23;2;2;2;2;1;1;Node;4;1;2;3;4;HomogeneousCoordinates;4;1.;1.;1.;1.;KnotVector;8;"
                 "0.;0.;1.;1.;0.;0.;1.;1.;"
                 "Trimming;1;30\n"
                 "ContactTrimCurve;30;3;3;2;Coordinates;6;0.;0.;0.5;1.;1.;0.;HomogeneousCoordinates;3;1.;1.;1.;"
                 "KnotVector;6;0.;0.;0.;1.;1.;1.\n"
                 "ContactRigidSurfaceList;24;20;21;22;23\n"
                 "ContactRigidMotion;40;1;5\n"
                 "ContactBodyParams;50;FrictionCoefficient;0.1\n"
                 "ContactBody1;1;50;ThreeDimensional;Deformable;10\n"
                 "ContactBody1;2;;ThreeDimensional;Rigid;24;40\n"
                 "ContactBody;3;ThreeDimensional;Deformable;11\n"
                 "ContactFaceRegion;4;50;ThreeDimensional;Face;;;1;S1\n"
                 "ContactNodeRegion;5;50;ThreeDimensional;1;2;3\n"
                 "ContactNodeRegion;6;7;8");
    REQUIRE(unresolvedIn(model) == 0);

    RecordingVisitor body;
    model.table<ContactBody1Table>().visitReferences(1, body);
    REQUIRE(body.fields == std::vector<std::string>{"surfaceId", "rigidMotionId"});
    REQUIRE(body.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::ContactSurface, ReferenceTarget::ContactRigid});
    RecordingVisitor surface;
    model.table<ContactFaceRegionTable>().visitReferences(0, surface);
    REQUIRE(surface.fields == std::vector<std::string>{"bodyParamsId", "faceElemId"});
    REQUIRE(surface.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::ContactBodyParameters, ReferenceTarget::Element});
    // The SOL 700 cloud of grid points is a surface a BCBODY may be made of (a BSID).
    Model cloud = readBulk(solidBlock() + "ContactNodeRegion;6;7;8\nContactBody;3;ThreeDimensional;Deformable;6");
    REQUIRE(unresolvedIn(cloud) == 0);
    Model lacking = readBulk(solidBlock() +
                             "ContactBody1;1;50;ThreeDimensional;Deformable;10\nContactBody1;2;;ThreeDimensional;Rigid;"
                             "24;40\nContactFaceRegion;4;;ThreeDimensional;Face;;;9;S1");
    REQUIRE(unresolvedIn(lacking) == 5); // BCBDPRP 50, BSURF 10, BCRGSRF 24, BCRIGID 40 and the element 9

    // A body is of the category of the bodies: BCBODY, BCBODY1, BCSURF and BCGRID number the same set.
    Renumbering::apply(model, RenumberSpace::ContactBody, Renumbering::offset(100));
    REQUIRE((cardAt<ContactBody1Table>(model, 0).id == 101 && cardAt<ContactBody1Table>(model, 1).id == 102));
    REQUIRE((cardAt<ContactBodyTable>(model).id == 103 && cardAt<ContactFaceRegionTable>(model).id == 104));
    REQUIRE((cardAt<ContactNodeRegionTable>(model, 0).id == 105 && cardAt<ContactNodeRegionTable>(model, 1).id == 106));
    // The surfaces: BSURF, BCPROP, BCMATL, BCBOX and the rigid surfaces; a BSID, a BCRGSRF list and BCBODY follow.
    Renumbering::apply(model, RenumberSpace::ContactSurface, Renumbering::offset(1000));
    REQUIRE(cardAt<ContactElemSurfaceTable>(model).id == 1010);
    REQUIRE(cardAt<ContactBoxRegionTable>(model).id == 1013);
    REQUIRE(cardAt<ContactPatchSurfaceTable>(model).id == 1020);
    REQUIRE(cardAt<ContactRigidSurfaceListTable>(model).id == 1024);
    REQUIRE(cardAt<ContactRigidSurfaceListTable>(model).surfaces ==
            std::vector<base::EntityId>{1020, 1021, 1022, 1023});
    REQUIRE(cardAt<ContactBody1Table>(model, 0).surfaceId == 1010);
    REQUIRE(cardAt<ContactBody1Table>(model, 1).surfaceId == 1024);
    REQUIRE(cardAt<ContactBodyTable>(model).surfaceId == 1011);
    Renumbering::apply(model, RenumberSpace::ContactRigid, Renumbering::offset(100));
    REQUIRE(cardAt<ContactRigidMotionTable>(model).id == 140);
    REQUIRE(cardAt<ContactBody1Table>(model, 1).rigidMotionId == 140);
    Renumbering::apply(model, RenumberSpace::ContactBodyParameters, Renumbering::offset(100));
    REQUIRE(
        (cardAt<ContactBodyParamsTable>(model).id == 150 && cardAt<ContactBody1Table>(model, 0).bodyParamsId == 150));
    REQUIRE((cardAt<ContactFaceRegionTable>(model).bodyParamsId == 150 &&
             cardAt<ContactNodeRegionTable>(model, 0).bodyParamsId == 150));
    Renumbering::apply(model, RenumberSpace::TrimmingCurve, Renumbering::offset(100));
    REQUIRE((cardAt<ContactTrimCurveTable>(model).id == 130 &&
             cardAt<ContactNurbsSurfaceTable>(model).trimCurveIds == std::vector<base::EntityId>{130}));
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(10));
    REQUIRE(cardAt<ContactBezierSurfaceTable>(model).nodes == std::vector<base::EntityId>{15, 16, 17, 18});
    REQUIRE(cardAt<ContactNurbsSurfaceTable>(model).data.grids == std::vector<base::EntityId>{11, 12, 13, 14});
    REQUIRE(cardAt<ContactRigidMotionTable>(model).referenceNode == 11);
    REQUIRE(cardAt<ContactRigidMotionTable>(model).control == 15); // the grid point that carries a load: a node
    REQUIRE(cardAt<ContactBoxRegionTable>(model).cornerNodes[0] == 11);
    Renumbering::apply(model, RenumberSpace::Element, Renumbering::offset(100));
    REQUIRE(cardAt<ContactElemSurfaceTable>(model).elements == std::vector<ContactRun>{{101, 101, 1}});
    REQUIRE(cardAt<ContactFaceRegionTable>(model).faces[0].id == 101);
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(10));
    REQUIRE(cardAt<ContactPropertyRegionTable>(model).properties == std::vector<ContactRun>{{11, 11, 1}});
    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(10));
    REQUIRE(cardAt<ContactMaterialRegionTable>(model).materials == std::vector<base::EntityId>{11});
    REQUIRE(unresolvedIn(model) == 0);
}

TEST_CASE("an invalid contact body, region or rigid surface is reported by field", "[model][cards][contact]")
{
    expectInvalid("ContactPropertyRegion;1", "");
    expectInvalid("ContactPropertyRegion;1;5;Through;3", "");
    expectInvalid("ContactPropertyRegion;1;0", "");
    expectInvalid("ContactMaterialRegion;1", "");
    expectInvalid("ContactMaterialRegion;1;0", "");
    expectInvalid("ContactRigidSurfaceList;1", "");
    expectInvalid("ContactEllipsoidList;1", "");
    expectInvalid("ContactSegment;0;5;1;2;3;4", "id");
    expectInvalid("ContactSegment;1;5;1;2", "");
    expectInvalid("ContactSegment;1;5;1;2;2;3", "");
    expectInvalid("ContactBoxRegion;1;2;;1;2;;3;;4;;5;;6;;7;;8", "inclusionRule");
    expectInvalid("ContactBoxRegion;1;0;;1;2;;3;;4;;5;;6;;7", "");
    expectInvalid("ContactPatchSurface;1", "");
    expectInvalid("ContactPatchSurface;1;1;1;2;3;3", "");
    expectInvalid("ContactBezierSurface;1;2;2;3;3;1;2;3", "");
    expectInvalid(
        "ContactNurbsCurve;1;3;3;2;Node;2;1;2;HomogeneousCoordinates;3;1.;1.;1.;KnotVector;6;0.;0.;0.;1.;1.;1.", "");
    expectInvalid(
        "ContactNurbsCurve;1;3;3;2;Node;3;1;2;3;Coordinates;6;0.;0.;1.;1.;2.;2.;HomogeneousCoordinates;3;1.;1.;1.;"
        "KnotVector;6;0.;0.;0.;1.;1.;1.",
        "");
    expectInvalid(
        "ContactNurbsSurface;1;2;2;2;2;1;1;Node;4;1;2;3;4;HomogeneousCoordinates;4;1.;1.;1.;1.;KnotVector;4;0.;0.;1.;"
        "1.",
        "");
    expectInvalid(
        "ContactNurbsSurface;1;2;2;2;2;1;1;Node;4;1;2;3;4;HomogeneousCoordinates;4;1.;1.;1.;2.;KnotVector;8;0.;0.;1.;1."
        ";0.;0.;1.;1.",
        "");
    expectInvalid(
        "ContactTrimCurve;1;3;3;2;Coordinates;5;0.;0.;0.5;1.;1.;HomogeneousCoordinates;3;1.;1.;1.;KnotVector;6;0.;0.;0."
        ";1.;1.;1.",
        "");
    expectInvalid("ContactBody;1;4D", "dimension");
    expectInvalid("ContactBody;1;ThreeDimensional;SOFT", "behavior");
    expectInvalid("ContactBody;1;ThreeDimensional;Deformable;5;-1", "contactKind");
    expectInvalid("ContactBody;1;ThreeDimensional;Deformable;5;0;-0.1", "friction");
    expectInvalid("ContactBody;1;ThreeDimensional;Deformable;5;0;;;-2", "control");
    expectInvalid("ContactBody;1;ThreeDimensional;Rigid;0;;;;;PatchSurface;2;1;1;1;2;3;4", "patchCount");
    expectInvalid("ContactBody;1;ThreeDimensional;Rigid;0;;;;;BezierSurface;2;2;3;3;3;1;2;3", "node");
    expectInvalid("ContactBody;1;ThreeDimensional;Rigid;0;;;;;Rigid;0;0", "entityCount");
    expectInvalid("ContactBody1;0;;;;5", "id");
    expectInvalid("ContactBody1;1", "surfaceId");
    expectInvalid("ContactBody1;1;;4D", "dimension");
    expectInvalid("ContactBody1;1;;ThreeDimensional;BAD", "behavior");
    expectInvalid("ContactBody1;1;;ThreeDimensional;Deformable;5;;-2", "forceReferencePoint");
    expectInvalid("ContactFaceRegion;1;;ThreeDimensional;SKIN", "form");
    expectInvalid("ContactFaceRegion;1;;ThreeDimensional;Face;MAYBE", "includeThickness");
    expectInvalid("ContactFaceRegion;1;;ThreeDimensional;Face;Yes;3", "edgeContact");
    expectInvalid("ContactFaceRegion;1;;ThreeDimensional;Face;;;1;S7", "");
    expectInvalid("ContactFaceRegion;1;;ThreeDimensional;Face;;;1;S1;LIST", "");
    expectInvalid("ContactFaceRegion;1;;ThreeDimensional;Node;;;1;2;3", "elemId");
    expectInvalid("ContactFaceRegion;1;;TwoDimensional;Node;;;1;2;3;4",
                  "elemId"); // an edge of a 2D body has two corners
    expectInvalid("ContactFaceRegion;1;;TwoDimensional;Node;;;1;2;;4", "elemId"); // G1 and G2, not G3
    expectInvalid("ContactFaceRegion;1;;ThreeDimensional;Rigid", "");
    expectInvalid("ContactNodeRegion;1", "nodeId");
    expectInvalid("ContactNodeRegion;1;;5D;5", "dimension");
    expectInvalid("ContactRigidMotion;0", "id");
    expectInvalid("ContactRigidMotion;1;;-2", "control");
}

// ----------------------------------------------------------------------------------------- pairs and contact tables

TEST_CASE("BCONECT, BCTABL1, BCTABLE, BCMOVE and BCHANGE have their layouts", "[model][cards][contact]")
{
    const Model model = readBulk(
        "ContactPair;330;310;320;110;111\n"
        "ContactPair;331;;;;;Secondary;3;110;120;121;Primary;8;111;122;130;131;132;133;134;135\n"
        "ContactPair;332;;;;;Secondary;1;0;Primary;1;111\n"
        "ContactTable1;340;330;331\n"
        "ContactTable1;341;330;331;Through;333\n"
        "ContactTable1;;330\n"
        "ContactTable;350;110;111;1;3;2\n"
        "ContactTable;351;;;2;0;0;Secondary;110;0.01;1.E-5;0.3;0.;1;1;1;0;0.5;;;;;;;;;;;;;;;;;;;;;FrictionAndBias;0.1;"
        "0.01;0.2;0.3;"
        "1;2;"
        "GlueBreak;1.;1.;1.;1.;SegmentContact;1.E+5;0.1;1.E+4;0.01;HeatTransfer;1.;2.;3.;4.;0.9;5.;6.;7.;1;Primary;8;"
        "111;122;130;131;132;133;"
        "134;"
        "135;Secondary;120;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;Primary;1;121\n"
        "ContactTable;352;;;1;;;Secondary;0;0.01;1.E-5;0.3;0.;0;1;1;0;0.5;1.E+5;1.5;Full;No;0.1;0.2;0.3;1.1;0.;1.E+3;0."
        "5;"
        "5;3;"
        "Yes;Both;Both;FACTOR;0.1;0.2;On;Primary;1;111\n"
        "ContactTable;353;;;1;;;Secondary;110;;;;;1;;;2;;;;;;;;;;;;;;;;;;;;;;Primary;1;111\n"
        "ContactBodyMove;1;Approach\n"
        "ContactBodyMove;2;Release;110;111\n"
        "ContactBodyMove;3\n"
        "ContactBodyChange;3;Node;110;1;100;1;111;5;50;2\n"
        "ContactBodyChange;4;Exclude;110;-1;2;1;110;3;4;1");
    const ContactPairCard shortForm = cardAt<ContactPairTable>(model, 0);
    REQUIRE((shortForm.id == 330 && shortForm.geomParamsId == 310 && shortForm.physicalParamsId == 320 &&
             shortForm.secondaryBodyId == 110 && shortForm.primaryBodyId == 111 && shortForm.secondaryBodies.empty() &&
             shortForm.primaryBodies.empty()));
    const ContactPairCard lists = cardAt<ContactPairTable>(model, 1);
    REQUIRE((lists.secondaryBodyId == 0 && lists.primaryBodyId == 0 && lists.geomParamsId == 0 &&
             lists.physicalParamsId == 0));
    REQUIRE(lists.secondaryBodies == std::vector<base::EntityId>{110, 120, 121});
    REQUIRE(lists.primaryBodies == std::vector<base::EntityId>{111, 122, 130, 131, 132, 133, 134, 135});
    REQUIRE(cardAt<ContactPairTable>(model, 2).secondaryBodies ==
            std::vector<base::EntityId>{0}); // SOL 700: every element

    REQUIRE(cardAt<ContactTable1Table>(model, 0).pairs == std::vector<ContactRun>{{330, 330, 1}, {331, 331, 1}});
    REQUIRE(cardAt<ContactTable1Table>(model, 1).pairs == std::vector<ContactRun>{{330, 330, 1}, {331, 333, 1}});
    REQUIRE(cardAt<ContactTable1Table>(model, 2).id == 0); // a blank BCID is the load case 0

    const ContactTableCard shortTable = cardAt<ContactTableTable>(model, 0);
    REQUIRE((shortTable.id == 350 && shortTable.secondaryBodyId == 110 && shortTable.primaryBodyId == 111 &&
             shortTable.setCount == 1 && shortTable.secondaryContactOption == 3 &&
             shortTable.primaryContactOption == 2 && shortTable.groups.empty()));
    const ContactTableCard sets = cardAt<ContactTableTable>(model, 1);
    REQUIRE((sets.setCount == 2 && sets.groups.size() == 2));
    const ContactTableGroup& first = sets.groups[0];
    REQUIRE((first.secondaryBodyId == 110 &&
             first.primaryBodies == std::vector<base::EntityId>{111, 122, 130, 131, 132, 133, 134, 135}));
    REQUIRE((realOf(first.params, "ContactDistanceTolerance") == 0.01 &&
             realOf(first.params, "SeparationForce") == 1.E-5 && realOf(first.params, "FrictionCoefficient") == 0.3));
    REQUIRE((realOf(first.params, "InterferenceClosure") == 0.0 && integerOf(first.params, "GlueOption") == 1));
    REQUIRE((integerOf(first.params, "ContactSearchOrder") == 1 &&
             integerOf(first.params, "StressFreeInitialContact") == 1 &&
             integerOf(first.params, "GlueSeparationOption") == 0));
    REQUIRE(realOf(first.params, "NearContactDistance") == 0.5);
    REQUIRE(
        (realOf(first.params, "FrictionStressLimit") == 0.1 && realOf(first.params, "ContactToleranceBias") == 0.01 &&
         realOf(first.params, "SlidingVelocityThreshold") == 0.2 && realOf(first.params, "HardSoftRatio") == 0.3 &&
         integerOf(first.params, "TouchingBodyOptions") == 1 && integerOf(first.params, "TouchedBodyOptions") == 2));
    REQUIRE((realOf(first.params, "GlueBreakNormalStress") == 1.0 &&
             realOf(first.params, "GlueBreakShearExponent") == 1.0));
    REQUIRE((realOf(first.params, "NormalPenaltyFactor") == 1.E5 &&
             realOf(first.params, "AugmentationDistance") == 0.1 &&
             realOf(first.params, "TangentPenaltyFactor") == 1.E4 && realOf(first.params, "StickSlip") == 0.01));
    REQUIRE((realOf(first.params, "ContactHeatTransferCoefficient") == 1.0 &&
             realOf(first.params, "Emissivity") == 0.9 &&
             realOf(first.params, "DistanceDependentConvectionCoefficient") == 5.0 &&
             realOf(first.params, "NearContactNonlinearConvectionCoefficient") == 6.0 &&
             realOf(first.params, "NearContactNonlinearConvectionExponent") == 7.0 &&
             integerOf(first.params, "GlueHeatTransfer") == 1));
    REQUIRE((sets.groups[1].secondaryBodyId == 120 && sets.groups[1].params.empty() &&
             sets.groups[1].primaryBodies == std::vector<base::EntityId>{121}));

    const ContactTableCard explosiveCard = cardAt<ContactTableTable>(model, 2);
    const ContactTableGroup& explosive = explosiveCard.groups[0];
    REQUIRE((explosive.secondaryBodyId == 0 && explosive.primaryBodies == std::vector<base::EntityId>{111}));
    REQUIRE((realOf(explosive.params, "KineticFrictionCoefficient") == 1.E5 &&
             realOf(explosive.params, "FrictionDecayCoefficient") == 1.5 &&
             wordOf(explosive.params, "ContactMethod") == "Full" &&
             wordOf(explosive.params, "AdaptiveContactSurface") == "No" &&
             realOf(explosive.params, "ContactThickness") == 0.1 &&
             realOf(explosive.params, "ThicknessOffset") == 0.2 &&
             realOf(explosive.params, "PenetrationLimit") == 0.3));
    REQUIRE((realOf(explosive.params, "PenaltyScaleFactor") == 1.1 &&
             realOf(explosive.params, "ContactStartTime") == 0.0 &&
             realOf(explosive.params, "ContactEndTime") == 1.E3 &&
             realOf(explosive.params, "MaxParametricCoordinate") == 0.5 &&
             integerOf(explosive.params, "InitialPenetrationAdjustment") == 5 &&
             integerOf(explosive.params, "IgnoreInitialPenetration") == 3));
    REQUIRE((wordOf(explosive.params, "AutomaticContact") == "Yes" &&
             wordOf(explosive.params, "ContactSide") == "Both" &&
             wordOf(explosive.params, "ContactForceWeighting") == "Both" &&
             wordOf(explosive.params, "MonitorDisplacement") == "FACTOR" &&
             realOf(explosive.params, "MonitorDisplacementValue") == 0.1 &&
             realOf(explosive.params, "InitialMonitor") == 0.2 && wordOf(explosive.params, "ContactDamping") == "On"));

    const ContactTableCard gluedCard = cardAt<ContactTableTable>(model, 3);
    const ContactTableGroup& glued = gluedCard.groups[0];
    REQUIRE((integerOf(glued.params, "GlueOption") == 1 && integerOf(glued.params, "GlueSeparationOption") == 2 &&
             glued.params.size() == 2));

    REQUIRE((cardAt<ContactBodyMoveTable>(model, 0).moveKind == "Approach" &&
             cardAt<ContactBodyMoveTable>(model, 0).bodies.empty()));
    REQUIRE((cardAt<ContactBodyMoveTable>(model, 1).moveKind == "Release" &&
             cardAt<ContactBodyMoveTable>(model, 1).bodies == std::vector<base::EntityId>{110, 111}));
    REQUIRE(cardAt<ContactBodyMoveTable>(model, 2).moveKind == "Approach");
    const ContactBodyChangeCard nodes = cardAt<ContactBodyChangeTable>(model, 0);
    REQUIRE((nodes.id == 3 && nodes.kind == "Node" && nodes.ranges.size() == 2));
    REQUIRE((nodes.ranges[0].bodyId == 110 && nodes.ranges[0].firstNode == 1 && nodes.ranges[0].lastNode == 100 &&
             nodes.ranges[0].increment == 1));
    REQUIRE((nodes.ranges[1].bodyId == 111 && nodes.ranges[1].firstNode == 5 && nodes.ranges[1].lastNode == 50 &&
             nodes.ranges[1].increment == 2));
    const ContactBodyChangeCard exclude = cardAt<ContactBodyChangeTable>(model, 1);
    REQUIRE((exclude.kind == "Exclude" && exclude.ranges[0].firstNode == -1 && exclude.ranges[1].firstNode == 3));
    requireRoundTrip(model);
}

TEST_CASE("the contact pairs and tables refer to bodies, parameters and each other, and renumber with them",
          "[model][cards][contact]")
{
    Model model = readBulk(solidBlock() +
                           "ContactElemSurface;10;1\n"
                           "ContactBody1;1;;ThreeDimensional;Deformable;10\n"
                           "ContactBody1;2;;ThreeDimensional;Deformable;10\n"
                           "ContactBody1;3;;ThreeDimensional;Deformable;10\n"
                           "ContactPairGeomParams;31;ContactSearchOrder;1\n"
                           "ContactPairPhysicalParams;32;FrictionCoefficient;0.1\n"
                           "ContactPair;5;31;32;1;2\n"
                           "ContactPair;6;;;;;Secondary;2;1;3;Primary;1;2\n"
                           "ContactPair;4;;;1;3\n"
                           "ContactTable1;7;5;6\n"
                           "ContactTable1;8;4;5;Through;6\n"
                           "ContactTable;9;1;2\n"
                           "ContactTable;10;;;1;;;Secondary;1;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;Primary;2;2;3\n"
                           "ContactBodyMove;11;Release;2\n"
                           "ContactBodyChange;12;Node;1;1;4;1\n"
                           "ContactBodyChange;13;Exclude;2;-1;2;1;2;3;4;1");
    REQUIRE(unresolvedIn(model) == 0);
    RecordingVisitor connection;
    model.table<ContactPairTable>().visitReferences(0, connection);
    REQUIRE(connection.targets == std::vector<ReferenceTarget>{ReferenceTarget::ContactGeometricParameters,
                                                               ReferenceTarget::ContactPhysicalParameters,
                                                               ReferenceTarget::ContactBody,
                                                               ReferenceTarget::ContactBody});
    RecordingVisitor table;
    model.table<ContactTable1Table>().visitReferences(1, table);
    // a single ID, then the two ends of the range
    REQUIRE(table.targets == std::vector<ReferenceTarget>(3, ReferenceTarget::ContactPair));
    Model lacking =
        readBulk(solidBlock() +
                 "ContactElemSurface;10;1\nContactBody1;1;;ThreeDimensional;Deformable;10\nContactPair;5;31;32;1;"
                 "2\nContactTable1;7;5;6\nContactTable;9;1;2\nContactBodyMove;11;Release;2");
    // the BCONPRG 31, the BCONPRP 32, the body 2 of the pair, the BCONECT 6, the body 2 of the table and of the move
    REQUIRE(unresolvedIn(lacking) == 6);

    // The IDs of the bodies, in every card that names one.
    Renumbering::apply(model, RenumberSpace::ContactBody, Renumbering::offset(100));
    REQUIRE((cardAt<ContactPairTable>(model, 0).secondaryBodyId == 101 &&
             cardAt<ContactPairTable>(model, 0).primaryBodyId == 102));
    REQUIRE(cardAt<ContactPairTable>(model, 1).secondaryBodies == std::vector<base::EntityId>{101, 103});
    REQUIRE(cardAt<ContactPairTable>(model, 1).primaryBodies == std::vector<base::EntityId>{102});
    REQUIRE((cardAt<ContactTableTable>(model, 0).secondaryBodyId == 101 &&
             cardAt<ContactTableTable>(model, 0).primaryBodyId == 102));
    REQUIRE(cardAt<ContactTableTable>(model, 1).groups[0].secondaryBodyId == 101);
    REQUIRE(cardAt<ContactTableTable>(model, 1).groups[0].primaryBodies == std::vector<base::EntityId>{102, 103});
    REQUIRE(cardAt<ContactBodyMoveTable>(model).bodies == std::vector<base::EntityId>{102});
    REQUIRE(cardAt<ContactBodyChangeTable>(model, 0).ranges[0].bodyId == 101);
    REQUIRE(cardAt<ContactBodyChangeTable>(model, 1).ranges[0].firstNode ==
            -1); // the minus sign marks the first range of a patch
    REQUIRE(cardAt<ContactBodyChangeTable>(model, 1).ranges[0].bodyId == 102);
    // The nodes of a BCHANGE are renumbered with their sign.
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(10));
    REQUIRE(cardAt<ContactBodyChangeTable>(model, 1).ranges[0].firstNode == -11);
    REQUIRE(cardAt<ContactBodyChangeTable>(model, 1).ranges[0].lastNode == 12);
    REQUIRE(cardAt<ContactBodyChangeTable>(model, 0).ranges[0].lastNode == 14);
    // The pairs: the BCONECT ID and the lists of the tables.
    Renumbering::apply(model, RenumberSpace::ContactPair, Renumbering::offset(50));
    REQUIRE((cardAt<ContactPairTable>(model, 0).id == 55 && cardAt<ContactPairTable>(model, 1).id == 56));
    REQUIRE(cardAt<ContactTable1Table>(model, 0).pairs == std::vector<ContactRun>{{55, 55, 1}, {56, 56, 1}});
    REQUIRE(cardAt<ContactTable1Table>(model, 1).pairs == std::vector<ContactRun>{{54, 54, 1}, {55, 56, 1}});
    REQUIRE(cardAt<ContactPairTable>(model, 2).id == 54);
    Renumbering::apply(model, RenumberSpace::ContactGeometricParameters, Renumbering::offset(100));
    Renumbering::apply(model, RenumberSpace::ContactPhysicalParameters, Renumbering::offset(200));
    REQUIRE((cardAt<ContactPairGeomParamsTable>(model).id == 131 &&
             cardAt<ContactPairTable>(model, 0).geomParamsId == 131));
    REQUIRE((cardAt<ContactPairPhysicalParamsTable>(model).id == 232 &&
             cardAt<ContactPairTable>(model, 0).physicalParamsId == 232));
    REQUIRE(unresolvedIn(model) == 0);
}

TEST_CASE("the BCONTACT command names a BCTABL1, a BCTABLE or a BCONECT and renumbers with it",
          "[model][cards][contact][control]")
{
    const std::string cards = solidBlock() +
                              "ContactElemSurface;10;1\n"
                              "ContactBody1;1;;ThreeDimensional;Deformable;10\n"
                              "ContactBody1;2;;ThreeDimensional;Deformable;10\n"
                              "ContactPair;5;;;1;2\n"
                              "ContactTable1;7;5\n"
                              "ContactTable;8;1;2\n";
    Model model = readModelText(
        "[executive]\n"
        "SolutionSequence;101\n"
        "[case]\n"
        "SubcaseStart;1\n"
        "ContactSelection;7\n"
        "SubcaseStart;2\n"
        "ContactSelection;8\n"
        "SubcaseStart;3\n"
        "ContactSelection;5\n"
        "SubcaseStart;4\n"
        "ContactSelection;All\n"
        "[model]\n" +
        cards);
    REQUIRE(unresolvedIn(model) == 0);
    REQUIRE(resolveReference(model, ReferenceTarget::ContactSelection, 7).has_value());
    REQUIRE(resolveReference(model, ReferenceTarget::ContactSelection, 8).has_value());
    REQUIRE(resolveReference(model, ReferenceTarget::ContactSelection, 5).has_value());
    REQUIRE_FALSE(resolveReference(model, ReferenceTarget::ContactSelection, 6).has_value());
    Model missing = readModelText("[executive]\nSolutionSequence;101\n[case]\nContactSelection;99\n[model]\n" + cards);
    REQUIRE(unresolvedIn(missing) == 1);
    Model keyword =
        readModelText("[executive]\nSolutionSequence;101\n[case]\nContactSelection;AutoContact\n[model]\n" + cards);
    REQUIRE(unresolvedIn(keyword) == 0); // a keyword names no card

    // The tables and the pairs are two spaces: the command follows the one that has the ID.
    Renumbering::apply(model, RenumberSpace::ContactTable, Renumbering::offset(20));
    REQUIRE((cardAt<ContactTable1Table>(model).id == 27 && cardAt<ContactTableTable>(model).id == 28));

    oor::testing::requireModelFileRoundTrip(model);
    Renumbering::apply(model, RenumberSpace::ContactPair, Renumbering::offset(30));

    REQUIRE(cardAt<ContactTable1Table>(model).pairs == std::vector<ContactRun>{{35, 35, 1}});
    REQUIRE(unresolvedIn(model) == 0);

    // An ID that a table and a pair both have is ambiguous: the command is left alone, as SDAMPING is.
    Model both =
        readModelText("[executive]\nSolutionSequence;101\n[case]\nContactSelection;5\n[model]\n" + solidBlock() +
                      "ContactElemSurface;10;1\nContactBody1;1;;ThreeDimensional;Deformable;10\nContactBody1;2;;"
                      "ThreeDimensional;Deformable;"
                      "10\nContactPair;5;;;1;2\nContactTable;5;1;2\n");
    Renumbering::apply(both, RenumberSpace::ContactTable, Renumbering::offset(100));
    REQUIRE(globalCommand(both, "ContactSelection") == 5);
    REQUIRE(cardAt<ContactTableTable>(both).id == 105);
}

// ------------------------------------------------------------------------------ parameter lists and beam contact

TEST_CASE("the parameter cards keep their pairs in order and read each value as the kind the format reference says",
          "[model][cards][contact]")
{
    const Model model = readBulk(
        "ContactParams;0;ContactDistanceTolerance;0.01;ContactToleranceBias;0.5;SeparationForce;1.E-5;ContactMethod;"
        "SegmentToSegment;AugmentationOption;1;NormalPenaltyFactor;1.E+4\n"
        "ContactParams;1;ContactToleranceBias;0.\n"
        "ContactPairGeomParams;310;ContactSearchOrder;1;ContactDistanceTolerance;0.01;GlueOption;1;"
        "GlueSeparationOption;1;TouchingBodyOptions;0\n"
        "ContactPairGeomParams;311;ContactMethod;Full;AdaptiveContactSurface;No;ContactThickness;0.1;AutomaticContact;"
        "Yes;ContactSide;Both\n"
        "ContactPairPhysicalParams;320;FrictionCoefficient;0.2;SeparationForce;0.001\n"
        "ContactPairPhysicalParams;321;KineticFrictionCoefficient;1.E+5;FrictionDecayCoefficient;1.;ContactStartTime;0."
        ";ContactDamping;On\n"
        "ContactBodyParams;300;FrictionCoefficient;0.1;InterfaceSubtype;2;DisplacementOption;5;SeparationAngle;45.;"
        "ContactHeatTransferCoefficient;1.5\n"
        "ContactAutoParams;0;ContactKind;CONTACT;DetectionDistance;0.05;FrictionKind;6;FrictionCoefficient;0.3\n"
        "ContactAutoParams;1;FrictionKind;BilinearCoulomb;ElemSet;5");
    const ContactParamsCard parameters = cardAt<ContactParamsTable>(model, 0);
    REQUIRE(parameters.params.size() == 6);
    REQUIRE((parameters.params[0].name == "ContactDistanceTolerance" && parameters.params[3].name == "ContactMethod"));
    REQUIRE((realOf(parameters.params, "ContactDistanceTolerance") == 0.01 &&
             realOf(parameters.params, "SeparationForce") == 1.E-5 &&
             wordOf(parameters.params, "ContactMethod") == "SegmentToSegment" &&
             integerOf(parameters.params, "AugmentationOption") == 1 &&
             realOf(parameters.params, "NormalPenaltyFactor") == 1.E4));
    REQUIRE(cardAt<ContactParamsTable>(model, 1).id == 1);
    REQUIRE(cardAt<ContactParamsTable>(readBulk("ContactParams;;ContactToleranceBias;0.")).id ==
            0); // a blank ID is 0, all the subcases
    // BIAS is a real: an integer is read as the real of the same value.
    const Model integerBias = readBulk("ContactParams;0;ContactToleranceBias;2");
    REQUIRE(realOf(cardAt<ContactParamsTable>(integerBias).params, "ContactToleranceBias") == 2.0);
    const ContactPairGeomParamsCard geometric = cardAt<ContactPairGeomParamsTable>(model, 0);
    REQUIRE((geometric.id == 310 && integerOf(geometric.params, "ContactSearchOrder") == 1 &&
             realOf(geometric.params, "ContactDistanceTolerance") == 0.01 &&
             integerOf(geometric.params, "GlueSeparationOption") == 1 &&
             integerOf(geometric.params, "TouchingBodyOptions") == 0 && geometric.params.size() == 5));
    const ContactPairGeomParamsCard explosive = cardAt<ContactPairGeomParamsTable>(model, 1);
    REQUIRE((wordOf(explosive.params, "ContactMethod") == "Full" &&
             wordOf(explosive.params, "AdaptiveContactSurface") == "No" &&
             realOf(explosive.params, "ContactThickness") == 0.1 && wordOf(explosive.params, "ContactSide") == "Both"));
    const ContactPairPhysicalParamsCard physical = cardAt<ContactPairPhysicalParamsTable>(model, 0);
    REQUIRE((physical.id == 320 && realOf(physical.params, "FrictionCoefficient") == 0.2 &&
             realOf(physical.params, "SeparationForce") == 0.001));
    const ContactPairPhysicalParamsCard physical700 = cardAt<ContactPairPhysicalParamsTable>(model, 1);
    REQUIRE((realOf(physical700.params, "KineticFrictionCoefficient") == 1.E5 &&
             wordOf(physical700.params, "ContactDamping") == "On"));
    const ContactBodyParamsCard body = cardAt<ContactBodyParamsTable>(model);
    REQUIRE((body.id == 300 && integerOf(body.params, "InterfaceSubtype") == 2 &&
             integerOf(body.params, "DisplacementOption") == 5 && realOf(body.params, "SeparationAngle") == 45.0 &&
             realOf(body.params, "ContactHeatTransferCoefficient") == 1.5));
    const ContactAutoParamsCard automatic = cardAt<ContactAutoParamsTable>(model, 0);
    REQUIRE((
        wordOf(automatic.params, "ContactKind") == "CONTACT" && realOf(automatic.params, "DetectionDistance") == 0.05 &&
        integerOf(automatic.params, "FrictionKind") == 6 && realOf(automatic.params, "FrictionCoefficient") == 0.3));
    // FTYPE is a word (BLCOUL) or a number.
    REQUIRE((wordOf(cardAt<ContactAutoParamsTable>(model, 1).params, "FrictionKind") == "BilinearCoulomb"));
    requireRoundTrip(model);
}

TEST_CASE("an ESET, SEEDESET or PROPSET of a BCAUTOP is a set of the Bulk Data and renumbers with the sets",
          "[model][cards][contact]")
{
    Model model = readBulk(
        "IdListSet1;5;1;2;3\nIdListSet1;6;1;2\nContactAutoParams;0;ElemSet;5;PropertySet;6;DetectionDistance;0.1");
    REQUIRE(unresolvedIn(model) == 0);
    Model lacking = readBulk("ContactAutoParams;0;ElemSet;5;DetectionDistance;0.1");
    REQUIRE(unresolvedIn(lacking) == 1);
    Renumbering::apply(model, RenumberSpace::Set, Renumbering::offset(100));
    const ContactAutoParamsCard card = cardAt<ContactAutoParamsTable>(model);
    REQUIRE((integerOf(card.params, "ElemSet") == 105 && integerOf(card.params, "PropertySet") == 106));
    REQUIRE(realOf(card.params, "DetectionDistance") == 0.1);
}

TEST_CASE("BCSCAP, BCPFLG and BCBMRAD have their layouts and refer to elements, properties and bodies",
          "[model][cards][contact]")
{
    Model model = readBulk(solidBlock() +
                           "ContactBeamCap;1;1;16\n"
                           "ContactBeamCap;2\n"
                           "ContactBeamSectionFlags;1;0;1;1;2\n"
                           "ContactBeamSectionFlags;2;0;1\n"
                           "ContactBeamRadius;0.5;ElemId;1;2;3\n"
                           "ContactBeamRadius;0.6;Body;1;Through;5;By;2\n"
                           "ContactBeamRadius;0.4;All\n"
                           "ContactBeamRadius;0.7;;9\n"
                           "ContactElemSurface;5;1\n"
                           "ContactBody1;1;;ThreeDimensional;Deformable;5\n");
    const ContactBeamCapCard cap = cardAt<ContactBeamCapTable>(model, 0);
    REQUIRE((cap.id == 1 && cap.capKind == 1 && cap.segmentCount == 16));
    REQUIRE((cardAt<ContactBeamCapTable>(model, 1).capKind == 0 &&
             cardAt<ContactBeamCapTable>(model, 1).segmentCount == 32));
    const ContactBeamSectionFlagsCard flags = cardAt<ContactBeamSectionFlagsTable>(model, 0);
    REQUIRE(flags.id == 1);
    REQUIRE((flags.segments.size() == 2 && flags.segments[1] == std::array<std::int32_t, 2>{1, 2}));
    REQUIRE(cardAt<ContactBeamRadiusTable>(model, 0).radius == 0.5);
    REQUIRE(cardAt<ContactBeamRadiusTable>(model, 0).idKind == "ElemId");
    REQUIRE(cardAt<ContactBeamRadiusTable>(model, 1).idKind == "Body");
    REQUIRE(cardAt<ContactBeamRadiusTable>(model, 1).ids == std::vector<ContactRun>{{1, 5, 2}});
    REQUIRE((cardAt<ContactBeamRadiusTable>(model, 2).idKind == "All" &&
             cardAt<ContactBeamRadiusTable>(model, 2).ids.empty()));
    REQUIRE(cardAt<ContactBeamRadiusTable>(model, 3).idKind == "ElemId");
    RecordingVisitor radius;
    model.table<ContactBeamRadiusTable>().visitReferences(1, radius);
    REQUIRE(radius.targets == std::vector<ReferenceTarget>{ReferenceTarget::ContactBody, ReferenceTarget::ContactBody});
    RecordingVisitor elements;
    model.table<ContactBeamRadiusTable>().visitReferences(0, elements);
    REQUIRE(elements.targets.size() == 3);
    REQUIRE(elements.targets[0] == ReferenceTarget::Element);
    Renumbering::apply(model, RenumberSpace::Element, Renumbering::offset(100));
    REQUIRE(cardAt<ContactBeamCapTable>(model, 0).id == 101);
    REQUIRE(cardAt<ContactBeamRadiusTable>(model, 0).ids ==
            std::vector<ContactRun>{{101, 101, 1}, {102, 102, 1}, {103, 103, 1}});
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(10));
    REQUIRE(cardAt<ContactBeamSectionFlagsTable>(model, 0).id == 11);
    requireRoundTrip(model);
}

TEST_CASE("an invalid parameter list or beam contact option is reported by field", "[model][cards][contact]")
{
    expectInvalid("ContactParams;0;NOSUCH;1", "");
    expectInvalid("ContactParams;0;ContactDistanceTolerance;ABC", "");
    expectInvalid("ContactParams;0;AugmentationOption;1.5", "");
    expectInvalid("ContactParams;-1;ContactDistanceTolerance;1.", "");
    expectInvalid("ContactPairGeomParams;1;NOSUCH;1", "");
    expectInvalid("ContactPairGeomParams;0;ContactDistanceTolerance;1.", "");
    expectInvalid("ContactPairPhysicalParams;1;FrictionCoefficient;ABC", "");
    expectInvalid("ContactBodyParams;1;InterfaceSubtype;1.5", "");
    expectInvalid("ContactBeamCap;1;6", "capKind");
    expectInvalid("ContactBeamCap;1;1;2", "segmentCount");
    expectInvalid("ContactBeamCap;0", "id");
    expectInvalid("ContactBeamSectionFlags;1", "");
    expectInvalid("ContactBeamSectionFlags;1;0;5", "");
    expectInvalid("ContactBeamRadius;-1.", "radius");
    expectInvalid("ContactBeamRadius;0.5;XXX;1", "idKind");
    expectInvalid("ContactBeamRadius;0.5;ElemId", "");
    expectInvalid("ContactBeamRadius;0.5;All;3", "");
}

// ------------------------------------------------------------------------------------ bolts and the user service

TEST_CASE("BOLT, BOLT1 and BCONUDS have their layouts and refer to grid points, elements and bodies",
          "[model][cards][contact]")
{
    Model model = readBulk(
        solidBlock() +
        "SolidHexa;2;1;1;2;3;4;5;6;7;8\n"
        "BoltConnection;60;9;Top;3;1;2;3;Bottom;3;4;5;6\n"
        "BoltPretension;61;9;0;;;;0.5;List;Elems;2;1;2;Nodes;2;5;6\n"
        "BoltPretension;62;9;1;0.;0.;1.;;Set;Elems;1;5;Nodes;1;6\n"
        "TypedListSet3;5;Elements;1\n"
        "TypedListSet3;6;Node;1;2\n"
        "ContactElemSurface;5;1\n"
        "ContactBody1;7;;ThreeDimensional;Deformable;5\n"
        "ContactUserService;7;ContactBody1;BODYUDS;MOTION;IntegerData;3;1;2;3;RealData;2;0.5;1.5;TextData;2;AAA;BBB\n"
        "Node;9;;5.;5.;5.");
    const BoltConnectionCard bolt = cardAt<BoltConnectionTable>(model);
    REQUIRE((bolt.id == 60 && bolt.controlNode == 9 && bolt.topNodes == std::vector<base::EntityId>{1, 2, 3} &&
             bolt.bottomNodes == std::vector<base::EntityId>{4, 5, 6}));
    const BoltPretensionCard elements = cardAt<BoltPretensionTable>(model, 0);
    REQUIRE((elements.id == 61 && elements.controlNode == 9 && elements.form == 0 && elements.offset == 0.5 &&
             elements.idType == "List"));
    REQUIRE((elements.elements == std::vector<base::EntityId>{1, 2} &&
             elements.nodes == std::vector<base::EntityId>{5, 6}));
    const BoltPretensionCard axis = cardAt<BoltPretensionTable>(model, 1);
    REQUIRE((axis.form == 1 && axis.axis == std::array<double, 3>{0., 0., 1.} && axis.idType == "Set"));
    const ContactUserServiceCard service = cardAt<ContactUserServiceTable>(model);
    REQUIRE((service.bodyId == 7 && service.bodyKind == "ContactBody1" && service.groupName == "BODYUDS" &&
             service.userRoutineName == "MOTION"));
    REQUIRE(service.integers == std::vector<std::int32_t>{1, 2, 3});
    REQUIRE(service.reals == std::vector<double>{0.5, 1.5});
    REQUIRE(service.characters == std::vector<std::string>{"AAA", "BBB"});
    REQUIRE(unresolvedIn(model) == 0);
    // With IDTYPE SET the lists name SET3 cards.
    RecordingVisitor sets;
    model.table<BoltPretensionTable>().visitReferences(1, sets);
    REQUIRE(sets.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::Node, ReferenceTarget::Set, ReferenceTarget::Set});
    Renumbering::apply(model, RenumberSpace::Set, Renumbering::offset(100));
    REQUIRE((cardAt<BoltPretensionTable>(model, 1).elements == std::vector<base::EntityId>{105}));
    REQUIRE((cardAt<BoltPretensionTable>(model, 1).nodes == std::vector<base::EntityId>{106}));
    REQUIRE(cardAt<BoltPretensionTable>(model, 0).elements == std::vector<base::EntityId>{1, 2});
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(10));
    REQUIRE(cardAt<BoltConnectionTable>(model).controlNode == 19);
    REQUIRE(cardAt<BoltConnectionTable>(model).topNodes == std::vector<base::EntityId>{11, 12, 13});
    requireRoundTrip(model);
}

TEST_CASE("an invalid bolt or user service is reported by field", "[model][cards][contact]")
{
    expectInvalid("BoltConnection;1;9;Top;2;1;2;Bottom;1;3", "");
    expectInvalid("BoltConnection;1;9;Top;2;1;2", "");
    expectInvalid("BoltConnection;1;9;Top;2;1;2;Bottom;2;3;9", "");
    expectInvalid("BoltConnection;0;9;Top;1;1;Bottom;1;2", "");
    expectInvalid("BoltPretension;1;9;2;;;;;;Elems;1;1", "form");
    expectInvalid("BoltPretension;1;9;0;;;;;BOTH;Elems;1;1", "idType");
    expectInvalid("BoltPretension;1;9;0;;;;;;Nodes;1;1", "elements");
    expectInvalid("BoltPretension;1;9;0;;;;;Set;Elems;2;1;2", "");
    expectInvalid("ContactUserService;1;ContactFaceRegion;GRP;SUB", "bodyKind");
    expectInvalid("ContactUserService;1;ContactBody;;SUB", "");
    expectInvalid("ContactUserService;1;ContactBody;GRP;;IntegerData;1;1.5", "");
    expectInvalid("ContactUserService;1;ContactBody;GRP;SUB;IntegerData;1;1;IntegerData;1;2", "");
}

// ------------------------------------------------------------------------------------------------ non-linear cards

TEST_CASE("NLSTEP has its keyword lines with the named fields in the places of the format reference",
          "[model][cards][nonlinear]")
{
    const Model model = readBulk(
        "NonlinearStepControl;10;3.7;;General;;;6;;FixedStepping;24;;MechanicalAnalysis;PV;;0.02\n"
        "NonlinearStepControl;20;3.7;;General;;;5;;AdaptiveStepping;0.02;1.E-5;;5;;20\n"
        "NonlinearStepControl;11;3.7;SeverelyNonlinear;AdaptiveStepping\n"
        "NonlinearStepControl;12;;LoadCaseAccumulation\n"
        "NonlinearStepControl;13;;LoadCasePerformance;LoadCaseContinue;4;P;;1.E-4;;;;25;2\n"
        "NonlinearStepControl;30;2.;;AdaptiveStepping;0.01;1.E-5;0.5;4;1.2;0;99999;4;2.E-4;5;2;0;0.1;10.;1;5;0.6;0.5;"
        "HeatAnalysis;UPW;0."
        "01;0.01;0.01;AUTO;1;10;4;0.5;CoupledAnalysis;0.9;0.5\n"
        "NonlinearStepControl;31;;;ArcLengthMethod;Riks;0.01;0.25;4.;4;1000;MechanicalAnalysis;PV;0.1;0.1;0.1;ITER;5;2;"
        "25;4;0.5;0.2\n"
        "NonlinearStepControl;32;;;General;15;2;7;1");
    const NonlinearStepControlCard fixed = cardAt<NonlinearStepControlTable>(model, 0);
    REQUIRE((fixed.id == 10 && fixed.totalTime == 3.7 && fixed.controlSetting.empty() && fixed.sections.size() == 3));
    REQUIRE((fixed.sections[0].keyword == "General" && integerOf(fixed.sections[0].params, "MaxBisections") == 6));
    REQUIRE(fixed.sections[0].params.size() == 1);
    REQUIRE(integerOf(sectionOf(fixed.sections, "FixedStepping"), "IncrementCount") == 24);
    REQUIRE((wordOf(sectionOf(fixed.sections, "MechanicalAnalysis"), "ConvergenceCriteria") == "PV" &&
             realOf(sectionOf(fixed.sections, "MechanicalAnalysis"), "LoadTolerance") == 0.02));
    const NonlinearStepControlCard automatic = cardAt<NonlinearStepControlTable>(model, 1);
    const auto adapt = sectionOf(automatic.sections, "AdaptiveStepping");
    REQUIRE((realOf(adapt, "InitialTimeStepFactor") == 0.02 && realOf(adapt, "MinTimeStepFactor") == 1.E-5 &&
             integerOf(adapt, "DesiredIterations") == 5 && integerOf(adapt, "IntermediateOutput") == 20 &&
             adapt.size() == 4));
    REQUIRE((cardAt<NonlinearStepControlTable>(model, 2).controlSetting == "SeverelyNonlinear" &&
             cardAt<NonlinearStepControlTable>(model, 2).sections.size() == 1));
    REQUIRE(cardAt<NonlinearStepControlTable>(model, 2).sections[0].params.empty());
    REQUIRE((cardAt<NonlinearStepControlTable>(model, 3).controlSetting == "LoadCaseAccumulation" &&
             cardAt<NonlinearStepControlTable>(model, 3).totalTime == 1.0));
    const auto contact = sectionOf(cardAt<NonlinearStepControlTable>(model, 4).sections, "LoadCaseContinue");
    REQUIRE((integerOf(contact, "IncrementCountCoupled") == 4 && wordOf(contact, "ConvergenceCoupled") == "P" &&
             realOf(contact, "LoadToleranceCoupled") == 1.E-4 && integerOf(contact, "MaxIterationsCoupled") == 25 &&
             integerOf(contact, "MinIterationsCoupled") == 2));
    const NonlinearStepControlCard everything = cardAt<NonlinearStepControlTable>(model, 5);
    const auto every = sectionOf(everything.sections, "AdaptiveStepping");
    REQUIRE(every.size() == 18);
    REQUIRE((realOf(every, "TimeStepScaleFactor") == 1.2 && integerOf(every, "MaxSubsteps") == 99999 &&
             integerOf(every, "DampingKind") == 4 && realOf(every, "DampingFactor") == 2.E-4 &&
             integerOf(every, "CriteriaTableId") == 5 && integerOf(every, "PhysicsOption") == 2 &&
             realOf(every, "MaxStepRatio") == 10.0 && integerOf(every, "TimeStepAdjustInterval") == 1 &&
             integerOf(every, "StepMultiplier") == 5 && realOf(every, "TimeStepHoldBounds") == 0.6 &&
             realOf(every, "DisplacementConvergence") == 0.5));
    const auto heat = sectionOf(everything.sections, "HeatAnalysis");
    REQUIRE((wordOf(heat, "ConvergenceHeat") == "UPW" && wordOf(heat, "StiffnessMethodHeat") == "AUTO" &&
             integerOf(heat, "StiffnessStepHeat") == 1 && integerOf(heat, "MaxQuasiNewtonVectorsHeat") == 10 &&
             integerOf(heat, "MaxLineSearchesHeat") == 4 && realOf(heat, "LineSearchToleranceHeat") == 0.5));
    const auto coupled = sectionOf(everything.sections, "CoupledAnalysis");
    REQUIRE((realOf(coupled, "HeatFromPlasticWork") == 0.9 && realOf(coupled, "HeatFromFriction") == 0.5));
    const NonlinearStepControlCard arc = cardAt<NonlinearStepControlTable>(model, 6);
    const auto length = sectionOf(arc.sections, "ArcLengthMethod");
    REQUIRE((wordOf(length, "SteppingMethod") == "Riks" && realOf(length, "MaxAdaptiveRatio") == 4.0 &&
             integerOf(length, "DesiredIterationsAdapted") == 4 && integerOf(length, "MaxSubstepsAdapted") == 1000));
    const auto mechanical = sectionOf(arc.sections, "MechanicalAnalysis");
    REQUIRE((realOf(mechanical, "DisplacementTolerance") == 0.1 && wordOf(mechanical, "StiffnessMethod") == "ITER" &&
             integerOf(mechanical, "StiffnessUpdateStep") == 5 && integerOf(mechanical, "MassRatioConvergence") == 2 &&
             integerOf(mechanical, "MaxQuasiNewtonVectors") == 25 && integerOf(mechanical, "MaxLineSearches") == 4 &&
             realOf(mechanical, "LineSearchTolerance") == 0.5 && realOf(mechanical, "FrictionStress") == 0.2));
    const auto general = sectionOf(cardAt<NonlinearStepControlTable>(model, 7).sections, "General");
    REQUIRE((integerOf(general, "MaxIterations") == 15 && integerOf(general, "MinIterations") == 2 &&
             integerOf(general, "MaxBisections") == 7 && integerOf(general, "CreepOption") == 1));
    requireRoundTrip(model);
}

TEST_CASE("NLADAPT has its STEP and CREEP lines in any order", "[model][cards][nonlinear]")
{
    Model model = readBulk(
        "NonlinearStaticParams;700\n"
        "NonlinearAdaptiveStepping;700;Stepping;;;;;;;;4;;Creep;1\n"
        "NonlinearAdaptiveStepping;800;Creep;1;;;;Stepping;;;;;;;;4\n"
        "NonlinearAdaptiveStepping;801;Stepping;0.1;1.5;0.001;0.5;99999;10;1.2;4;2.E-4;Creep;0;0.5;0.1;0.05");
    const NonlinearAdaptiveSteppingCard first = cardAt<NonlinearAdaptiveSteppingTable>(model, 0);
    REQUIRE((first.id == 700 && first.sections.size() == 2 && first.sections[0].keyword == "Stepping"));
    REQUIRE(integerOf(sectionOf(first.sections, "Stepping"), "DampingKind") == 4);
    REQUIRE(integerOf(sectionOf(first.sections, "Creep"), "CreepStepCheck") == 1);
    const NonlinearAdaptiveSteppingCard second = cardAt<NonlinearAdaptiveSteppingTable>(model, 1);
    REQUIRE((second.sections[0].keyword == "Creep" &&
             integerOf(sectionOf(second.sections, "Stepping"), "DampingKind") == 4));
    const NonlinearAdaptiveSteppingCard full = cardAt<NonlinearAdaptiveSteppingTable>(model, 2);
    const auto step = sectionOf(full.sections, "Stepping");
    REQUIRE((realOf(step, "MinStepRatio") == 0.1 && realOf(step, "MaxStepRatio") == 1.5 &&
             realOf(step, "MinTimeStep") == 0.001 && realOf(step, "MaxTimeStep") == 0.5 &&
             integerOf(step, "MaxSubsteps") == 99999 && integerOf(step, "RecycleCount") == 10 &&
             realOf(step, "TimeStepScaleFactor") == 1.2 && realOf(step, "DampingFactor") == 2.E-4));
    const auto creep = sectionOf(full.sections, "Creep");
    REQUIRE((integerOf(creep, "CreepStepCheck") == 0 && realOf(creep, "TensionCutoffStrain") == 0.5 &&
             realOf(creep, "TensionCutoffStress") == 0.1 && realOf(creep, "CutoffTime") == 0.05));
    // The ID is that of the NLPARM it extends: a renumbering of the NLPARM moves both.
    Renumbering::apply(model, RenumberSpace::Nonlinear, Renumbering::offset(1000));
    REQUIRE((cardAt<NonlinearStaticParamsTable>(model).id == 1700 &&
             cardAt<NonlinearAdaptiveSteppingTable>(model, 0).id == 1700));
    requireRoundTrip(model);
}

TEST_CASE("NLHARM, NLFREQ, NLFREQ1 and NLFREQC have their layouts and NLHARM names the frequencies",
          "[model][cards][nonlinear]")
{
    Model model = readBulk(
        "NonlinearHarmonicParams;41;;3;15\n"
        "NonlinearHarmonicParams;42;3;5;16\n"
        "NonlinearHarmonicParams;43;;2;17\n"
        "NonlinearHarmonicFreqList;15;4.;9.;26.;13.\n"
        "NonlinearHarmonicFreqList;18;1.;2.;3.;4.;5.;6.;7.;8.;9.\n"
        "NonlinearHarmonicFreqSteps;16;5.;1.5;8\n"
        "NonlinearHarmonicFreqSteps;19;10.;-0.5\n"
        "NonlinearHarmonicFreqContinuation;17;15.;0.02;0.;0.;8000;0.;0.;1.E-5;Yes;DefaultOutput;0.35;4.;22.5;31.\n"
        "NonlinearHarmonicFreqContinuation;89;;0.5");
    REQUIRE(unresolvedIn(model) == 0);
    const NonlinearHarmonicParamsCard harmonic = cardAt<NonlinearHarmonicParamsTable>(model, 0);
    REQUIRE((harmonic.id == 41 && harmonic.subharmonicFactor == 1 && harmonic.harmonicCount == 3 &&
             harmonic.freqListId == 15));
    REQUIRE((cardAt<NonlinearHarmonicParamsTable>(model, 1).subharmonicFactor == 3 &&
             cardAt<NonlinearHarmonicParamsTable>(model, 1).freqListId == 16));
    REQUIRE(cardAt<NonlinearHarmonicFreqListTable>(model, 0).frequencies ==
            std::vector<double>{4., 9., 26., 13.}); // in the order given
    REQUIRE(cardAt<NonlinearHarmonicFreqListTable>(model, 1).frequencies.size() == 9);
    const NonlinearHarmonicFreqStepsCard steps = cardAt<NonlinearHarmonicFreqStepsTable>(model, 0);
    REQUIRE((steps.id == 16 && steps.startFreq == 5.0 && steps.freqStep == 1.5 && steps.stepCount == 8));
    REQUIRE((cardAt<NonlinearHarmonicFreqStepsTable>(model, 1).freqStep == -0.5 &&
             cardAt<NonlinearHarmonicFreqStepsTable>(model, 1).stepCount == 1));
    const NonlinearHarmonicFreqContinuationCard continuation = cardAt<NonlinearHarmonicFreqContinuationTable>(model, 0);
    REQUIRE((continuation.id == 17 && continuation.initialFreq == 15.0 && continuation.initialStep == 0.02));
    REQUIRE((continuation.minimumStepSize == 0.0 && continuation.maximumStepSize == 0.0 &&
             continuation.maximumSteps == 8000 && continuation.minimumFreq == 0.0 && continuation.maximumFreq == 0.0 &&
             continuation.derivativeStep == 1.E-5 && continuation.printOnFailure == "Yes"));
    REQUIRE((continuation.outputRequest && continuation.outputFreqStep == 0.35 &&
             continuation.outputFreqTolerance == 4.0 && continuation.outputStartFreq == 22.5 &&
             continuation.outputEndFreq == 31.0));
    // The defaults that depend on STEPINIT, MAXSTEP and FREQINI stay blank in the card.
    const NonlinearHarmonicFreqContinuationCard minimal = cardAt<NonlinearHarmonicFreqContinuationTable>(model, 1);
    REQUIRE((minimal.initialStep == 0.5 && !minimal.minimumStepSize && !minimal.maximumStepSize &&
             !minimal.maximumFreq && minimal.maximumSteps == 3 && minimal.derivativeStep == 1.E-3 &&
             minimal.printOnFailure == "No" && !minimal.outputRequest && !minimal.outputStartFreq));
    // The three kinds of frequency list are one set of IDs (the NLFREQ field of an NLHARM names any); NLHARM 43 is
    // dangling.
    RecordingVisitor visitor;
    model.table<NonlinearHarmonicParamsTable>().visitReferences(0, visitor);
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::NonlinearFrequency});
    Model lacking = readBulk("NonlinearHarmonicParams;43;;2;99");
    REQUIRE(unresolvedIn(lacking) == 1);
    Renumbering::apply(model, RenumberSpace::NonlinearFrequency, Renumbering::offset(100));
    REQUIRE((cardAt<NonlinearHarmonicFreqListTable>(model, 0).id == 115 &&
             cardAt<NonlinearHarmonicFreqStepsTable>(model, 0).id == 116 &&
             cardAt<NonlinearHarmonicFreqContinuationTable>(model, 0).id == 117));
    REQUIRE((cardAt<NonlinearHarmonicParamsTable>(model, 0).freqListId == 115 &&
             cardAt<NonlinearHarmonicParamsTable>(model, 1).freqListId == 116 &&
             cardAt<NonlinearHarmonicParamsTable>(model, 2).freqListId == 117));
    Renumbering::apply(model, RenumberSpace::NonlinearHarmonic, Renumbering::offset(1000));
    REQUIRE(cardAt<NonlinearHarmonicParamsTable>(model, 0).id == 1041);
    requireRoundTrip(model);
}

TEST_CASE("NLSTEP and NLHARM are selected by their Case Control commands and renumber with them",
          "[model][cards][nonlinear][control]")
{
    Model model = readModelText(
        "[executive]\n"
        "SolutionSequence;400\n"
        "[case]\n"
        "SubcaseStart;1\n"
        "NonlinearStepSelection;10\n"
        "NonlinearHarmonicSelection;41\n"
        "[model]\n"
        "NonlinearStepControl;10;3.7;;FixedStepping;24\n"
        "NonlinearHarmonicParams;41;;3;15\n"
        "NonlinearHarmonicFreqList;15;3.\n");
    REQUIRE(unresolvedIn(model) == 0);
    Model missing = readModelText(
        "[executive]\nSolutionSequence;400\n[case]\nNonlinearStepSelection;99\nNonlinearHarmonicSelection;98\n[model]"
        "\nNode;1\n");
    REQUIRE(unresolvedIn(missing) == 2);
    Renumbering::apply(model, RenumberSpace::NonlinearStep, Renumbering::offset(100));
    Renumbering::apply(model, RenumberSpace::NonlinearHarmonic, Renumbering::offset(200));

    oor::testing::requireModelFileRoundTrip(model);
    REQUIRE(
        (cardAt<NonlinearStepControlTable>(model).id == 110 && cardAt<NonlinearHarmonicParamsTable>(model).id == 241));
    REQUIRE(unresolvedIn(model) == 0);
}

TEST_CASE("NLOUT and NLOUTUD have their layouts; NLOUTUD names elements or grid points", "[model][cards][nonlinear]")
{
    Model model = readBulk(solidBlock() +
                           "NonlinearOutputItems;1;TotalTemperature\n"
                           "NonlinearOutputItems;2;TotalTemperature;;;;;;;ElemStateVariables;SV2;SV3\n"
                           "NonlinearOutputItems;3;;;;;;;;FailureIndex1;FailureIndex2;FailureIndex3;FailureIndex4\n"
                           "NonlinearOutputItems;4\n"
                           "NonlinearOutputItems;5;CauchyStress;CauchyTotalStrain;CauchyElasticStrain;"
                           "CauchyPlasticStrain;CauchyCreepStrain;CauchyThermalStrain;TSTRNPS;MajorEngineeringStrain\n"
                           "NonlinearUserOutput;4;OUTUDS;ElemExplicitOutput;0.01;;RESOUT1;1\n"
                           "NonlinearUserOutput;5;GRP2;NodeExplicitOutput;;10;GRIDOUT;5;6\n"
                           "NonlinearUserOutput;6;GRP3;;;;NAME;1");
    REQUIRE(cardAt<NonlinearOutputItemsTable>(model, 0).outputs == std::vector<std::string>{"TotalTemperature"});
    REQUIRE(cardAt<NonlinearOutputItemsTable>(model, 1).stateVariables == std::vector<std::string>{"SV2", "SV3"});
    REQUIRE(cardAt<NonlinearOutputItemsTable>(model, 1).outputs == std::vector<std::string>{"TotalTemperature"});
    REQUIRE(cardAt<NonlinearOutputItemsTable>(model, 2).outputs.size() == 4); // the words may start on the second line
    REQUIRE(cardAt<NonlinearOutputItemsTable>(model, 3).outputs.empty());
    REQUIRE(cardAt<NonlinearOutputItemsTable>(model, 4).outputs.size() == 8);
    REQUIRE(cardAt<NonlinearOutputItemsTable>(model, 4).outputs[0] == "CauchyStress");
    const NonlinearUserOutputCard elements = cardAt<NonlinearUserOutputTable>(model, 0);
    REQUIRE((elements.id == 4 && elements.groupName == "OUTUDS" && elements.outputKind == "ElemExplicitOutput" &&
             elements.timeInterval == 0.01 && !elements.stepInterval && elements.outputName == "RESOUT1" &&
             elements.entityIds == std::vector<base::EntityId>{1}));
    const NonlinearUserOutputCard grids = cardAt<NonlinearUserOutputTable>(model, 1);
    REQUIRE((grids.outputKind == "NodeExplicitOutput" && !grids.timeInterval && grids.stepInterval == 10 &&
             grids.entityIds == std::vector<base::EntityId>{5, 6}));
    REQUIRE(cardAt<NonlinearUserOutputTable>(model, 2).outputKind == "ElemExplicitOutput"); // the default
    REQUIRE(unresolvedIn(model) == 0);
    Model lacking = readBulk(solidBlock() +
                             "NonlinearUserOutput;4;G;ElemExplicitOutput;;1;NAME;1;99\nNonlinearUserOutput;5;G;"
                             "NodeExplicitOutput;;1;NAME;1;99");
    REQUIRE(unresolvedIn(lacking) == 2); // the element 99 and the grid point 99
    Renumbering::apply(model, RenumberSpace::Element, Renumbering::offset(100));
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(10));
    REQUIRE(cardAt<NonlinearUserOutputTable>(model, 0).entityIds == std::vector<base::EntityId>{101});
    REQUIRE(cardAt<NonlinearUserOutputTable>(model, 1).entityIds == std::vector<base::EntityId>{15, 16});
    requireRoundTrip(model);
}

TEST_CASE("NLRGAP and NLRSFD are nonlinear loads selected by NONLINEAR, with grid points and tables",
          "[model][cards][nonlinear]")
{
    Model model = readModelText(
        "[executive]\n"
        "SolutionSequence;129\n"
        "[case]\n"
        "NonlinearLoadSelection;26\n"
        "[model]\n"
        "Node;5\n"
        "Node;6\n"
        "Node;1001\n"
        "Node;1002\n"
        "TableDynamic1;7;;;0.;1.;1.;2.;EndTable\n"
        "TableDynamic1;12;;;0.;1.;1.;2.;EndTable\n"
        "TableDynamic1;8;;;0.;1.;1.;2.;EndTable\n"
        "NonlinearRadialGap;26;5;6;PlaneXy;7;12;8;2.4;Gamma;4.E+5\n"
        "NonlinearRadialGap;22;5;6;PlaneZx;-7\n"
        "NonlinearRadialGap;22;5;6\n"
        "NonlinearSqueezeFilmDamper;100;1001;1002;PlaneXy;1.5;2.5;0.04;Long;1.8;250.;1;110.;35.;130.;80.;61;0.02;0.;"
        "SFDGRP;;"
        "0.;;;;0.;0.;0.;0.\n"
        "NonlinearSqueezeFilmDamper;101;1001;1002;PlaneXy;1.;1.;1.;;1.;1;1;1.;1.;1.;1.;1;1.;1.;SFDSUB;EXT;1.E-4\n"
        "NonlinearSqueezeFilmDamper;102;1001;1002;;1.5;2.5;0.04;Short;1.8;250.;2;110.;35.;130.;80.\n"
        "NonlinearSqueezeFilmDamper;103;1001;1002;;;;;;;;;;;;;;;;SFDSUB\n",
        true);
    REQUIRE(unresolvedIn(model) == 0);
    const NonlinearRadialGapCard gap = cardAt<NonlinearRadialGapTable>(model, 0);
    REQUIRE((gap.setId == 26 && gap.nodeA == 5 && gap.nodeB == 6 && gap.plane == "PlaneXy" &&
             gap.stiffnessTableId == 7 && gap.clearanceTableId == 12 && gap.frictionTableId == 8 && gap.radius == 2.4 &&
             gap.smoothing && gap.smoothingValue == 4.0E5));
    REQUIRE((cardAt<NonlinearRadialGapTable>(model, 1).plane == "PlaneZx" &&
             cardAt<NonlinearRadialGapTable>(model, 1).stiffnessTableId == -7));
    const NonlinearRadialGapCard plain = cardAt<NonlinearRadialGapTable>(model, 2);
    REQUIRE((plain.plane == "PlaneXy" && plain.stiffnessTableId == 0 && plain.radius == 0.0 && !plain.smoothing &&
             plain.smoothingValue == 1.0E6));
    const NonlinearSqueezeFilmDamperCard ring = cardAt<NonlinearSqueezeFilmDamperTable>(model, 0);
    REQUIRE((ring.setId == 100 && ring.nodeA == 1001 && ring.nodeB == 1002 && ring.plane == "PlaneXy" &&
             ring.bearingDiameter == 1.5 && ring.bearingLength == 2.5 && ring.bearingClearance == 0.04 &&
             ring.bearingKind == "Long" && ring.viscosity == 1.8 && ring.vapourPressure == 250.0 &&
             ring.portCount == 1 && ring.pressure1 == 110.0 && ring.portAngle1 == 35.0 && ring.pressure2 == 130.0 &&
             ring.portAngle2 == 80.0 && ring.pointCount == 61 && ring.offset1 == 0.02 && ring.offset2 == 0.0 &&
             ring.serviceGroup == "SFDGRP"));
    const NonlinearSqueezeFilmDamperCard service = cardAt<NonlinearSqueezeFilmDamperTable>(model, 1);
    REQUIRE((service.bearingKind == "Long" && service.pointCount == 1 && service.offset1 == 1.0 &&
             service.serviceGroup == "SFDSUB" && service.serviceEvaluation == "EXT" &&
             service.serviceParameters[0] == 1.0E-4 && service.serviceParameters[1] == 0.0));
    const NonlinearSqueezeFilmDamperCard twoPorts = cardAt<NonlinearSqueezeFilmDamperTable>(model, 2);
    REQUIRE((twoPorts.plane == "PlaneXy" && twoPorts.bearingKind == "Short" && twoPorts.portCount == 2 &&
             twoPorts.pointCount == 101 && twoPorts.serviceGroup.empty()));
    // With an external service the characteristics may be blank.
    const NonlinearSqueezeFilmDamperCard blank = cardAt<NonlinearSqueezeFilmDamperTable>(model, 3);
    REQUIRE((blank.serviceGroup == "SFDSUB" && blank.bearingDiameter == 0.0 && blank.portCount == 0));
    // They are load sets of the NONLINEAR command, with the NOLIN cards.
    REQUIRE(cardsOfSet(model, CardCategory::DynamicLoad, 26).size() == 1);
    REQUIRE(cardsOfSet(model, CardCategory::DynamicLoad, 22).size() == 2);
    RecordingVisitor visitor;
    model.table<NonlinearRadialGapTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields ==
            std::vector<std::string>{"nodeA", "nodeB", "stiffnessTableId", "clearanceTableId", "frictionTableId"});
    REQUIRE(visitor.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::Node, ReferenceTarget::Node, ReferenceTarget::DynamicTable,
                                         ReferenceTarget::DynamicTable, ReferenceTarget::DynamicTable});
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(10));
    REQUIRE((cardAt<NonlinearRadialGapTable>(model, 0).nodeA == 15 &&
             cardAt<NonlinearRadialGapTable>(model, 0).nodeB == 16));
    REQUIRE((cardAt<NonlinearSqueezeFilmDamperTable>(model, 0).nodeA == 1011 &&
             cardAt<NonlinearSqueezeFilmDamperTable>(model, 0).nodeB == 1012));
    Renumbering::apply(model, RenumberSpace::DynamicTable, Renumbering::offset(100));
    REQUIRE((cardAt<NonlinearRadialGapTable>(model, 0).stiffnessTableId == 107 &&
             cardAt<NonlinearRadialGapTable>(model, 0).clearanceTableId == 112 &&
             cardAt<NonlinearRadialGapTable>(model, 0).frictionTableId == 108));
    REQUIRE(cardAt<NonlinearRadialGapTable>(model, 1).stiffnessTableId == -107); // a negative TABK keeps its sign
    Renumbering::apply(model, RenumberSpace::DynamicLoadSet, Renumbering::offset(1000));
    REQUIRE((cardAt<NonlinearRadialGapTable>(model, 0).setId == 1026 &&
             cardAt<NonlinearSqueezeFilmDamperTable>(model, 0).setId == 1100));
    requireRoundTrip(model);
}

TEST_CASE("an invalid non-linear card is reported by field", "[model][cards][nonlinear]")
{
    expectInvalid("NonlinearStepControl;0", "id");
    expectInvalid("NonlinearStepControl;1;;BAD", "controlSetting");
    expectInvalid("NonlinearStepControl;1;;;FixedStepping;30;;AdaptiveStepping;0.1", "sections");
    expectInvalid("NonlinearStepControl;1;;;FixedStepping;30;;LoadCaseContinue;3", "sections");
    expectInvalid("NonlinearStepControl;1;;;ArcLengthMethod;SOMETHING", "sections");
    expectInvalid("NonlinearStepControl;1;;;FixedStepping;30;;FixedStepping;40", "sections");
    expectInvalid("NonlinearStepControl;1;;;FixedStepping;1.5", "");
    expectInvalid("NonlinearAdaptiveStepping;0", "id");
    expectInvalid("NonlinearAdaptiveStepping;1;Stepping;;;;;;;;3", "sections");
    expectInvalid("NonlinearAdaptiveStepping;1;Creep;2", "sections");
    expectInvalid("NonlinearHarmonicParams;0;;3;15", "id");
    expectInvalid("NonlinearHarmonicParams;1;0;3;15", "subharmonicFactor");
    expectInvalid("NonlinearHarmonicParams;1;;0;15", "harmonicCount");
    expectInvalid("NonlinearHarmonicParams;1;;3", "freqListId");
    expectInvalid("NonlinearHarmonicFreqList;1", "frequencies");
    expectInvalid("NonlinearHarmonicFreqList;1;0.", "frequencies");
    expectInvalid("NonlinearHarmonicFreqList;1;-3.", "frequencies");
    expectInvalid("NonlinearHarmonicFreqSteps;1;0.;1.", "startFreq");
    expectInvalid("NonlinearHarmonicFreqSteps;1;3.;0.", "freqStep");
    expectInvalid("NonlinearHarmonicFreqSteps;1;3.;1.;0", "stepCount");
    expectInvalid("NonlinearHarmonicFreqSteps;1;3.", "freqStep");
    expectInvalid("NonlinearHarmonicFreqContinuation;1", "initialStep");
    expectInvalid("NonlinearHarmonicFreqContinuation;1;-1.;0.5", "initialFreq");
    expectInvalid("NonlinearHarmonicFreqContinuation;1;;0.", "initialStep");
    expectInvalid("NonlinearHarmonicFreqContinuation;1;;0.5;-1.", "minimumStepSize");
    expectInvalid("NonlinearHarmonicFreqContinuation;1;;0.5;;;-1", "maximumSteps");
    expectInvalid("NonlinearHarmonicFreqContinuation;1;;0.5;;;;;;;MAYBE", "printOnFailure");
    expectInvalid("NonlinearHarmonicFreqContinuation;1;;0.5;;;;;;;;SOMETHING", "outputRequest");
    expectInvalid("NonlinearHarmonicFreqContinuation;1;;0.5;;;;;;;;DefaultOutput", "outputFreqStep");
    expectInvalid("NonlinearHarmonicFreqContinuation;1;;0.5;;;;;;;;;0.1", "");
    expectInvalid("NonlinearOutputItems;0", "id");
    expectInvalid("NonlinearOutputItems;1;;;;;;;;ElemStateVariables", "stateVariables");
    expectInvalid("NonlinearOutputItems;1;A;ElemStateVariables;B;ElemStateVariables", "stateVariableList");
    expectInvalid("NonlinearOutputItems;1;12.5", "outputs");
    expectInvalid("NonlinearUserOutput;0;G;ElemExplicitOutput;;1;N;1", "id");
    expectInvalid("NonlinearUserOutput;1;;ElemExplicitOutput;;1;N;1", "groupName");
    expectInvalid("NonlinearUserOutput;1;G;SOMETHING;;1;N;1", "outputKind");
    expectInvalid("NonlinearUserOutput;1;G;ElemExplicitOutput;0.;;N;1", "timeInterval");
    expectInvalid("NonlinearUserOutput;1;G;ElemExplicitOutput;0.1;5;N;1", "stepInterval");
    expectInvalid("NonlinearUserOutput;1;G;ElemExplicitOutput;0.1;;;1", "outputName");
    expectInvalid("NonlinearUserOutput;1;G;ElemExplicitOutput;0.1;;N", "entityIds");
    expectInvalid("NonlinearRadialGap;0;3;4", "setId");
    expectInvalid("NonlinearRadialGap;1;0;4", "nodeA");
    expectInvalid("NonlinearRadialGap;1;3;4;XX", "plane");
    expectInvalid("NonlinearRadialGap;1;3;4;;;-1", "clearanceTableId");
    expectInvalid("NonlinearRadialGap;1;3;4;;;;;-1.", "radius");
    expectInvalid("NonlinearRadialGap;1;3;4;;;;;;SOMETHING", "smoothing");
    expectInvalid("NonlinearRadialGap;1;3;4;;;;;;;5.", "smoothingValue");
    expectInvalid("NonlinearRadialGap;1;3;4;;;;;;Gamma;-1.", "smoothingValue");
    expectInvalid("NonlinearSqueezeFilmDamper;1;3;4;PlaneXy;;2.5;0.04;;1.8;250.;1;110.;35.", "bearingDiameter");
    expectInvalid("NonlinearSqueezeFilmDamper;1;3;4;PlaneXy;1.5;2.5;0.04;;1.8;250.;3;110.;35.", "portCount");
    expectInvalid("NonlinearSqueezeFilmDamper;1;3;4;PlaneXy;1.5;2.5;0.04;;1.8;250.;1;;30.", "pressure1");
    expectInvalid("NonlinearSqueezeFilmDamper;1;3;4;PlaneXy;1.5;2.5;0.04;;1.8;250.;2;110.;35.;120.", "portAngle2");
    expectInvalid("NonlinearSqueezeFilmDamper;1;3;4;PlaneXy;1.5;2.5;0.04;;1.8;250.;1;100.;400.", "portAngle1");
    expectInvalid("NonlinearSqueezeFilmDamper;1;3;4;PlaneXy;1.5;2.5;0.04;MEDIUM;1.8;250.;1;110.;35.", "bearingKind");
    expectInvalid("NonlinearSqueezeFilmDamper;1;3;4;XX;1.5;2.5;0.04;;1.8;250.;1;110.;35.", "plane");
    expectInvalid("NonlinearSqueezeFilmDamper;1;3;4;PlaneXy;1.5;2.5;0.04;;1.8;250.;1;110.;35.;;;50", "pointCount");
}

TEST_CASE("a card of the family keeps its values through an edit that moves its ID", "[model][cards][contact]")
{
    // The ID of a card kept whole (CardRows) lives in the card and in its ID column: both are changed together.
    Model model = readBulk(
        "ContactTable;5;1;2\n"
        "NonlinearStepControl;7;1.;;FixedStepping;10\n"
        "NonlinearHarmonicFreqContinuation;9;;0.5\n"
        "NonlinearSqueezeFilmDamper;11;3;4;;1.5;2.5;0.04;;1.8;250.;1;110.;35.\n"
        "NonlinearOutputItems;13;TotalTemperature");
    ContactTableTable& tables = model.table<ContactTableTable>();
    tables.setIdAtRow(0, 55);
    REQUIRE((tables.idAtRow(0) == 55 && tables.card(0).id == 55));
    NonlinearStepControlTable& steps = model.table<NonlinearStepControlTable>();
    steps.setIdAtRow(0, 77);
    REQUIRE((steps.idAtRow(0) == 77 && steps.card(0).id == 77 && steps.card(0).sections.size() == 1));
    NonlinearHarmonicFreqContinuationTable& continuation = model.table<NonlinearHarmonicFreqContinuationTable>();
    continuation.setIdAtRow(0, 99);
    REQUIRE((continuation.card(0).id == 99 && continuation.card(0).initialStep == 0.5));
    NonlinearSqueezeFilmDamperTable& dampers = model.table<NonlinearSqueezeFilmDamperTable>();
    dampers.setIdAtRow(0, 111);
    REQUIRE(dampers.card(0).setId == 111);
    NonlinearOutputItemsTable& outputs = model.table<NonlinearOutputItemsTable>();
    outputs.setIdAtRow(0, 133);
    REQUIRE(outputs.card(0).id == 133);
    requireRoundTrip(model);
}

TEST_CASE("a BCTADD joins contact sets under an ID of its own and refuses a set that is itself",
          "[model][cards][contact]")
{
    Model model = readBulk("ContactSetUnion;100;101\nContactSetUnion;200;101;102;103;104;105;106;107;108;109");
    REQUIRE(cardAt<ContactSetUnionTable>(model, 0).id == 100);
    REQUIRE(cardAt<ContactSetUnionTable>(model, 0).sets == std::vector<base::EntityId>{101});
    REQUIRE(cardAt<ContactSetUnionTable>(model, 1).sets ==
            std::vector<base::EntityId>{101, 102, 103, 104, 105, 106, 107, 108, 109});
    // The sets are BCTSET cards: each is looked up as a contact table, and none exists here.
    REQUIRE(unresolvedIn(model) == 10);
    RecordingVisitor visitor;
    model.table<ContactSetUnionTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"memberSetId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::ContactTable});
    Model withSet = readBulk("ContactSetUnion;100;101\nContactSet;101;1;2");
    REQUIRE(unresolvedIn(withSet) == 0);
    // The CSID and the sets are IDs of the contact table category: a renumbering of the tables moves both.
    Renumbering::apply(withSet, RenumberSpace::ContactTable, Renumbering::offset(1000));
    REQUIRE(cardAt<ContactSetUnionTable>(withSet).id == 1100);
    REQUIRE(cardAt<ContactSetUnionTable>(withSet).sets == std::vector<base::EntityId>{1101});
    REQUIRE(unresolvedIn(withSet) == 0);
    requireRoundTrip(model);
    REQUIRE_THROWS_AS(readBulk("ContactSetUnion;100;101\nContactSetUnion;100;102"),
                      base::DeckError); // the CSID is unique
    expectInvalid("ContactSetUnion;100", "memberSetId");
    expectInvalid("ContactSetUnion;100;100", "memberSetId");
    expectInvalid("ContactSetUnion;100;-1", "memberSetId");
    expectInvalid("ContactSetUnion;0;1", "id");
}
