# DOM Engine

## Goal
Build a custom, embeddable DOM and rendering engine in C++20 with strict systems-level boundaries.

## Structure
- `src/Node.h`, `src/Node.cpp`: DOM node model, ownership, dirty-state invalidation.
- `src/Renderer.h`, `src/Renderer.cpp`: terminal rendering boundary.
- `src/Diff.h`, `src/Diff.cpp`: reconciliation boundary.
- `src/main.cpp`: minimal integration entrypoint.
- `build/`: out-of-source build artifacts.

## Phase Plan (Systems Architect View)
1. Clear Phase
Define invariants, data ownership, and frame lifecycle before adding features.

2. Core DOM Phase
Stabilize tree mutations, re-parenting, and dirty-flag propagation with tests.

3. Layout Phase
Add deterministic layout passes that run only on dirty subtrees.

4. Diff Phase
Compute minimal mutations between desired and live state.

5. Renderer Phase
Translate computed layout into batched terminal buffer writes.

6. Optimization Phase
Use profiling and flame graphs to remove full-tree work and redundant updates.

## Invariants to Protect
- Parent owns children via `shared_ptr`.
- Child references parent via `weak_ptr`.
- No parent-child cycles.
- No-op mutations must not set dirty flags.
- `LAYOUT_DIRTY` is not downgraded by paint-only events in the same frame.
