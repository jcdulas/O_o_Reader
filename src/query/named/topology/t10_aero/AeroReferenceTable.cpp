#include "AeroQuerySupport.hpp"
#include "oor/model/cards/aero/aeroStaticParams/AeroStaticParamsTable.hpp"
#include "oor/model/cards/aero/aeroUnsteadyParams/AeroUnsteadyParamsTable.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/topology/t10_aero/AeroQueries.hpp"

#include <limits>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// aeroReferenceTable: the AERO cards (the unsteady analyses) and then the AEROS cards (the static ones). Each card
// fills the columns it has a field for and leaves the others null.
class AeroReferenceTableQuery final : public QueryBase
{
public:
    AeroReferenceTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const std::vector<model::AeroUnsteadyParamsCard> unsteadyCards =
            aero::cardsOf<model::AeroUnsteadyParamsTable>(*run.model);
        const std::vector<model::AeroStaticParamsCard> staticCards =
            aero::cardsOf<model::AeroStaticParamsTable>(*run.model);
        const std::size_t count = unsteadyCards.size() + staticCards.size();
        if (count == 0)
            return emptyTable(info().defaultColumns());

        std::vector<std::string> card;
        std::vector<std::int32_t> acsid(count), rcsid(count), symXz(count), symXy(count);
        std::vector<std::uint8_t> rcsidValid(count, 0);
        std::vector<double> velocity(count, NOT_A_NUMBER), chord(count), span(count, NOT_A_NUMBER),
            area(count, NOT_A_NUMBER), density(count, NOT_A_NUMBER);
        std::size_t row = 0;
        for (const model::AeroUnsteadyParamsCard& unsteady : unsteadyCards)
        {
            card.emplace_back("AeroUnsteadyParams");
            acsid[row] = unsteady.aeroFrameId;
            velocity[row] = unsteady.velocity.value_or(NOT_A_NUMBER);
            chord[row] = unsteady.referenceChord;
            density[row] = unsteady.referenceDensity;
            symXz[row] = unsteady.symmetricXz;
            symXy[row] = unsteady.symmetricXy;
            ++row;
        }
        for (const model::AeroStaticParamsCard& steady : staticCards)
        {
            card.emplace_back("AeroStaticParams");
            acsid[row] = steady.aeroFrameId;
            rcsid[row] = steady.referenceFrameId;
            rcsidValid[row] = 1;
            chord[row] = steady.referenceChord;
            span[row] = steady.referenceSpan;
            area[row] = steady.referenceArea;
            symXz[row] = steady.symmetricXz;
            symXy[row] = steady.symmetricXy;
            ++row;
        }
        TableBuilder builder;
        builder.dictionary("card", card);
        builder.int32("aeroFrameId", std::move(acsid));
        builder.int32("rigidBodyFrameId", std::move(rcsid), std::move(rcsidValid));
        builder.real("velocity", std::move(velocity));
        builder.real("refChord", std::move(chord));
        builder.real("refSpan", std::move(span));
        builder.real("refArea", std::move(area));
        builder.real("refDensity", std::move(density));
        builder.int32("symXz", std::move(symXz));
        builder.int32("symXy", std::move(symXy));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "aeroReferenceTable";
        info.family = "T10";
        info.description =
            "The AERO and AEROS cards, one row for each (the AERO cards first, then the AEROS ones, each in the order "
            "of the deck). AERO gives the `velocity` (null when blank), the reference chord and the reference density; "
            "AEROS gives `rigidBodyFrameId` (the system of the rigid body motions), the reference chord, span and "
            "area; the "
            "columns a card has no field for are null. `aeroFrameId` is the aerodynamic coordinate system, whose x "
            "axis is "
            "the flow. The symmetry keys are those of the card: the Case Control AESYMXZ and AESYMXY override them.";
        info.defaultSort =
            "card (AeroUnsteadyParams for an AERO, AeroStaticParams for an AEROS), then the order of the "
            "cards";
        ColumnListBuilder columns;
        columns.dictionary("card")
            .int32("aeroFrameId")
            .int32("rigidBodyFrameId")
            .real("velocity")
            .real("refChord")
            .real("refSpan");
        columns.real("refArea").real("refDensity").int32("symXz").int32("symXy");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addAeroReferenceTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<AeroReferenceTableQuery>());
}

} // namespace oor::query
