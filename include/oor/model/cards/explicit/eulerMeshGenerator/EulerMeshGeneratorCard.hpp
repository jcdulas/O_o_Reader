#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace oor::model
{

// The kind of Euler mesh (the TYPE field of a MESH): a mesh that follows a coupling surface by adding and removing
// elements as it moves, or a plain box of elements. The enumerators are numbered in the order of EULER_MESH_TYPE_WORDS.
enum class EulerMeshGeneratorType : std::uint8_t
{
    Adapt, // ADAPT: the mesh adapts itself around a coupling surface (a COUPLE names the MESH in its MESHID)
    Box    // BOX: a rectangular mesh of CHEXA elements aligned with the basic system (or with CID)
};
inline constexpr std::array<std::string_view, 2> EULER_MESH_TYPE_WORDS{"Adaptive", "Box"};

// The kind of mesh that is generated (the PROP field): the format reference lists EULER only, so the enumeration has
// one word.
enum class EulerMeshGeneratorProperty : std::uint8_t
{
    Euler // EULER (the default)
};
inline constexpr std::array<std::string_view, 1> EULER_MESH_PROPERTY_WORDS{"Euler"};

// How the element size of an ADAPT mesh changes during the run (the RESIZE field).
enum class EulerMeshGeneratorResize : std::uint8_t
{
    None,  // NONE (the default): the sizes stay as they are
    Scale, // SCALE: the elements are scaled by a factor that is a function of time (the tables TID-X, TID-Y, TID-Z)
    Length // LENGTH: the element lengths are given as a function of time
};
inline constexpr std::array<std::string_view, 3> EULER_MESH_RESIZE_WORDS{"None", "Scale", "Length"};

// Which Euler elements are re-meshed when the size changes (the METHOD field).
enum class EulerMeshGeneratorRemesh : std::uint8_t
{
    All,     // ALL (the default): every existing element, void regions kept
    Material // MATERIAL: only the elements that contain material, void regions removed
};
inline constexpr std::array<std::string_view, 2> EULER_MESH_REMESH_WORDS{"All", "Material"};

// Where the grading of a biased mesh starts (the BIAS field): from the centre of the box, or from the reference point.
enum class EulerMeshGeneratorBiasStart : std::uint8_t
{
    Center,   // CENTER
    Reference // REF
};
inline constexpr std::array<std::string_view, 2> EULER_MESH_BIAS_START_WORDS{"Center", "Reference"};

// The direction in which the cubes of the mesh are first shared among the processors (the PROCDIR field).
enum class EulerMeshGeneratorPartition : std::uint8_t
{
    X,     // X (the default)
    Y,     // Y
    Z,     // Z
    User,  // USER: the number of cubes in each direction is given by NPX, NPY, NPZ
    Simple // SIMPLE
};
inline constexpr std::array<std::string_view, 5> EULER_MESH_PARTITION_WORDS{"XDirection", "YDirection", "ZDirection",
                                                                            "User", "Simple"};

// The default of XREF, YREF and ZREF.
inline constexpr double EULER_MESH_DEFAULT_REFERENCE = -1.0e-6;

// One MESH card (SOL 700): the generator of an Eulerian mesh, either a box of elements or a mesh that adapts around a
// coupling surface. It generates the grid points and the CHEXA elements of the Euler domain, with the property PID (a
// PEULER or a PEULER1). The card shares its name with the PARAM,MESH parameter of the format reference chapter on
// parameters, which is another thing; the MESH and BIAS cards are the Bulk Data entries of the Euler mesh.
//
// Card layout, eight lines, each of eight fields (a line is a block of fields 8n+1 to 8n+8):
//   1 to 8   MID TYPE DXEL DYEL DZEL XREF YREF ZREF
//   9 to 16  X0 Y0 Z0 DX DY DZ (two blank fields)
//   17 to 24 NX NY NZ SUBMESH NSTGP NSTEL PROP PID
//   25 to 32 RESIZE TID-X TID-Y TID-Z METHOD (three blank fields)
//   33 to 40 BIAS GROWX GROWY GROWZ IBIDX IBIDY IBIDZ (one blank field)
//   41 to 48 X0BX Y0BX Z0BX DXBX DYBX DZBX (two blank fields); the grid of the format prints the first and the third as
//            XOBX and ZOBY, the descriptions (and this card) use X0BX and Z0BX
//   49 to 56 CID (seven blank fields)
//   57 to 64 NELCUBE NBX NBY NBZ PROCDIR NPX NPY NPZ
// A field is blank when the format reference leaves it out of the picture; the card refuses a value in such a field.
//
// What a blank field means, and how the card stores it:
//   - DXEL, DYEL, DZEL, DX, DY, DZ, NX, NY, NZ and the auxiliary box X0BX to DZBX have no default: blank is None, and
//   the
//     check asks for the combinations of the format (see below). DXEL is not computed from DX / NX: the card keeps what
//     was written.
//   - XREF, YREF and ZREF are -1.0e-6 when blank (a static default: the writer leaves the default blank).
//   - X0, Y0 and Z0 default to XREF, YREF and ZREF for a BOX mesh and are not used by an ADAPT mesh. A default that
//     depends on another field is stored resolved, so the reader gives a BOX mesh the reference point where they are
//     blank and always writes them; an ADAPT mesh keeps them as written (None when blank).
//   - TID-Y and TID-Z default to TID-X, NBY and NBZ to NBX, NPY and NPZ to NPX: stored resolved when the first one is
//     given, always written (so the check asks for them whenever TID-X, NBX, NPX is given).
//   - NBX has the default 1 but, when given, takes precedence over NELCUBE, so a blank NBX is kept
//     blank (the cubes then come from NELCUBE); NPX likewise (it is required for PROCDIR = USER only).
//   - NSTGP and NSTEL: blank (0) means one more than the largest number in use, which only the model knows.
//   - SUBMESH, CID, the three IBID fields: 0 is blank. NELCUBE is 1 when blank.
// The rules of the card (every one comes from the format): TYPE is required; PID is required; a BOX needs NX, NY and NZ
// and either (DXEL, DYEL, DZEL) or (DX, DY, DZ); an ADAPT needs (DXEL, DYEL, DZEL) or (DX, DY, DZ) with (NX, NY, NZ)
// (there are only these two forms); RESIZE other than NONE is for ADAPT only and needs TID-X; CID cannot be used with
// ADAPT; a BIAS needs GROWX, GROWY and GROWZ unless the IBID of that direction is given (the IBID overrules the grow
// factor); the auxiliary box is all six values or none; PROCDIR = USER needs NPX and each NBi a multiple of the NPi.
struct EulerMeshGeneratorCard
{
    // Fields 1 to 8
    base::EntityId id = 0; // unique among the meshes
    EulerMeshGeneratorType type = EulerMeshGeneratorType::Box;
    std::optional<double> elemSizeX; // element sizes
    std::optional<double> elemSizeY;
    std::optional<double> elemSizeZ;
    double xReference = EULER_MESH_DEFAULT_REFERENCE; // reference point
    double yReference = EULER_MESH_DEFAULT_REFERENCE;
    double zReference = EULER_MESH_DEFAULT_REFERENCE;

    // Fields 9 to 16
    std::optional<double> originX; // origin of a BOX mesh, resolved from the reference point
    std::optional<double> originY;
    std::optional<double> originZ;
    std::optional<double> widthX; // widths of the mesh
    std::optional<double> widthY;
    std::optional<double> widthZ;

    // Fields 17 to 24
    std::optional<std::int32_t> elemCountX; // numbers of elements
    std::optional<std::int32_t> elemCountY;
    std::optional<std::int32_t> elemCountZ;
    base::EntityId subMeshId = 0;   // a finer MESH that replaces a part of this one, 0 for none
    base::EntityId firstNodeId = 0; // first grid point number, 0 for the model's next free one
    base::EntityId firstElemId = 0; // first element number, 0 for the model's next free one
    EulerMeshGeneratorProperty propKind = EulerMeshGeneratorProperty::Euler;
    base::EntityId propId = 0; // a PEULER or PEULER1

    // Fields 25 to 32
    EulerMeshGeneratorResize resize = EulerMeshGeneratorResize::None;
    base::EntityId resizeTableIdX = 0; // TABLED1 of the resizing in each direction (TID-Y, TID-Z resolved from TID-X)
    base::EntityId resizeTableIdY = 0;
    base::EntityId resizeTableIdZ = 0;
    EulerMeshGeneratorRemesh method = EulerMeshGeneratorRemesh::All;

    // Fields 33 to 40
    std::optional<EulerMeshGeneratorBiasStart> bias; // blank: no bias
    std::optional<double> growthX;                   // ratio of the finest to the coarsest element size
    std::optional<double> growthY;
    std::optional<double> growthZ;
    base::EntityId biasIdX = 0; // the BIAS card of each direction, 0 for none
    base::EntityId biasIdY = 0;
    base::EntityId biasIdZ = 0;

    // Fields 41 to 48: the auxiliary output box of an ADAPT mesh (start point and widths)
    std::optional<double> boxOriginX;
    std::optional<double> boxOriginY;
    std::optional<double> boxOriginZ;
    std::optional<double> boxWidthX;
    std::optional<double> boxWidthY;
    std::optional<double> boxWidthZ;

    // Fields 49 to 56
    base::EntityId frameId = 0; // a coordinate system that places the box, 0 for the basic one

    // Fields 57 to 64
    std::int32_t elemsPerCube = 1;          // guideline for the number of elements per cube
    std::optional<std::int32_t> cubeCountX; // number of cubes in each direction (NBY, NBZ resolved from NBX)
    std::optional<std::int32_t> cubeCountY;
    std::optional<std::int32_t> cubeCountZ;
    EulerMeshGeneratorPartition partitionDirection = EulerMeshGeneratorPartition::X;
    std::optional<std::int32_t>
        partitionCountX; // partition of the cubes for PROCDIR = USER (NPY, NPZ resolved from NPX)
    std::optional<std::int32_t> partitionCountY;
    std::optional<std::int32_t> partitionCountZ;
};

} // namespace oor::model
