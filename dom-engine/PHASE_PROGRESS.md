# DOM Engine Phase Progress

## Phase 1 - Core DOM Model
Status: Complete

Verified outcomes:
- Tree ownership model implemented (`shared_ptr` children, `weak_ptr` parent).
- Cycle-safe `add_child`, safe `remove_child`, and invariant checks.
- Dirty-state monotonic escalation and subtree dirty propagation.

## Phase 2 - Rendering Loop and Observer Hooks
Status: Complete

Verified outcomes:
- Frame pipeline working: mutate -> layout -> collect mutations -> render -> swap -> notify.
- Adapter hooks wired and integration-tested.
- Dirty-subtree pruning implemented for layout and renderer.

## Phase 3 - Query, Serializer, Diff, Adapter Replay
Status: Complete

Verified outcomes:
- Selector support: tag, id, attribute presence, attribute value.
- JSON serializer/deserializer round-trip checks.
- Diff engine id-based reconciliation + mutation application.
- Chess adapter move replay and mutation logging.

## Phase 4 - Performance and Concurrency
Status: Complete

Verified benchmark numbers (10k nodes, 1% mutations):
- `visit_ratio=0.0651`
- `layout_visited_nodes=651`
- `render_us=127`
- `layout_us=211`
- `collect_mutations_us=323`

## Phase 5 - Testing, CI, and Documentation
Status: Complete

Verified outcomes:
- Coverage and stress tests added and passing.
- CTest labels added (`unit`, `stress`, `bench`).
- CI workflow added for build + unit + stress.
- Public API headers documented with Doxygen comments.
- Contributor guide added.

## Demo
A runnable embedded-library integration demo is available:
- Source: `examples/chess_pgn_demo.cpp`
- Target: `chess_demo`

### Build
```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++
cmake --build build -j4
```

### Run
```powershell
./build/chess_demo.exe
```

What it does:
- Builds a full 8x8 chess board with `Document::create_element()`.
- Replays 10 hard-coded PGN-style moves through `ChessBoardAdapter` + `FrameRunner` at 16ms/frame.
- Validates final board position with assertions.
- Emits ANSI terminal output for the final board state with piece and square colors.
