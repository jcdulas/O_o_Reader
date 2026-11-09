// The geometry of the interface elements (CACINF3, CACINF4, CAXISYM, CBUSH2D, CIFHEX, CIFPENT, CIFQDX, CIFQUAD): the
// base of an acoustic infinite element is a shell with no thickness, the axisymmetric line and the 2-D bush are
// segments, and a cohesive element has its centre only.

#include "oor/analysis/frames/FrameMath.hpp"
#include "oor/analysis/geometry/ElementGeometry.hpp"
#include "oor/model/cards/elements/acousticInfiniteTria3/AcousticInfiniteTria3Table.hpp"
#include "oor/model/cards/elements/acousticInfiniteQuad4/AcousticInfiniteQuad4Table.hpp"
#include "oor/model/cards/elements/shellAxisym/ShellAxisymTable.hpp"
#include "oor/model/cards/elements/bushing2D/Bushing2DTable.hpp"
#include "oor/model/cards/elements/cohesiveAxisymQuad/CohesiveAxisymQuadTable.hpp"
#include "oor/model/cards/elements/cohesiveHexa/CohesiveHexaTable.hpp"
#include "oor/model/cards/elements/cohesivePenta/CohesivePentaTable.hpp"
#include "oor/model/cards/elements/cohesiveQuad/CohesiveQuadTable.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <string>

using namespace oor;
using namespace oor::analysis;
using namespace oor::model;
using Catch::Approx;
using oor::testing::readModelText;

namespace
{

constexpr double EPS = 1.0e-12;

ElementValues geometryOf(const Model& model, base::EntityId eid)
{
    const auto location = model.locate(base::IdSpace::Element, eid);
    REQUIRE(location.has_value());
    return ElementGeometry::of(model, *location);
}

void requireVec(const base::Vec3& actual, double x, double y, double z, double margin = EPS)
{
    REQUIRE(actual.x == Approx(x).margin(margin));
    REQUIRE(actual.y == Approx(y).margin(margin));
    REQUIRE(actual.z == Approx(z).margin(margin));
}

// A node record: id, x, y, z (in the basic system).
std::string grid(int id, double x, double y, double z)
{
    return "Node;" + std::to_string(id) + ";;" + std::to_string(x) + ";" + std::to_string(y) + ";" + std::to_string(z) +
           "\n";
}

// Only the centre of a cohesive element is defined: everything else is NaN.
void requireCentreOnly(const ElementValues& values, double x, double y, double z)
{
    requireVec(values.center, x, y, z);
    REQUIRE(std::isnan(values.area));
    REQUIRE(std::isnan(values.volume));
    REQUIRE(std::isnan(values.length));
    REQUIRE(std::isnan(values.normal.x));
}

} // namespace

TEST_CASE("every interface element card type has geometry", "[analysis][geometry][interface]")
{
    REQUIRE(ElementGeometry::supports(typeIndexOf<AcousticInfiniteTria3Table>()));
    REQUIRE(ElementGeometry::supports(typeIndexOf<AcousticInfiniteQuad4Table>()));
    REQUIRE(ElementGeometry::supports(typeIndexOf<ShellAxisymTable>()));
    REQUIRE(ElementGeometry::supports(typeIndexOf<Bushing2DTable>()));
    REQUIRE(ElementGeometry::supports(typeIndexOf<CohesiveHexaTable>()));
    REQUIRE(ElementGeometry::supports(typeIndexOf<CohesivePentaTable>()));
    REQUIRE(ElementGeometry::supports(typeIndexOf<CohesiveAxisymQuadTable>()));
    REQUIRE(ElementGeometry::supports(typeIndexOf<CohesiveQuadTable>()));
}

TEST_CASE("the base of an acoustic infinite element is a shell with no thickness", "[analysis][geometry][interface]")
{
    const Model model = readModelText(grid(1, 0, 0, 0) + grid(2, 1, 0, 0) + grid(3, 1, 1, 0) + grid(4, 0, 1, 0) +
                                      "AcousticInfiniteQuad4;700;10;1;2;3;4\nAcousticInfiniteTria3;701;10;1;2;"
                                      "3\nAcousticInfiniteTria3;702;10;1;3;2\n");
    ElementValues values = geometryOf(model, 700);
    REQUIRE(values.area == Approx(1.0));
    requireVec(values.normal, 0, 0, 1);
    requireVec(values.center, 0.5, 0.5, 0.0);
    REQUIRE(std::isnan(values.thickness));
    REQUIRE(std::isnan(values.volume));
    REQUIRE(std::isnan(values.length));
    values = geometryOf(model, 701);
    REQUIRE(values.area == Approx(0.5));
    requireVec(values.normal, 0, 0, 1);
    requireVec(values.center, 2.0 / 3.0, 1.0 / 3.0, 0.0);
    // The normal follows the right-hand rule on the order of the points.
    values = geometryOf(model, 702);
    requireVec(values.normal, 0, 0, -1);
    REQUIRE(values.area == Approx(0.5));
}

TEST_CASE("the axisymmetric line has the middle of its chord and the length of its curve",
          "[analysis][geometry][interface]")
{
    // G1 (1, 1), G2 (4, 5): a chord of 5. G3 (1, 5) is a point of the curve: the chords G1-G3 and G3-G2 are 4 and 3.
    const Model model = readModelText(
        grid(1, 1, 1, 0) + grid(2, 4, 5, 0) + grid(3, 1, 5, 0) +
        "ShellAxisym;710;5;1;2\nShellAxisym;711;5;1;2;3\nShellAxisym;712;5;1;2;99\nScalarPoint;99\n");
    ElementValues values = geometryOf(model, 710);
    requireVec(values.center, 2.5, 3.0, 0.0);
    REQUIRE(values.length == Approx(5.0));
    REQUIRE(std::isnan(values.area));
    REQUIRE(std::isnan(values.volume));
    values = geometryOf(model, 711);
    requireVec(values.center, 2.5, 3.0, 0.0); // the middle of the chord G1-G2, not the point G3
    REQUIRE(values.length == Approx(7.0));
    // A G3 that is not a grid point leaves the length undefined, and the centre stays the middle of the chord.
    values = geometryOf(model, 712);
    requireVec(values.center, 2.5, 3.0, 0.0);
    REQUIRE(std::isnan(values.length));
}

TEST_CASE("the 2-D bush is the segment between its two grids", "[analysis][geometry][interface]")
{
    const Model model =
        readModelText(grid(1, 0, 0, 0) + grid(2, 0, 0, 0) + grid(3, 3, 4, 0) +
                      "Bushing2D;720;5;1;2\nBushing2D;721;5;1;3;;PlaneYz\nBushing2D;722;5;1;99\nScalarPoint;99\n");
    ElementValues values = geometryOf(model, 720); // the format asks for two coincident grids
    requireVec(values.center, 0, 0, 0);
    REQUIRE(values.length == Approx(0.0).margin(EPS));
    values = geometryOf(model, 721);
    requireVec(values.center, 1.5, 2.0, 0.0);
    REQUIRE(values.length == Approx(5.0));
    REQUIRE(std::isnan(values.area));
    values = geometryOf(model, 722); // GB is not a grid point
    REQUIRE(std::isnan(values.center.x));
    REQUIRE(std::isnan(values.length));
}

TEST_CASE("the cohesive elements have the mean of their corner points as centre", "[analysis][geometry][interface]")
{
    // The unit cube for the CIFHEX: the bottom face z = 0 (G1 to G4) and the top face z = 1 (G5 to G8); the edge point
    // 9 is far away, to show that the optional points do not count.
    std::string deck = grid(1, 0, 0, 0) + grid(2, 1, 0, 0) + grid(3, 1, 1, 0) + grid(4, 0, 1, 0);
    deck += grid(5, 0, 0, 1) + grid(6, 1, 0, 1) + grid(7, 1, 1, 1) + grid(8, 0, 1, 1) + grid(9, 9, 9, 9);
    deck += "ScalarPoint;99\nCohesiveHexa;730;7;1;2;3;4;5;6;7;8;9\nCohesiveHexa;731;7;1;2;3;4;5;99;7;8\n";
    // A wedge: the bottom triangle G1 to G3 and the top triangle G4 to G6 (the grids 1, 2, 4 and 5, 6, 8).
    deck += "CohesivePenta;732;7;1;2;4;5;6;8\n";
    // Planar and axisymmetric layers: the corners G1 and G2 below, G3 and G4 above.
    deck += "CohesiveQuad;733;7;1;2;6;5\nCohesiveQuad;734;7;1;2;6;5;9\nCohesiveAxisymQuad;735;7;1;2;6;5\n";
    const Model model = readModelText(deck);
    requireCentreOnly(geometryOf(model, 730), 0.5, 0.5, 0.5);
    // A corner that is not a grid point: no centre (and still no other quantity).
    const ElementValues missing = geometryOf(model, 731);
    REQUIRE(std::isnan(missing.center.x));
    requireCentreOnly(geometryOf(model, 732), 1.0 / 3.0, 1.0 / 3.0, 0.5);
    requireCentreOnly(geometryOf(model, 733), 0.5, 0.0, 0.5);
    requireCentreOnly(geometryOf(model, 734), 0.5, 0.0, 0.5);
    requireCentreOnly(geometryOf(model, 735), 0.5, 0.0, 0.5);
}
