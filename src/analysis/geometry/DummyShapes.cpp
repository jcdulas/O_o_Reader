#include "ElementProvider.hpp"
#include "oor/model/cards/elements/userDefined1/UserDefined1Table.hpp"
#include "oor/model/cards/elements/userDefined2/UserDefined2Table.hpp"
#include "oor/model/cards/elements/userDefined3/UserDefined3Table.hpp"
#include "oor/model/cards/elements/userDefined4/UserDefined4Table.hpp"
#include "oor/model/cards/elements/userDefined5/UserDefined5Table.hpp"
#include "oor/model/cards/elements/userDefined6/UserDefined6Table.hpp"
#include "oor/model/cards/elements/userDefined7/UserDefined7Table.hpp"
#include "oor/model/cards/elements/userDefined8/UserDefined8Table.hpp"
#include "oor/model/cards/elements/userDefined9/UserDefined9Table.hpp"

// The user dummy elements CDUM1 to CDUM9.
//
// A dummy element is an element whose stiffness, mass and output the user writes in routines of his own, so nothing of
// its shape is known: the provider of each card type gives its grid points (the leading run of positive integers after
// the PID, see UserDefined1Card) and leaves every quantity undefined. Centre, normal, area, volume, length, thickness and
// the frames are NaN, the element is a point in the node-to-element index (it is not in the list of line elements) and
// it carries no mass.
//
// A grid point that no card defines is an error of the geometry (a GeometryError names the element and the node), as
// for every element. A CDUM whose first additional field is an integer that follows its last grid point directly has
// that integer among its nodes, because the card cannot tell it from a grid point (the NG of the ADUMi card says); the
// geometry of such an element then asks for a node of that number.

namespace oor::analysis::detail
{

namespace
{

struct UserDefined1Shape
{
    static void nodes(const model::UserDefined1Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const std::span<const base::EntityId> grids = table.nodes(row);
        ids.insert(ids.end(), grids.begin(), grids.end());
    }
    static void evaluate(const model::UserDefined1Table&, const Evaluation&, ElementValues&)
    {
        // A user element: nothing is known of its shape.
    }
};

struct UserDefined2Shape
{
    static void nodes(const model::UserDefined2Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const std::span<const base::EntityId> grids = table.nodes(row);
        ids.insert(ids.end(), grids.begin(), grids.end());
    }
    static void evaluate(const model::UserDefined2Table&, const Evaluation&, ElementValues&)
    {
        // A user element: nothing is known of its shape.
    }
};

struct UserDefined3Shape
{
    static void nodes(const model::UserDefined3Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const std::span<const base::EntityId> grids = table.nodes(row);
        ids.insert(ids.end(), grids.begin(), grids.end());
    }
    static void evaluate(const model::UserDefined3Table&, const Evaluation&, ElementValues&)
    {
        // A user element: nothing is known of its shape.
    }
};

struct UserDefined4Shape
{
    static void nodes(const model::UserDefined4Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const std::span<const base::EntityId> grids = table.nodes(row);
        ids.insert(ids.end(), grids.begin(), grids.end());
    }
    static void evaluate(const model::UserDefined4Table&, const Evaluation&, ElementValues&)
    {
        // A user element: nothing is known of its shape.
    }
};

struct UserDefined5Shape
{
    static void nodes(const model::UserDefined5Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const std::span<const base::EntityId> grids = table.nodes(row);
        ids.insert(ids.end(), grids.begin(), grids.end());
    }
    static void evaluate(const model::UserDefined5Table&, const Evaluation&, ElementValues&)
    {
        // A user element: nothing is known of its shape.
    }
};

struct UserDefined6Shape
{
    static void nodes(const model::UserDefined6Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const std::span<const base::EntityId> grids = table.nodes(row);
        ids.insert(ids.end(), grids.begin(), grids.end());
    }
    static void evaluate(const model::UserDefined6Table&, const Evaluation&, ElementValues&)
    {
        // A user element: nothing is known of its shape.
    }
};

struct UserDefined7Shape
{
    static void nodes(const model::UserDefined7Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const std::span<const base::EntityId> grids = table.nodes(row);
        ids.insert(ids.end(), grids.begin(), grids.end());
    }
    static void evaluate(const model::UserDefined7Table&, const Evaluation&, ElementValues&)
    {
        // A user element: nothing is known of its shape.
    }
};

// CDUM8 and CDUM9 are the numbers the format reference reserves for the crack tip elements CRAC2D and CRAC3D (ADUM8 and
// ADUM9); written as CDUM8 or CDUM9 they are dummy elements like the others and have no shape here either.
struct UserDefined8Shape
{
    static void nodes(const model::UserDefined8Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const std::span<const base::EntityId> grids = table.nodes(row);
        ids.insert(ids.end(), grids.begin(), grids.end());
    }
    static void evaluate(const model::UserDefined8Table&, const Evaluation&, ElementValues&)
    {
        // A user element: nothing is known of its shape.
    }
};

struct UserDefined9Shape
{
    static void nodes(const model::UserDefined9Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const std::span<const base::EntityId> grids = table.nodes(row);
        ids.insert(ids.end(), grids.begin(), grids.end());
    }
    static void evaluate(const model::UserDefined9Table&, const Evaluation&, ElementValues&)
    {
        // A user element: nothing is known of its shape.
    }
};

} // namespace

void registerDummyProviders(ProviderRegistry& registry)
{
    registry.add<model::UserDefined1Table, UserDefined1Shape>();
    registry.add<model::UserDefined2Table, UserDefined2Shape>();
    registry.add<model::UserDefined3Table, UserDefined3Shape>();
    registry.add<model::UserDefined4Table, UserDefined4Shape>();
    registry.add<model::UserDefined5Table, UserDefined5Shape>();
    registry.add<model::UserDefined6Table, UserDefined6Shape>();
    registry.add<model::UserDefined7Table, UserDefined7Shape>();
    registry.add<model::UserDefined8Table, UserDefined8Shape>();
    registry.add<model::UserDefined9Table, UserDefined9Shape>();
}

} // namespace oor::analysis::detail
