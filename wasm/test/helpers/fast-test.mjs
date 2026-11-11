import assert from "node:assert/strict";
import { test as nodeTest } from "node:test";

const MAX_TEST_MS = 1000;

// The runner timeout handles asynchronous stalls. The elapsed check also catches synchronous WASM work.
export function test(name, body) {
  return nodeTest(name, { timeout: MAX_TEST_MS }, async context => {
    const started = performance.now();
    await body(context);
    const elapsed = performance.now() - started;
    assert.ok(elapsed < MAX_TEST_MS, `${name} took ${elapsed.toFixed(1)} ms; limit ${MAX_TEST_MS} ms`);
  });
}
