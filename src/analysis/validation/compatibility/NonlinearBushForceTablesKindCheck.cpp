#include "oor/analysis/validation/compatibility/CompatibilityChecks.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/cards/nonlinear_contact/nonlinearBushForceTables/NonlinearBushForceTablesTable.hpp"

#include <string>
#include <string_view>

namespace oor::analysis
{

namespace
{

// Every term Pij of an LBSH3DG is the ID of a TABLED5, the table that gives a TABLED1 for each rotor speed (format
// reference LBSH3DG, Pij). The terms are references to the dynamic tables, which resolve to any TABLED1 to TABLED5, so
// a term that names a TABLED1 reads without a reference problem and the solver finds no table of rotor speeds.
class NonlinearBushForceTablesKindCheck final : public ValidationCheck
{
public:
    static constexpr CheckInfo INFO{"compat.nonlinear-bush-force-tables-kind", base::Severity::Error, "compatibility",
                                    "A term of a nonlinear bush force table card names a table that is not a dynamic "
                                    "table of type 5."};

    [[nodiscard]] const CheckInfo& info() const noexcept override { return INFO; }

    void run(const ValidationContext& context, FindingSink& sink) const override
    {
        const model::Model& model = context.model();
        forEachCard<model::NonlinearBushForceTablesTable>(
            model,
            [&](const model::NonlinearBushForceTablesCard& card, model::CardLocation where)
            {
                for (std::size_t row = 0; row < model::BUSHING_MATRIX_SIZE; ++row)
                {
                    for (std::size_t column = 0; column < model::BUSHING_MATRIX_SIZE; ++column)
                        checkTerm(model, sink, where, row, column, card.tables[row][column]);
                }
            });
    }

private:
    static void checkTerm(const model::Model& model, FindingSink& sink, model::CardLocation where, std::size_t row,
                          std::size_t column, base::EntityId table)
    {
        if (table <= 0)
            return;
        const std::optional<model::CardLocation> found = model.locate(base::IdSpace::DynamicTable, table);
        if (!found)
            return; // a table that does not exist is the business of ref.unresolved
        const std::string_view kind = cardNameAt(model, *found);
        if (kind == "TableDynamic5")
            return;
        const std::string term = std::string("tableTerms") + " with i = " + std::to_string(row + 1) +
                                 " and j = " + std::to_string(column + 1);
        const std::string speedTables = std::string("TableDynamic5");
        sink.reportCard(
            where, "tableTerms",
            "the term " + term + " names the table " + std::to_string(table) + ", which is a " + std::string(kind) +
                ": the terms of a " + std::string("NonlinearBushForceTables") + " are " + speedTables + " entries",
            "name a " + speedTables + " that gives the " + std::string("TableDynamic1") + " of each rotor speed");
    }
};

} // namespace

void addNonlinearBushForceTablesKindCheck(CheckRegistry& registry)
{
    registry.add(std::make_unique<NonlinearBushForceTablesKindCheck>());
}

} // namespace oor::analysis
