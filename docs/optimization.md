# Preparing and applying an optimization design

Reader supplies the model and design declarations to a JavaScript-driven optimization. Solver computes the analyses,
response derivatives, Jacobian products and supported Hessian products. See the
[Solver optimization guide](https://jcdulas.github.io/O_o_Solver/docs/optimization.html) for the complete iteration API,
matrix conventions, callbacks and derivative limits.

## Inspect variables and export declarations

```javascript
const design = oor.designVariables(deck);
console.log(design.variables, design.problems, design.notApplied);

// Independent owned buffers; safe to transfer to another module or worker.
const tables = oor.optimizationTables(deck);
```

`designVariables` returns ID, label, initial value, bounds, move limit and dependency flag for every variable.
`problems` lists declarations that prevent a valid design point. `notApplied` lists declarations that the Reader
cannot write, with their card type, ID and reason.

`optimizationTables` exports the typed design variables, links, relations, constants, shape terms, compiled equations,
responses, response arguments, constraints, constraint unions, subcase selections and screening rules. Each value is
the corresponding query's table package. No pointer into Reader memory is given to Solver.

The model and package formats are unchanged. Existing named queries still expose their complete design metadata;
use `runQuery` and `queryCatalogue` when an application needs a particular table or selection.

## Connect the solver

```javascript
const problem = solver.optimizationFromReader(oor, deck, {
  responses: [{ id: 101, type: "Volume", label: "volume" }],
  objective: { responseId: 101 },
});
const info = problem.metadata();
const point = problem.evaluate(info.initialValues);
try {
  console.log(point.values(), point.gradients());
} finally {
  point.delete();
  problem.delete();
}
```

The application imports both built modules. The deck must define the variables and relations used by this example.
The solver owns an immutable snapshot: trial evaluations never modify the Reader model. Edits to the deck require
rebuilding the solver problem.

## Apply the accepted point

```javascript
const ids = Array.from(info.independentIndices, i => info.variables[i].id);
const update = oor.applyDesignPoint(deck, ids, acceptedX);
console.log(update.changedFields, update.movedGrids);
const text = deck.toModelText();
deck.writeArchive("/work/accepted-model.ooh5"); // save the accepted model as native columns
```

The IDs are explicit because Solver's vector contains independent variables only, whereas Reader lists all variables.
IDs must be distinct, defined, independent positive integers; values must be finite and within variable bounds.
Omitted independent variables keep their initial value. Linked variables are computed from their links.

The operation reads the current design, evaluates relations and supported shape moves, then writes the point atomically.
A validation failure leaves the model unchanged. It refuses unsupported declarations and field limit violations by
default. Optional `limits: "clip" | "ignore"` changes the field limit policy; variable bounds remain checked.

`setInitialValues` defaults to true: the design variable cards are saved at the accepted point. Shape initial values
and grid coordinates remain consistent, so applying the same point again does not repeat a shape move.
The report includes each field's before, requested and after values, clipping status and changed flag.

To save the accepted model with its result tables, attach it to the result set and call `results.writeArchive`.
An archive reopened through `results.model()` shares its deck with subsequent archive writes. See
[model/results archives](wasm-api.md#a-model-and-results-in-one-archive) for ownership and loading behavior.

Declarations added only to the solver from JavaScript do not automatically create Reader cards. Add their declarations
to the model before using `applyDesignPoint`, or write the final fields through the existing card-editing interface.
Reader evaluates and writes design relations; derivatives of structural responses belong to Solver.
