# Custom DOM & Terminal Rendering Engine (C++20)

## 1) Project Intent
Build a high-performance, embeddable DOM + layout + renderer stack from first principles in modern C++.

This project is not a UI library exercise. It is a systems exercise:
- explicit ownership
- explicit state transitions
- explicit invalidation (dirty flags)
- bounded work per frame

## 2) Non-Negotiable Constraints
- Language: C++20
- Render target: terminal 2D character buffer
- Ownership model:
  - parent owns children (`std::shared_ptr<Node>`)
  - child references parent non-owning (`std::weak_ptr<Node>`)
- State model: mutable live DOM with dirty flags
  - `CLEAN`
  - `LAYOUT_DIRTY` (geometry changed)
  - `PAINT_DIRTY` (visual-only change)

## 3) Architectural Boundaries (Must Stay Strict)
- `Node`: data + tree topology + invalidation triggers only
- `LayoutEngine`: computes geometry from constraints/styles
- `Renderer`: consumes computed geometry and paints to buffer

Do not let `Node` perform layout computation or terminal I/O.
Do not let `Renderer` mutate DOM topology.

## 4) Direct Evaluation of Current Design
Current `Node` header is a good start and aligns with the ownership model.

What is correct:
- `weak_ptr` parent avoids cycle leaks.
- private geometry/state enforces encapsulation.
- explicit mutator methods (`set_position`, `set_size`) provide a single invalidation path.

What must be corrected/added:
1. `shared_from_this()` requires inheritance from `std::enable_shared_from_this<Node>`.
2. Re-parenting in `add_child` must be atomic in behavior:
   - detach from old parent
   - attach to new parent
   - update child parent link
3. No-op setters must not dirty the tree:
   - if `(new_x == x && new_y == y)`, return immediately.
4. Dirty semantics should be monotonic per frame:
   - once `LAYOUT_DIRTY`, do not downgrade to `PAINT_DIRTY`.

## 5) `shared_ptr` vs `weak_ptr` (Precise Reasoning)
### Why `shared_ptr` for children?
A child must stay alive as long as any owning parent/subsystem still needs it. Parent-child lifetime coupling is explicit and safe.

### Why `weak_ptr` for parent?
If both sides are `shared_ptr`, reference count never reaches zero in cycles.
`weak_ptr` breaks the ownership cycle while preserving navigability (`lock()` when needed).

### Common misunderstanding
"`weak_ptr` is faster and should replace `shared_ptr` everywhere" is wrong.
`weak_ptr` is not ownership. Use it only where non-owning relationship is intentional.

## 6) `add_child` Correct Algorithm
Preconditions:
- reject null child
- reject self-attach (`child.get() == this`)
- reject attaching ancestor as child (cycle prevention)

Behavior:
1. If child already under this parent, do nothing.
2. If child has old parent, remove from old parent's `children`.
3. Set `child->parent = shared_from_this()`.
4. Push child into `children`.
5. Mark current subtree `LAYOUT_DIRTY` (structure changed).

Implementation note:
- `shared_from_this()` is only valid if `this` is already owned by `std::shared_ptr<Node>`.
- Never create a fresh `shared_ptr<Node>(this)`; that causes double-delete bugs.

## 7) `set_position` Correct Algorithm
Only trigger invalidation on actual change.

Pseudo-flow:
1. Compare new and current position.
2. If equal: return.
3. Update `x`, `y`.
4. Mark node `LAYOUT_DIRTY`.
5. Optionally bubble an aggregate "subtree dirty" indicator upward for scheduling.

Why `LAYOUT_DIRTY` and not `PAINT_DIRTY`?
Position changes alter geometry relationships and can affect overlap, clipping, and parent scroll regions.

## 8) Dirty Propagation Rules (Recommended)
- Local mutation picks initial state (`PAINT` or `LAYOUT`).
- Upward propagation should be coarse and cheap:
  - parent tracks "has dirty descendants" bit or equivalent.
- During a frame:
  - process layout pass only on dirty subtrees
  - process paint pass only where layout/paint changed
- At frame end: `mark_clean()` only after successful layout + paint application

## 9) What the Flame Likely Showed
If you observed a flame graph during updates, the likely dominant costs in early versions are:
1. full-tree traversal every tick (instead of dirty-subtree traversal)
2. repeated vector scans in child removal/re-parenting (`O(n)` per operation)
3. excessive string-based child lookup/removal by `id`
4. repeated dirty propagation even on no-op setters
5. refcount churn from unnecessary `shared_ptr` copies

Actionable interpretation:
- your biggest win is not micro-optimizing render characters first
- your biggest win is reducing work scope with strict invalidation and traversal pruning

## 10) Immediate Implementation Checklist
1. Make `Node` inherit `std::enable_shared_from_this<Node>`.
2. Implement cycle-safe `add_child` with re-parenting.
3. Implement no-op-guarded `set_position`.
4. Add invariant asserts in debug builds:
   - parent-child consistency
   - no self/ancestor cycles
5. Add microbench:
   - 10k nodes, random 1% position updates/frame
   - report layout nodes visited vs total nodes

## 11) Candid Mentor Note
Your architectural direction is correct, but correctness and bounded work are still fragile until invariants and invalidation rules are enforced in code, not in intent. Build guardrails now, before adding style/layout complexity.
