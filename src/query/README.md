# `query`

Query plans, operators, query tables, Arrow export, apply, named queries.

- Headers: `include/oor/query/`. Sources: this folder. Tests: `tests/cpp/query/`.
- May use: `analysis`, `results`, `model`, `deck`, `io`, `base` (checked by `tools/check_layering.py`).
- Start reading at `plan/Query.hpp` (the builder and the executor), then `plan/QuerySource.hpp` (what a source and a join
  target give) and `table/QueryTable.hpp` (the result).
- `table/`: `QueryColumn`, `QueryTable`, `ArrowExport` (the Arrow C Data Interface, no copy), `CsvExport`.
- `operators/`: `Expression` and `ExpressionEvaluator`, `SortOperator`, `GroupOperator`, `ColumnOps`, `ExecutionContext`.
- `plan/`: `Query`, `QueryPlan`, `QueryFrame` (private), `TableSource`.
- `providers/`: the columns of the cards of a model: `StoredColumnCatalogue`, `CardColumns`, `ComputedColumns`,
  `ModelSource`.
- `apply/`: `CellValue`, `ColumnEdits`, `ChangeTracker`, `EditableTable`, `QueryApply` (edits of a result and `apply`).
- `named/`: the named queries: `NamedQuery`/`QueryBase`, `QueryInfo` (the catalogue entry), `QueryValue` (typed
  arguments), `NamedQueryRegistry` (finds, checks and runs a query, applies `columns`/`sort`/`limit`), `TableBuilder`, `common/` (helpers shared
  by the queries), and one folder per family under `topology/` (`t1_inventory` ... `t13_comparison`), one `.cpp` for each query. The components, the
  boundaries and the revision stamps they use are in `src/analysis/components/`.
