#pragma once

#include "oor/base/math/Vec3.hpp"
#include "oor/query/named/NamedQuery.hpp"
#include "oor/query/providers/quantities/LaminateStrainRows.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace oor::query
{

// What a kind of result block reads (the building blocks for the plugin packages): the named query whose providers
// give its rows, the components it gives by default, and what tells the places of an element apart.
struct ResultBlockKind
{
    std::string kind;  // "stress", "strain", "plyStress", "plyStrain", "shellForces", "elementForces",
                       // "fastenerForces"
    std::string query; // "stressTable", ...: its parameters are those of the call, its providers give the rows (the
                       // ply strains, which no query lists, take the parameters of compositePlyTable)
    std::vector<std::string> components;
    std::string onlyPlace{};     // the name of the one place of an element whose rows name none ("" the rows name it)
    bool layered = false;        // the layer of a row is a place of its own (the fibre of a shell, the ply)
    bool nodes = false;          // the grid point of a row is a place of its own (the corners of a shell or a solid)
    std::string positionField{}; // the position of a place: a value field ("fiberDistance"), "station" (x/L along a
                                 // beam, which also tells the places apart), "" none
    std::string layerIdField{};  // the value field of the global ID of a layer ("plyId" of a PCOMPG ply), "" none
};

// The kinds, in the order above.
[[nodiscard]] const std::vector<ResultBlockKind>& resultBlockKinds();
// One kind by name; QueryError naming the kinds for another name.
[[nodiscard]] const ResultBlockKind& resultBlockKind(std::string_view kind);

// The components of one result kind for a list of elements and a list of cases, as one dense array: values[case]
// [element][location][component], NaN where an element has no such location or a case has no value. The locations of
// an element are its places, the same for every case, in the order of their grid point (the centre first), layer and
// station; an element has as many as it needs, the others are masked.
// An allocator whose vectors leave the numbers they are resized to unwritten: the values of a block that its threads
// all write once (a companion package: the first writes of a block of tens of megabytes, page by page, on one thread,
// were a tenth of a read of the forces of bars and beams).
template <typename T>
struct UnwrittenAllocator : std::allocator<T>
{
    template <typename U>
    struct rebind
    {
        using other = UnwrittenAllocator<U>;
    };
    UnwrittenAllocator() noexcept = default;
    template <typename U>
    explicit UnwrittenAllocator(const UnwrittenAllocator<U>&) noexcept
    {
    }
    template <typename U>
    void construct(U* place) noexcept
    {
        ::new (static_cast<void*>(place)) U;
    }
    template <typename U, typename... Arguments>
    void construct(U* place, Arguments&&... arguments)
    {
        ::new (static_cast<void*>(place)) U(std::forward<Arguments>(arguments)...);
    }
};

struct ResultBlock
{
    std::size_t caseCount = 0;
    std::size_t elementCount = 0;
    std::size_t locationCount = 0;
    std::size_t componentCount = 0;
    bool single = false;        // every row is of a table of single precision: the values are those of float32 numbers
    std::vector<double> values; // caseCount * elementCount * locationCount * componentCount
    // The values as float32 instead, when `single` and the caller asked for them narrow (`values` is then empty).
    std::vector<float, UnwrittenAllocator<float>> singleValues;
    std::vector<std::uint8_t> mask; // elementCount * locationCount: 1 where the element has the location
    std::vector<std::int32_t> elementIds;
    std::vector<std::string> locationNames; // elementCount * locationCount; "" where masked
    // The same names as codes into `locationTexts` instead (`locationNames` then empty), when a block asked narrow
    // gives them so (the forces of bars and beams: some 10 ns a place saved).
    std::vector<std::uint8_t> locationCodes;
    std::vector<std::string> locationTexts;
    std::vector<std::int32_t> locationNodes;
    std::vector<std::int32_t> layers;
    std::vector<std::int32_t> layerIds;
    std::vector<double> positions;
    std::vector<std::string> components;
    std::vector<std::string> caseAnalyses; // "" when one result set is attached
    std::vector<std::int32_t> caseSubcases;
    std::vector<double> caseSteps;    // NaN for a case with no step
    std::vector<std::string> sources; // per case: the providers of its rows, joined
};

// Reads a block with the arguments of `run` (those of the query of `kind`: `elementIds`, `subcases`, `frame`,
// `onMissing`, ...). The rows are those the query would list, taken from its providers before they are made a table
// (a block keeps a few columns of many rows: the table of every column would cost more than the rows). `components`:
// the values to keep (empty: the kind's own); `elementOrder`: the elements in this order, one even when it has no row
// (masked), else every element with a row by increasing ID; `casePairs`: the (result set, subcase) pairs to keep
// (empty: every case the arguments select; the arguments `analyses` and `subcases` alone keep every pair of the two).
// The cases are in the order of the result set, subcase and step. QueryError for complex results or a component the
// kind does not have. `narrow`: a block of single precision values keeps them as float32 (`singleValues`).
// `sandwichCore`: the core plies of the kind "sandwich" (the thickest ply of each laminate by default); QueryError when
// it declares one for another kind.
[[nodiscard]] ResultBlock readResultBlock(const QueryRun& run, const ResultBlockKind& kind,
                                          const std::vector<std::string>& components,
                                          const std::optional<std::vector<std::int32_t>>& elementOrder,
                                          const std::vector<std::pair<std::string, std::int32_t>>& casePairs = {},
                                          bool narrow = false, const SandwichCore& sandwichCore = {});

// The axes a caller gives one group, in the basic system: the x direction, and the
// normal (axis 3) or NaN for the area-weighted mean normal of the group; x is projected on the plane normal to axis 3.
// With `full`, `x`, `y` and `z` are the three axes as given (orthonormal and right-handed, else refused), used as they
// are.
struct GroupAxes
{
    base::Vec3 x;
    base::Vec3 y;
    base::Vec3 z;
    bool full = false;
};

// The frame of the averages over groups: the material axes the elements of each group share, a coordinate system (its
// axes at the area-weighted centre of the group; 0 the basic system), or the axes given for each group (`axes`, one
// per group, in their order: then `material` and `system` are not used). Whatever the form, axis 3 of the frame of a
// group must be within `normalTolerance` degrees of the area-weighted mean normal of its elements (either sense), else
// the group is refused.
struct GroupFrame
{
    bool material = false;
    std::int32_t system = 0;
    std::vector<GroupAxes> axes;
    double normalTolerance = 10.0;
};

// The averages over groups of shells of the centre values of "shellForces" or "laminateStrains":
// one row per group (the second axis, `elementIds` the positions of the groups), each element's values turned from its
// element frame into the frame of the group (the plane tensors as tensors, the transverse shears as vectors).
// Without `weights` (empty): the area-weighted average, the sum of area x value over the elements of the group that
// have a value, over their area. With `weights` (one list per group, a weight per element of the group, a companion
// package R1): the weighted sum, the sum of weight x value over every element of the group (a case where one has no
// value gives NaN). A component an element lacks (NaN) makes the component of its group NaN, and a plane tensor with a
// component NaN is NaN once turned. `run` reads the elements in their element frame. QueryError for another kind, a
// run without the model, an element without area, a group whose elements do not share their material axes when
// `frame` is the material one, given axes that are not a frame, or a frame whose axis 3 is not the normal of the
// group (naming the group).
[[nodiscard]] ResultBlock readGroupBlock(const QueryRun& run, const ResultBlockKind& kind,
                                         const std::vector<std::string>& components,
                                         const std::vector<std::vector<std::int32_t>>& groups,
                                         const std::vector<std::string>& groupNames, const GroupFrame& frame,
                                         const std::vector<std::pair<std::string, std::int32_t>>& casePairs = {},
                                         const std::vector<std::vector<double>>& weights = {});

} // namespace oor::query
