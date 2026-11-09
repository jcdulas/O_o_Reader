#pragma once

#include "oor/query/providers/quantities/ElementQuantities.hpp"
#include "oor/query/results/ResultModelIndex.hpp"

#include <array>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

namespace oor::query
{

// The value fields of the flats of the plies of a laminate: the stresses `o11 o22 t12 t1z t2z` (with `omax` and `omin`,
// the principal values the table gives, and `maxShear`), the strains `e11 e22 e12 e1z e2z`. The layer of a row is the
// ply.
[[nodiscard]] const std::vector<std::string>& plyStressFields();
[[nodiscard]] const std::vector<std::string>& plyStrainFields();

// The plies of one stress or strain table of a laminate (the containers called `*_composite`) as a flat: the rows of
// the plies the request keeps. Nothing for another table. A ply is a place of its own: the flat has no location and
// no position.
[[nodiscard]] std::optional<ElementFlat> flattenPlyTable(const ResultQueryContext& context, const TableRef& ref,
                                                         bool strain, const ElementRequest& request);

// The stress (or strain) tables of the laminates the request keeps, in the order of the sets and of the file.
[[nodiscard]] std::vector<TableRef> plyTables(const ResultQueryContext& context, bool strain,
                                              const ElementRequest& request);
// The flats of `tables` (flattenPlyTable), made on the threads of the call, in the order of the tables; the tables with
// no row asked for give none.
[[nodiscard]] std::vector<ElementFlat> flattenPlyTables(const ResultQueryContext& context,
                                                        const std::vector<TableRef>& tables, bool strain,
                                                        const ElementRequest& request);

// The stress (or strain) tables of the laminates the request keeps, to be read straight into a block (the stored rows
// of the plies at the speed of the stress tables): the tables of plyTables decoded on the threads of the call, each
// table's components in the order of plyStressFields (plyStrainFields), its layers the plies, one place code (0, the
// ply) for every row; the labels shared by a table with those of the one before. Nothing when the flats must be made:
// a request of properties, layers or places, a complex table, the read switched off, or no row.
[[nodiscard]] std::optional<std::vector<DirectForceTable>> storedPlyTables(const ResultQueryContext& context,
                                                                           const ElementRequest& request, bool strain);
// The name that switches off the direct read of the plies (the test of its equality with the providers').
inline constexpr const char* PLY_TABLE_SWITCH = "stored ply stresses and strains read straight into a block";
// The name that switches off the failure index of the plies matched without the flats of its chain (buildPlyData).
inline constexpr const char* PLY_FAILURE_SWITCH = "failure index of the plies matched without the flats of the chain";

// The theory a failure index table names, as the margins name it ("TSAI-WU" -> "tsaiWu").
[[nodiscard]] std::string storedTheoryName(const std::string& stored);

// What a failure theory says of one ply: the index at the stresses given, and its two parts, quadratic and linear in
// the stresses, so that the index at stresses scaled by s is q s^2 + l s. The theories are named as the `theory`
// argument of the margins names them: "maxStress", "maxStrain", "hill", "hoffman", "tsaiWu".
struct PlyIndex
{
    double index = 0.0;
    double quadratic = 0.0;
    double linear = 0.0;
};

struct PlyMaterial
{
    double xt = 0.0;
    double xc = 0.0;
    double yt = 0.0;
    double yc = 0.0;
    double s = 0.0;
    double f12 = 0.0;
    double e1 = 0.0;
    double e2 = 0.0;
    double nu12 = 0.0;
    double g12 = 0.0;
    bool strainAllowables = false; // MAT8 STRN: the allowables are strains
    // Hashin: the strengths of the MAT8 HFAIL group (HF1 HF2 HF3 HF4 HF10 HF11: Xt Xc Yt Yc, the in-plane shear and
    // the transverse shear of the matrix), when the card has it; else Xt ... S above, the transverse shear being S
    std::optional<std::array<double, 6>> hashin;
    // Puck: the inclination parameters p(+) and p(-) of the fracture envelope in the (sigma2, tau21) plane (the
    // values VDI 2014 gives for carbon: 0.35 and 0.30)
    double puckTension = 0.35;
    double puckCompression = 0.30;
    // LaRC03: the ratio of the fracture toughnesses G_Ic / G_IIc and the angle of the fracture plane under pure
    // transverse compression (degrees); the strengths are taken as given (in-situ strengths are the caller's)
    double toughnessRatio = 1.0;
    double fractureAngle = 53.0;
};

// What a theory of failure modes says of one ply (Hashin, Puck, LaRC03): the index of its fibre mode and of its matrix
// (inter-fibre) mode, the larger of the two, the mode that governs ("fiberTension", "fiberCompression",
// "matrixTension", "matrixCompression"; Puck's inter-fibre modes "iffA", "iffB", "iffC"; LaRC03's
// "fiberKinking"), and the strength ratio: the factor on the stresses that brings the index to the limit.
struct PlyModes
{
    double fiber = std::numeric_limits<double>::quiet_NaN();
    double matrix = std::numeric_limits<double>::quiet_NaN();
    double index = std::numeric_limits<double>::quiet_NaN();
    double ratio = std::numeric_limits<double>::quiet_NaN();
    std::string mode;
};

// True for the theories of failure modes: "hashin", "puck", "larc03".
[[nodiscard]] bool isModeTheory(const std::string& theory) noexcept;

// The modes of a ply under the in-plane stresses (s1, s2, t12) in its own axes, by `theory` ("hashin", "puck",
// "larc03"), the strength ratio to `limit`. NaN when the material lacks a strength the theory needs. Hashin (1980,
// plane stress): fibre (s1/Xt)^2 + (t12/S)^2 or (s1/Xc)^2, matrix (s2/Yt)^2 + (t12/S)^2 or (s2/2St)^2 +
// ((Yc/2St)^2 - 1) s2/Yc + (t12/S)^2. Puck (Puck and Schurmann 2002, plane stress): fibre s1/Xt or -s1/Xc, inter-fibre
// modes A, B, C, each a stress exposure linear in the stresses. LaRC03 (Davila, Camanho and Rose 2005, plane stress):
// fibre tension s1/Xt, fibre kinking in the frame of the fibre misalignment, matrix tension and compression on the
// fracture plane whose angle gives the largest index (the sign changes of the derivative on a grid of 3.75 degrees,
// each maximum refined by a bracketed Newton search).
[[nodiscard]] PlyModes plyFailureModes(const std::string& theory, double s1, double s2, double t12,
                                       const PlyMaterial& material, double limit = 1.0);
// The same modes without the strength ratio (NaN): the index alone, which LaRC03 gives without the search of its
// factor.
[[nodiscard]] PlyModes plyModeIndices(const std::string& theory, double s1, double s2, double t12,
                                      const PlyMaterial& material);

// The strength ratio of a ply by any theory: the factor by which its stresses can be multiplied before its index
// reaches `limit` (the root of the quadratic of the theories of one index, the smallest of the modes for Hashin and
// Puck, a search for LaRC03). NaN when the index does not reach the limit, or the material lacks a strength.
[[nodiscard]] double plyStrengthRatio(const std::string& theory, double s1, double s2, double t12,
                                      const PlyMaterial& material, double limit = 1.0);

// The derivatives of the failure index and of the strength ratio (to `limit`) of a ply in its stresses (s1, s2, t12),
// a companion package: those of the smooth piece the state of stress is on (the branch of its signs, the component or
// mode that governs; at a switch, the piece the value takes: one side's), in closed form; LaRC03's by the envelope
// theorem at the fracture plane found, and the implicit derivatives of its ratio (larc03Derivatives). NaN where the
// value is NaN. PlyGradients.cpp says how each theory is derived.
struct PlyGradient
{
    std::array<double, 3> index{std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(),
                                std::numeric_limits<double>::quiet_NaN()};
    std::array<double, 3> ratio{std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(),
                                std::numeric_limits<double>::quiet_NaN()};
};
[[nodiscard]] PlyGradient plyGradient(const std::string& theory, double s1, double s2, double t12,
                                      const PlyMaterial& material, double limit = 1.0);

// The second derivatives of the failure index and of the strength ratio of a ply in its stresses (s1, s2, t12),
// a companion package: 3 x 3, row-major, symmetric (one triangle computed, the other mirrored), of the same piece as
// plyGradient (one side's at a switch), in closed form; NaN where the gradient is NaN. The gradient is plyGradient's to
// the bit.
struct PlyHessian
{
    PlyGradient gradient;
    std::array<double, 9> index{std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(),
                                std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(),
                                std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(),
                                std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(),
                                std::numeric_limits<double>::quiet_NaN()};
    std::array<double, 9> ratio = index;
};
[[nodiscard]] PlyHessian plyHessian(const std::string& theory, double s1, double s2, double t12,
                                    const PlyMaterial& material, double limit = 1.0);

// The derivatives of LaRC03's index and ratio (PlyFailureModes.cpp): exact to rounding, those of the governing mode at
// the fracture plane found, the angle taken out by the envelope theorem (with `hessian`, H - f_sa f_as / f_aa); the
// ratio's from I(R sigma) = limit differentiated once and twice. Fills `hessian` when it is given.
[[nodiscard]] PlyGradient larc03Derivatives(double s1, double s2, double t12, const PlyMaterial& material, double limit,
                                            PlyHessian* hessian);

// The failure index of a ply with the in-plane stresses (s1, s2, t12) in its own axes. NaN when the material lacks an
// allowable the theory needs (the compressive allowables default to the tensile ones in the card, so only a missing
// tensile allowable or shear allowable gives NaN).
[[nodiscard]] PlyIndex plyFailureIndex(const std::string& theory, double s1, double s2, double t12,
                                       const PlyMaterial& material);

// A theory by name, read once for the plies of many rows (None for a name that is not one).
enum class PlyTheory : std::uint8_t
{
    None,
    MaxStress,
    MaxStrain,
    Hill,
    Hoffman,
    TsaiWu,
    Hashin,
    Puck,
    Larc03
};
[[nodiscard]] PlyTheory plyTheoryOf(const std::string& theory) noexcept;
[[nodiscard]] PlyIndex plyFailureIndex(PlyTheory theory, double s1, double s2, double t12, const PlyMaterial& material);

// The factor by which the stresses can be multiplied before the index reaches `limit`: the positive root of q s^2 + l s
// = limit (the reserve factor of the theory); NaN when the index is zero.
[[nodiscard]] double scaleToLimit(const PlyIndex& index, double limit);

// The bonding index of a ply (the solver's failure index for bonding, PCOMP SB): its interlaminar shear against the
// allowable shear of the bond, max(|t1z|, |t2z|) / sb as the solver stores it, or the resultant sqrt(t1z^2 + t2z^2) /
// sb with `resultant`. NaN for an allowable that is not positive (a blank SB). Linear in the stresses: the strength
// ratio of the bond is its inverse.
[[nodiscard]] double bondIndex(double t1z, double t2z, double sb, bool resultant) noexcept;

// The name of a PCOMP failure theory as the margins name it ("" for none).
[[nodiscard]] std::string theoryName(int failureTheory);

// One ply of the laminate of an element as the model gives it.
struct PlyFacts
{
    std::int32_t globalId = 0; // the global ply ID of a PCOMPG, 0 for a PCOMP
    std::int32_t material = 0;
    double angle = std::numeric_limits<double>::quiet_NaN();
    double thickness = std::numeric_limits<double>::quiet_NaN();
    std::optional<PlyMaterial> allowables; // the MAT8 of the ply
};

// The laminate of one element: its plies in stacking order (empty when the set has no model, the element no
// laminate), the theory of its PCOMP or PCOMPG ("" when it has none, as the margins name it) and its SB, the allowable
// interlaminar shear stress (NaN when blank or none).
struct LaminateFacts
{
    std::vector<PlyFacts> plies;
    std::string theory;
    double bondAllowable = std::numeric_limits<double>::quiet_NaN();
};

// The laminates of the elements of a result set, read from the model once for each element.
class LaminateLookup
{
public:
    explicit LaminateLookup(const ResultQueryContext& context);
    // The laminate of the element with this file ID.
    [[nodiscard]] const LaminateFacts& facts(std::size_t set, std::int32_t resultEid);
    [[nodiscard]] const std::vector<PlyFacts>& plies(std::size_t set, std::int32_t resultEid)
    {
        return facts(set, resultEid).plies;
    }
    [[nodiscard]] const std::string& propertyTheory(std::size_t set, std::int32_t resultEid)
    {
        return facts(set, resultEid).theory;
    }
    [[nodiscard]] double bondAllowable(std::size_t set, std::int32_t resultEid)
    {
        return facts(set, resultEid).bondAllowable;
    }
    // Reads the laminates of the elements of the rows of `flat` (a run of rows of one element is read once; the
    // elements of a flat with the rows of `before` are read already), so that `found` gives them on any thread.
    void prepare(const ElementFlat& flat, const ElementFlat* before = nullptr);
    // The laminate of an element `prepare` read (else null): const, for the threads of a call.
    [[nodiscard]] const LaminateFacts* found(std::size_t set, std::int32_t resultEid) const;

private:
    [[nodiscard]] static LaminateFacts readFacts(const model::Model& model, const model::CardLocation& location);

    const ResultQueryContext& m_context;
    ResultModelIndex m_index;
    std::map<std::uint64_t, const LaminateFacts*> m_entries;
    // the laminates read, one a property card of a model: the elements of a property share it
    std::map<std::tuple<const model::Model*, std::uint16_t, std::uint32_t>, LaminateFacts> m_properties;
};

// The way the failure index of the plies is obtained (the request `theory` says which theory a recomputation uses): the
// stored index, else computed from the ply stresses and the allowables of the materials. The flats hold the field
// `failureIndex` and the text `failureTheory`.
[[nodiscard]] const ElementChain& failureIndexChain();

} // namespace oor::query
