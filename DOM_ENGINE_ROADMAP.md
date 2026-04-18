# DOM Engine — Developer Roadmap
**Language:** C++20 | **Render target:** Terminal 2D character buffer | **Audience:** Any AI or team picking this up cold

---

## 1. Scope and Goals

### 1.1 Core Capabilities

| Capability | Description | Priority |
|---|---|---|
| Node model | Typed tree nodes (Element, Text, Document) with explicit ownership | P0 |
| DOM API surface | `createElement`, `appendChild`, `removeChild`, `setAttribute`, `querySelector` | P0 |
| Dirty-flag invalidation | `CLEAN → LAYOUT_DIRTY → PAINT_DIRTY` state machine, monotonic per frame | P0 |
| Layout engine | Constraint-based geometry pass, runs only on dirty subtrees | P1 |
| Paint / render loop | Terminal character-buffer writer, bounded work per frame | P1 |
| Event model | Capture / bubble phases, `dispatchEvent`, typed `EventListener` | P1 |
| Mutation observers | Asynchronous notification of subtree changes (like `MutationObserver` in browsers) | P2 |
| Adapter hooks | `onUpdate`, `onRender`, `onNotifyMutations` — plug-in points for external data models | P2 |
| Serialization | JSON / S-expression snapshot and restore of DOM state | P3 |

### 1.2 Required Adapters for Integration

Each use-case (chess dashboard, log viewer, form UI, etc.) connects to the engine through an **Adapter** interface, never by reaching into Node internals.

```
ChessBoardAdapter   → polls or receives game state → mutates DOM nodes
LogStreamAdapter    → appends text nodes on log events
FormAdapter         → binds attribute changes to validation callbacks
```

Every adapter satisfies the same three-hook contract (§4 API Surface) so they are interchangeable and testable in isolation.

### 1.3 Non-Goals (Scope Guards)

These are explicitly out of scope to prevent drift:

- **GPU or pixel rendering** — terminal buffer only; no OpenGL, Vulkan, or framebuffer abstractions.
- **CSS parsing** — style is expressed as typed struct fields, not parsed CSS strings.
- **JavaScript / scripting runtime** — no embedded interpreter; logic lives in C++ adapters.
- **Networked / multi-process DOM** — single-process only; no IPC or shared-memory DOM.
- **Full HTML5 spec compliance** — this is an embedded engine, not a browser.
- **Built-in font rasterization** — terminal character semantics only.

---

## 2. Architecture Overview

### 2.1 Module Map

```
┌─────────────────────────────────────────────────────────────┐
│                        Adapter Layer                        │
│   ChessBoardAdapter │ LogStreamAdapter │ FormAdapter │ ...   │
└────────────────┬────────────────────────────────────────────┘
                 │  mutates via public DOM API only
┌────────────────▼────────────────────────────────────────────┐
│                      Document (root)                        │
│  createElement / appendChild / removeChild / querySelector  │
│  dispatchEvent / MutationObserver registry                  │
└────┬──────────────────────┬────────────────────────────────┘
     │ owns (shared_ptr)    │ owns
┌────▼──────┐         ┌─────▼──────┐
│  Element  │◄──weak──│  TextNode  │
│  Node     │  parent │  Node      │
└────┬──────┘         └────────────┘
     │
┌────▼──────────────────────────────────────────────────────┐
│                    LayoutEngine                           │
│  geometry pass — dirty subtree traversal only            │
│  output: {x, y, width, height} written back to nodes    │
└────┬──────────────────────────────────────────────────────┘
     │
┌────▼──────────────────────────────────────────────────────┐
│                     Renderer                              │
│  reads computed geometry + style, writes to CharBuffer   │
│  never mutates DOM topology                              │
└────┬──────────────────────────────────────────────────────┘
     │
┌────▼──────────────────────────────────────────────────────┐
│                    CharBuffer                             │
│  2D array of Cell{char, fg, bg, attrs}                   │
│  double-buffered; diff + emit ANSI escape sequences      │
└───────────────────────────────────────────────────────────┘
```

Additional cross-cutting modules:

| Module | Role |
|---|---|
| `EventSystem` | Routes events through capture → target → bubble phases |
| `MutationObserver` | Queues `MutationRecord` batches; notified post-layout |
| `DirtyTracker` | Singleton frame scheduler; owns the set of dirty root subtrees |
| `Serializer` | Snapshot and restore; decoupled from Node internals via visitor |
| `Diff` | Reconciles a desired tree against the live tree with minimal mutations |

### 2.2 Design Patterns

**Adapter Pattern** — use-case plug-ins implement `IAdapter`:
```cpp
struct IAdapter {
    virtual void onUpdate(Document& doc)        = 0;   // push mutations
    virtual void onRender(const CharBuffer& buf) = 0;  // receive rendered frame
    virtual void onNotifyMutations(
        std::span<const MutationRecord> records) = 0;  // observe changes
    virtual ~IAdapter() = default;
};
```
The engine calls these hooks in frame order. Adapters do not hold raw pointers to nodes longer than the call duration.

**Observer Pattern** — `MutationObserver` mirrors the browser API:
```cpp
observer.observe(node, {.childList = true, .attributes = true, .subtree = true});
```
Notifications are batched and delivered after the layout pass in each frame. This decouples the chess engine (which updates state at move time) from the render loop (which runs at display rate).

**PImpl / Opaque Handle for ABI Stability** — public headers expose only forward-declared handles:
```cpp
// dom_engine/public/Node.h  — stable ABI surface
struct NodeHandle;
using NodeRef = std::shared_ptr<NodeHandle>;
NodeRef create_element(std::string_view tag);
```
Internal `Node` implementation lives in `src/internal/`. Embedders link against the public header only. This means the engine can refactor its internals without breaking adapter binaries.

**Monotonic Dirty State** — state transitions are one-way per frame:
```
CLEAN ──► LAYOUT_DIRTY ──► (no downgrade to PAINT_DIRTY in same frame)
```
`PAINT_DIRTY` is only reachable from `CLEAN`. Any structural or geometric change escalates directly to `LAYOUT_DIRTY`.

---

## 3. MVP Plan

### Phase 1 — Core DOM Model (Week 1–2)
**Goal:** A correct, leak-free, cycle-safe tree with enforced invariants.

- [ ] `Node` inherits `enable_shared_from_this<Node>`
- [ ] `add_child` with re-parenting, self-cycle and ancestor-cycle guards
- [ ] `remove_child` — O(1) via index or iterator, not string scan
- [ ] `set_position` / `set_size` with no-op guards
- [ ] `DirtyState` enum + monotonic `mark_dirty()` / `mark_clean()`
- [ ] Debug-mode invariant assertions (parent-child consistency, no cycles)
- [ ] Microbench: 10k nodes, 1% random mutations per frame, report nodes visited vs total
- [ ] Unit tests: add, remove, re-parent, no-op setter, dirty propagation

**Exit criterion:** Microbench shows layout pass visits ≤ 2× the mutated node count (dirty subtree, not full tree).

---

### Phase 2 — Rendering Loop and Observer Hooks (Week 3–4)
**Goal:** End-to-end frame: mutate → layout → paint → display.

- [ ] `CharBuffer` — double-buffered 2D `Cell` array, diff + ANSI emit
- [ ] `LayoutEngine` — depth-first dirty-subtree pass writing `{x, y, w, h}` to nodes
- [ ] `Renderer` — reads geometry, writes to `CharBuffer`; no DOM mutation
- [ ] `FrameLoop` — fixed-rate tick: poll adapters → layout → paint → swap buffers
- [ ] `MutationObserver` — batch queue, delivered post-layout
- [ ] `IAdapter` interface + `ChessBoardAdapter` stub exercising all three hooks
- [ ] Integration test: dummy adapter mutates 100 nodes/frame at 60 fps equivalent, assert no full-tree traversal

**Exit criterion:** Frame loop runs at target rate with CPU time dominated by dirty-subtree work only.

---

### Phase 3 — Adapters and Serialization (Week 5–6)
**Goal:** Real use-case integrations and state persistence.

- [ ] `ChessBoardAdapter` — subscribes to move events, updates board node attributes
- [ ] `LogStreamAdapter` — appends `TextNode` children on new log lines, prunes old ones
- [ ] `Serializer` — visitor-based; produces and consumes JSON snapshots
- [ ] `Diff` engine — given desired tree, compute `MutationRecord` list for minimal live mutations
- [ ] `querySelector` / `querySelectorAll` — CSS-selector subset (tag, id, class, attribute)
- [ ] Serialization round-trip tests

**Exit criterion:** Chess dashboard renders a full game replay from a PGN file via the adapter with no manual DOM manipulation in `main.cpp`.

---

### Phase 4 — Performance and Concurrency (Week 7–8)
**Goal:** Sustained high-frequency updates (target: ≤ 50 µs per layout pass on 10k node trees).

- [ ] Profile with perf / flamegraph; identify top-3 hot paths
- [ ] Evaluate `std::vector` vs flat pool allocator for `children`; benchmark both
- [ ] `DirtyTracker` — maintain a set of dirty roots, skip unrelated subtrees entirely
- [ ] Optional: worker thread for adapter `onUpdate` calls with mutex-guarded mutation queue
- [ ] `shared_ptr` refcount audit — eliminate unnecessary copies in hot paths (use `const&` passing)
- [ ] SIMD or ANSI batch-emit optimisation in `CharBuffer` diff

**Concurrency model:** The core DOM is single-threaded. Adapters may produce mutations on worker threads, but mutations are enqueued and applied on the main thread at frame start. No lock inside layout or paint passes.

---

### Phase 5 — Testing, CI, Documentation (Week 9–10)
**Goal:** The project is handoff-ready.

- [ ] Unit test suite (Google Test or Catch2) covering all invariants
- [ ] Property-based test: random tree mutations must never violate parent-child consistency
- [ ] Benchmark suite integrated into CI with regression threshold (±10% on layout microbench)
- [ ] CMake `CTest` integration; CI via GitHub Actions (Ubuntu + macOS)
- [ ] Doxygen comments on all public headers
- [ ] `CONTRIBUTING.md` explaining dirty-state rules and adapter contract
- [ ] `examples/chess_dashboard.cpp` — fully working demo

---

## 4. API Surface Sketch

### 4.1 Core Types

```cpp
// Dirty state — monotonic per frame
enum class DirtyState { CLEAN, PAINT_DIRTY, LAYOUT_DIRTY };

// Mutation record — mirrors browser MutationRecord
struct MutationRecord {
    enum class Type { ChildList, Attributes, CharacterData };
    Type            type;
    NodeRef         target;
    std::string     attribute_name;   // for Attributes type
    std::string     old_value;
    std::vector<NodeRef> added_nodes;
    std::vector<NodeRef> removed_nodes;
};

// Event — dispatched through capture/bubble chain
struct Event {
    std::string     type;             // "click", "keydown", "chess:move", etc.
    NodeRef         target;
    bool            bubbles     = true;
    bool            cancelable  = true;
    bool            propagation_stopped = false;
    void stop_propagation() { propagation_stopped = true; }
};
```

### 4.2 Node and Document

```cpp
class Node : public std::enable_shared_from_this<Node> {
public:
    // Tree mutation — all enforce invariants internally
    void add_child(std::shared_ptr<Node> child);
    void remove_child(std::shared_ptr<Node> child);
    void insert_before(std::shared_ptr<Node> child,
                       std::shared_ptr<Node> reference);

    // Geometry — no-op guarded, dirty-state aware
    void set_position(int x, int y);
    void set_size(int w, int h);

    // Attribute access
    void        set_attribute(std::string_view key, std::string_view value);
    std::string get_attribute(std::string_view key) const;
    bool        has_attribute(std::string_view key) const;
    void        remove_attribute(std::string_view key);

    // Dirty state
    DirtyState  dirty_state() const;
    void        mark_dirty(DirtyState s);   // only escalates, never degrades
    void        mark_clean();               // called by engine after full frame

    // Tree navigation
    std::shared_ptr<Node>              parent()    const;
    std::span<std::shared_ptr<Node>>   children()  const;
    std::shared_ptr<Node>              first_child() const;
    std::shared_ptr<Node>              next_sibling() const;

    // Identity
    std::string_view id()   const;
    std::string_view tag()  const;

    // Query
    std::shared_ptr<Node> query_selector(std::string_view selector);
    std::vector<std::shared_ptr<Node>> query_selector_all(
                                           std::string_view selector);

protected:
    explicit Node(std::string tag);

private:
    // --- geometry (layout engine writes these) ---
    int x_{0}, y_{0}, w_{0}, h_{0};

    // --- ownership ---
    std::vector<std::shared_ptr<Node>> children_;
    std::weak_ptr<Node>                parent_;

    // --- invalidation ---
    DirtyState state_{DirtyState::CLEAN};

    // --- attributes ---
    std::unordered_map<std::string, std::string> attributes_;

    friend class LayoutEngine;   // may read/write geometry fields
    friend class Renderer;       // may read geometry fields (const)
};

class Document {
public:
    std::shared_ptr<Node> create_element(std::string_view tag);
    std::shared_ptr<Node> create_text_node(std::string_view content);
    std::shared_ptr<Node> get_root() const;

    void dispatch_event(std::shared_ptr<Node> target, Event event);

    void add_mutation_observer(std::shared_ptr<Node>   target,
                               MutationObserverOptions opts,
                               MutationCallback        callback);
};
```

### 4.3 Adapter Hooks

```cpp
struct IAdapter {
    // Called at frame start: push mutations into `doc`
    virtual void on_update(Document& doc, std::chrono::nanoseconds dt) = 0;

    // Called after paint: receive the rendered buffer for display/output
    virtual void on_render(const CharBuffer& buf) = 0;

    // Called post-layout: receive batched mutation records
    virtual void on_notify_mutations(
        std::span<const MutationRecord> records) = 0;

    virtual ~IAdapter() = default;
};
```

### 4.4 Example: Minimal Chess Adapter

```cpp
class ChessBoardAdapter : public IAdapter {
    GameState& game_;

public:
    explicit ChessBoardAdapter(GameState& g) : game_(g) {}

    void on_update(Document& doc, std::chrono::nanoseconds) override {
        if (!game_.has_pending_move()) return;
        auto move = game_.consume_move();

        // Find the affected square nodes by id ("sq-e2", "sq-e4")
        auto from = doc.get_root()->query_selector("#sq-" + move.from);
        auto to   = doc.get_root()->query_selector("#sq-" + move.to);
        if (!from || !to) return;

        from->set_attribute("piece", "");
        to->set_attribute("piece", move.piece);
        // Only these two nodes become PAINT_DIRTY; layout is untouched
    }

    void on_render(const CharBuffer&) override {}  // terminal handles output

    void on_notify_mutations(std::span<const MutationRecord>) override {}
};
```

---

## 5. Data Structures and Performance Goals

### 5.1 Memory Layout Guidelines

**Node children — `std::vector<shared_ptr<Node>>`**
Correct default. Contiguous pointer array; iteration is cache-friendly.
Reserve initial capacity (e.g., 8) in `Node` constructor to avoid early reallocations.
Do not use `std::list` — pointer-chasing destroys prefetcher performance.

**Attribute map — `std::unordered_map<std::string, std::string>`**
Adequate for typical attribute counts (<20). For nodes with very hot attribute access in inner loops, consider a sorted `std::vector<std::pair<std::string_view, std::string_view>>` with binary search.

**CharBuffer cells — flat array**
```cpp
struct Cell {
    char32_t ch   = ' ';
    uint8_t  fg   = 7;    // ANSI color index
    uint8_t  bg   = 0;
    uint8_t  attrs = 0;  // bold, underline, etc.
};
// Stored as Cell[height * width], row-major — cache-friendly line scans
```
Double-buffer: `front` is what's on screen, `back` is what the renderer writes.
Diff emits ANSI escape sequences only for changed cells.

**DirtyTracker — `std::unordered_set<Node*>` of dirty roots**
Raw pointer here is safe: set is cleared every frame, nodes are owned by the tree.
O(1) insert/lookup; avoids walking the whole tree to find dirty nodes.

### 5.2 Reference Management

| Relationship | Type | Rationale |
|---|---|---|
| Parent → child | `shared_ptr` | Parent controls child lifetime |
| Child → parent | `weak_ptr` | Breaks ownership cycle; `lock()` when needed |
| Adapter → node (transient) | `const NodeRef&` parameter | Never stored; only valid during the call |
| LayoutEngine → node | `Node*` (raw, non-owning) | Layout pass holds tree lock; lifetime guaranteed |
| DirtyTracker → dirty roots | `Node*` (raw) | Frame-scoped; cleared before tree can mutate |

**Rule:** Never store a raw `Node*` or `weak_ptr<Node>` in adapter state across frame boundaries. Re-acquire via `query_selector` each frame, or store the node `id` string and look up.

### 5.3 Cache-Friendly Traversal

Layout traversal is depth-first, pre-order. This is inherently pointer-chasing (each child is a `shared_ptr` allocation), but the DirtyTracker prunes the search space significantly.

For future optimisation, consider a "flat node array" in `Document` where nodes are allocated from a pool in DFS insertion order. This packs frequently co-traversed nodes into adjacent memory addresses.

### 5.4 Concurrency Model

```
Main Thread
│
├── FrameLoop::tick()
│     ├── [mutex acquire] dequeue pending mutations from adapter worker threads
│     ├── adapter.on_update(doc)    ← single-threaded DOM mutation
│     ├── LayoutEngine::run(doc)    ← reads/writes Node geometry
│     ├── Renderer::paint(doc, buf) ← reads Node geometry + styles
│     ├── CharBuffer::diff_emit()   ← writes ANSI to stdout
│     ├── MutationObserver::flush() ← delivers MutationRecord batches
│     └── adapter.on_notify_mutations(records)
│
Worker Threads (optional)
└── produce mutations into a thread-safe queue
    (std::queue + std::mutex, or lock-free SPSC ring if profiling shows contention)
```

**Invariant:** `LayoutEngine` and `Renderer` run on the main thread with no concurrent DOM mutation. This eliminates data races without per-node locking, keeping the hot path lock-free.

### 5.5 Performance Targets

| Metric | Target |
|---|---|
| Layout pass, 10k nodes, 1% dirty | ≤ 50 µs |
| Layout pass, 10k nodes, 100% dirty | ≤ 2 ms |
| CharBuffer diff + ANSI emit, 80×24 terminal | ≤ 500 µs |
| add_child / remove_child | O(1) amortised |
| query_selector (id lookup) | O(1) via Document id-index map |
| query_selector (tag/class) | O(dirty subtree size) |

---

## 6. Deliverables and Artifacts

### 6.1 Directory Layout

```
dom-engine/
├── CMakeLists.txt              # top-level build
├── cmake/
│   └── DOMEngineConfig.cmake   # install/export config
├── include/
│   └── dom_engine/             # stable public ABI headers
│       ├── Node.h
│       ├── Document.h
│       ├── Event.h
│       ├── MutationObserver.h
│       ├── IAdapter.h
│       └── CharBuffer.h
├── src/
│   ├── internal/               # implementation — not part of ABI
│   │   ├── Node.cpp
│   │   ├── LayoutEngine.h
│   │   ├── LayoutEngine.cpp
│   │   ├── Renderer.h
│   │   ├── Renderer.cpp
│   │   ├── CharBuffer.cpp
│   │   ├── DirtyTracker.h
│   │   ├── EventSystem.cpp
│   │   └── MutationObserver.cpp
│   ├── Diff.h
│   ├── Diff.cpp
│   ├── Serializer.h
│   ├── Serializer.cpp
│   └── main.cpp                # minimal integration entrypoint
├── adapters/
│   ├── ChessBoardAdapter.h
│   ├── ChessBoardAdapter.cpp
│   └── LogStreamAdapter.h
├── tests/
│   ├── test_node.cpp
│   ├── test_dirty_propagation.cpp
│   ├── test_layout_engine.cpp
│   ├── test_serializer.cpp
│   └── bench_layout.cpp        # microbench: 10k nodes, 1% mutations
├── examples/
│   └── chess_dashboard.cpp
├── docs/
│   └── Doxyfile
└── .github/
    └── workflows/
        └── ci.yml
```

### 6.2 CMake Build System

```cmake
# CMakeLists.txt (root)
cmake_minimum_required(VERSION 3.22)
project(DOMEngine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Core library
add_library(dom_engine STATIC
    src/internal/Node.cpp
    src/internal/LayoutEngine.cpp
    src/internal/Renderer.cpp
    src/internal/CharBuffer.cpp
    src/internal/EventSystem.cpp
    src/internal/MutationObserver.cpp
    src/Diff.cpp
    src/Serializer.cpp
)
target_include_directories(dom_engine
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)
target_compile_options(dom_engine PRIVATE
    -Wall -Wextra -Wpedantic
    $<$<CONFIG:Debug>:-fsanitize=address,undefined>
    $<$<CONFIG:Debug>:-fno-omit-frame-pointer>
)
target_link_options(dom_engine PRIVATE
    $<$<CONFIG:Debug>:-fsanitize=address,undefined>
)

# Tests
enable_testing()
find_package(Catch2 3 REQUIRED)
add_executable(dom_engine_tests
    tests/test_node.cpp
    tests/test_dirty_propagation.cpp
    tests/test_layout_engine.cpp
    tests/test_serializer.cpp
)
target_link_libraries(dom_engine_tests PRIVATE dom_engine Catch2::Catch2WithMain)
include(CTest)
include(Catch)
catch_discover_tests(dom_engine_tests)

# Benchmark
add_executable(bench_layout tests/bench_layout.cpp)
target_link_libraries(bench_layout PRIVATE dom_engine)

# Example
add_executable(chess_dashboard examples/chess_dashboard.cpp)
target_link_libraries(chess_dashboard PRIVATE dom_engine)
```

Build commands:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
./build/bench_layout
```

### 6.3 Target Platforms

| Platform | Status | Notes |
|---|---|---|
| Linux (x86_64) | Primary | CI target; ANSI terminal assumed |
| macOS (Apple Silicon) | Supported | ANSI terminal via Terminal.app / iTerm2 |
| Windows | Stretch | Requires VT100 mode (`SetConsoleMode`) |

### 6.4 Key Design Decision Rationale

**Why `enable_shared_from_this` instead of raw `this` in `add_child`?**
`add_child` must set `child->parent = <owning pointer to this>`. Without `enable_shared_from_this`, the only way to get a `shared_ptr` to `this` is to construct a new one — which creates a second independent reference count and causes a double-delete. `shared_from_this()` retrieves the existing owning pointer safely.

**Why monotonic dirty escalation?**
If a node is `LAYOUT_DIRTY` (geometry changed) and then receives a paint-only update in the same frame, downgrading to `PAINT_DIRTY` would cause the layout pass to skip it — producing stale geometry. Monotonic escalation is the minimal rule that prevents this class of bug with zero runtime cost.

**Why batch MutationObserver delivery post-layout?**
Delivering mid-layout would allow observers to mutate the tree while layout is walking it — a classic re-entrancy hazard. Post-layout delivery matches browser semantics and is safe without any additional locking.

**Why PImpl / opaque handles on public headers?**
Adapters are compiled separately and may be distributed as plugins. Exposing `Node` internals in public headers means any change to `children_` type or dirty-flag representation forces a recompile of all adapters. Opaque handles isolate that churn.

**Why `std::vector` for children over `std::list`?**
Layout traversal iterates children in order repeatedly. `std::vector` packs child pointers contiguously; `std::list` scatters them. Even though each pointer dereferences to a heap allocation, the pointer array itself being contiguous is enough to improve prefetcher hit rate during the layout walk.

---

## Appendix A: Invariants Quick Reference

Any code change must preserve these. Add `assert()` checks in `DEBUG` builds.

1. `node->parent().lock()->children()` contains `node` — or parent is null.
2. No node appears twice in the same `children` vector.
3. No node is its own ancestor (no cycles).
4. `dirty_state()` only transitions `CLEAN → PAINT_DIRTY`, `CLEAN → LAYOUT_DIRTY`, or `PAINT_DIRTY → LAYOUT_DIRTY` within a frame. Never the reverse.
5. `mark_clean()` is called only by the frame loop after a successful layout + paint.
6. `LayoutEngine` is the only writer of `{x_, y_, w_, h_}` on `Node`.
7. `Renderer` never calls any mutating method on `Node`.
8. `shared_from_this()` is never called inside a `Node` constructor (object not yet managed).

---

## Appendix B: Common Bugs and Fixes

| Symptom | Root Cause | Fix |
|---|---|---|
| Double-free / heap corruption on tree teardown | `shared_ptr<Node>(this)` in `add_child` | Use `shared_from_this()` |
| Full tree re-layout every frame | Dirty set not consulted; `LayoutEngine` traverses all nodes | Feed `DirtyTracker` root set to layout pass |
| Refcount never reaches zero | Parent and child both hold `shared_ptr` to each other | Child → parent must be `weak_ptr` |
| Redundant dirty propagation on no-op | `set_position` doesn't compare before marking dirty | Guard: `if (new_x == x_ && new_y == y_) return;` |
| Observer fires mid-layout causing re-entrancy | `MutationObserver::flush()` called inside layout pass | Move flush to post-paint step in frame loop |
| O(n) child removal | `remove_child` scans `children_` by pointer equality | Keep an index map or use `std::find` + swap-and-pop |
