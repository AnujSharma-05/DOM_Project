# Contributing

## Dirty-State Rules

The engine uses monotonic dirty escalation per node:

- `CLEAN`: no pending visual/layout updates.
- `PAINT_DIRTY`: attributes/styles changed; visual repaint needed.
- `LAYOUT_DIRTY`: geometry/topology changed; layout recomputation needed.

Rules:

- Escalation is monotonic inside a frame (`CLEAN -> PAINT_DIRTY -> LAYOUT_DIRTY`).
- A lower-priority mark must not downgrade an already higher-priority state.
- `mark_clean()` is called after a frame is fully processed.
- Parent nodes track `has_dirty_descendant()` so layout/render can prune clean subtrees.

## Adapter Contract

Adapters implement `IAdapter` and are called in frame order:

1. `on_update(Node&, dt)`:
   - Allowed: mutate DOM (`set_attribute`, `set_position`, add/remove children).
   - Intended for game/UI input and state transitions.
2. `on_render(const CharBuffer&)`:
   - Allowed: inspect/render output, logging, telemetry.
   - Not allowed: mutate DOM topology.
3. `on_notify_mutations(span<MutationRecord>)`:
   - Allowed: consume batched mutation info, update adapter-side caches.
   - Prefer read-only behavior for DOM here.

## Core Invariants (`debug_validate_subtree`)

In debug builds, every subtree should satisfy:

- No null child pointers.
- Every child has `parent == this`.
- No duplicate child pointer entries in the same child list.
- No node can be its own child.
- No ancestor cycles.

When adding new tree mutations, run tests in debug mode and keep these invariants green.

## Running Locally

From `dom-engine`:

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++
cmake --build build -j4
ctest --test-dir build --output-on-failure
```

Run test groups by label:

```powershell
ctest --test-dir build -L unit --output-on-failure
ctest --test-dir build -L stress --output-on-failure
ctest --test-dir build -L bench --output-on-failure
```

Run benchmark binary directly:

```powershell
./build/bench_layout.exe
```
