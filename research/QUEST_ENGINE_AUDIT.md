# Quest Engine — Audit & Hardening Report

Scope: the portable JSON quest-engine core (`src/core/`), its JSON Schema
(`config/schema/quest.core.schema.json`), and the headless CLI conformance
harness (`tools/cli_harness/`). Goal: three-way audit (SPEC ↔ implementation ↔
schema), fix correctness bugs, add validation + clear errors, expand test
coverage. Sections 1–4 describe the historical `feature/court-wizard` audit;
that runtime is not present on current main. The standalone 2026-08-24 PRF
work is recorded separately in §5.

The SPEC (`QUEST_ENGINE_SPEC.md`) is the source of truth. Comments in
`src/core/` are English to match the existing style.

---

## 1. Three-way audit findings

### A. Correctness bugs (all FIXED)

| # | Severity | Finding | Fix |
|---|----------|---------|-----|
| A1 | **Crash** | Malformed actions/conditions crash the host. Every parameter read used `operator[]` / unchecked `get<T>()`, which **throws or `assert`-aborts** (SIGABRT) on a missing key or a type mismatch. Reproduced: `set_var` missing `var`, `add_var` with a string `value`, `var_gte` missing `value`, `schedule` missing `at`/`after_hours`, `set_objective_active` given an object. Violates SPEC §8 ("a problem reached at runtime MUST be logged and the current flow aborted safely, never crash"). | All runtime reads now go through safe accessors (`jsonAt` / `tryGetString` / `tryGetNumber` / `tryGetIdString`) that log a precise message and skip the action / return `false` for a condition. No input shape crashes anymore (verified by a degenerate-input fuzz set). |
| A2 | **Crash** | A non-object top-level document (e.g. a JSON array or scalar) crashed via `json::value("id", …)` (`type_error.306`) both in the core log paths and in the harness title print. | Core: new safe `questId()` helper tolerates a non-object doc; `validate()` reports "quest document is not a JSON object". Harness: guards the title print. |
| A3 | **Spec violation** | **A terminated quest still responded to triggers.** After `complete_quest`/`fail_quest`, `dispatchEvent`/`checkTimers` still ran matching triggers — directly violating SPEC §3.1.3 ("MUST stop responding to its triggers"). The on-chain stop (within one action list) worked, but cross-event triggers did not. | `fireTriggers` and `checkTimers` short-circuit when `st_.terminated`. `reset_quest` clears `terminated` *before* re-running, so repeatable-quest loops still work. |
| A4 | Robustness | `triggerMatches` matched only a **hardcoded** filter-key list (`dialogue/objective/character/form/location/key`). An adapter event filtered by any other key (e.g. Godot `node`, an `item` key) would be silently ignored — the trigger would match regardless of that key. This is exactly the kind of smuggled, engine-specific assumption the CLI harness exists to catch (SPEC appendix A.4). | Now **generic**: any trigger key other than the structural `on`/`when`/`do` is treated as a filter that MUST equal the event's value. Core stays adapter-agnostic; the demo's existing keys still match. |
| A5 | Robustness | `presentCurrentNode` did `l.get<std::string>()` on each `lines` entry and assumed `choices` is an array of objects; a non-string line or a non-object choice would throw. | Defensive: non-string lines are stringified, non-object choices are logged and skipped, non-array `lines`/`choices` are tolerated. |
| A6 | Robustness | `runActions`/`fireTriggers`/`applyInitialState` assumed `on_start`/`triggers`/`vars`/`objectives` were arrays/objects; a wrong shape (e.g. `on_start: "x"`) would throw or iterate unexpectedly. | All guarded with `is_array()`/`is_object()` checks that log and skip. |

### B. Validation gaps (FIXED — new `validate()` pass)

The SPEC §7 requires an implementation to validate each quest against the
effective schema and SHOULD point at the file + path of the offending
node/choice/action. There was **no validation pass at all** — problems only
surfaced as runtime crashes. Added `QuestEngine::validate()` (a new, additive
public method — see §4 for why this does not break the public-interface
contract). It collects **every** problem (does not stop at the first) with a
`[quest-id] json.path: message` format. It checks:

- top level: required `id`/`title`, integer `version`, enum `priority`;
- `vars` value types; `objectives` shape + valid `state` enum;
- **type mismatch** between a var's declared type and a `set_var`/`var_eq`/`var_neq` value (SPEC §8);
- `var_gte`/`var_lte`/`add_var` require a **number** operand;
- **undeclared var references** (quest var not in `vars`, or `global.<name>` not declared in the GlobalStore — SPEC §2.4/§7.6);
- **missing references**: `complete_objective`/`set_objective_active`/`fail_objective` → unknown objective; `start_dialogue` → unknown dialogue; choice `goto` / dialogue `entry` → missing node (SPEC §8);
- core-event filter sanity: `objective_completed`/`dialogue_end` triggers filtering on an unknown objective/dialogue;
- action well-formedness: exactly one verb key; `complete_quest`/`fail_quest`/`reset_quest` expect `true`; `show_message` is string-or-`{text}`; `schedule` needs `key` + (`at`|`after_hours`) and a wired Clock port (SPEC §5.7);
- condition well-formedness recursively (incl. nested `all`/`any`/`not`);
- node well-formedness: required `speaker`, and `choices` XOR `end:true`; choice required `text`, and `goto` XOR `end:true`;
- `random` remained flagged as **not implemented** in that audited runtime.
  The 2026-08-24 branch pins and tests the primitive only; it does not import or
  modify `QuestEngine::validate()`.

Adapter-extension verbs/conditions/events are deliberately **not** flagged — the
core does not understand them; per SPEC §4.4 they are checked by the merged
"effective schema". (The demo's `spawn_character` / `give_gold` / `spell_cast_on`
pass clean.)

The CLI harness runs `validate()` at load, prints all problems, and supports a
`--strict` flag (exit code 2, refuses to run a structurally invalid quest).

### C. Historical core-vocabulary coverage (§4.1–4.3, §4.5)

At the audit date, all vocabulary except the deliberately blocked `random`
primitive was present and verified:

- **Conditions (§4.1)**: `var_eq`/`var_neq` (type-aware via `std::variant`; a bool `true` correctly does **not** equal number `1`), `var_gte`/`var_lte`, `objective_state`, `all`/`any`/`not` (nested verified), multi-key AND. In the historical runtime, `random` is still a reserved false-returning stub.
- **Actions (§4.2)**: `set_var`/`add_var` (with `global.<name>` resolution), `set_objective_active`/`complete_objective` (emits `objective_completed`)/`fail_objective`, `complete_quest`/`fail_quest`, `start_dialogue`, `show_message` (string or `{text}`), `schedule` (re-schedule replaces; needs Clock), `reset_quest` (resets quest vars/objectives/dialogue/timers, re-runs `on_start`+`quest_start`, leaves globals untouched).
- **Triggers (§4.3)**: `quest_start`, `dialogue_end{dialogue}`, `objective_completed{objective}`, `timer{key}`; ordering = JSON array order.
- **§4.5 deliver_message/message_ack** (standard portable extension, SHOULD not core MUST): correctly handled — `deliver_message` routes to the ActionRunner (adapter renders it); `message_ack` is a normal event matched by the `key` filter. No core change needed; the schema comment already documents the boundary.
- **§2.4 globals**: `global.<name>` reads/writes resolve to the shared `GlobalStore`; survive `reset_quest`; undeclared references caught by `validate()`.

### D. Schema accuracy (UPDATED)

The schema already matched the core vocabulary closely. Tightened/clarified:

- `varValue`/`varNumber`: added `$comment`s documenting the `global.<name>` prefix, same-type comparison, and that a var/value **type mismatch is a validation error** (SPEC §8); added `minLength:1` on `var`.
- `random`: the 2026-08-24 schema comment now points at appendix B and explicit
  `key` has `minLength:1`, while warning that current main lacks runtime wiring.

No schema rule contradicts what the engine accepts. (The schema is intentionally
stricter than the tolerant runtime: `oneOf` choices/end, required `speaker`,
etc. — that is by design; the runtime degrades gracefully while the schema +
`validate()` are the gatekeepers.)

---

## 2. Tests added

The historical `feature/court-wizard` line added test quests under
`config/quests/` and a CLI conformance script. Those broad fixtures are not
copied into the focused 2026-08-24 branch.

`tools/quest_prf_test.cpp` contains `21` focused primitive assertions covering the
standard SHA-256 `abc` vector, two full
canonical-input golden vectors (including UTF-8), stable repeats, changes to
each framed field, chance boundaries/strict threshold behavior, and pure
reconstruction equivalence with a restored seed. It does not claim evaluator,
JSON-path plumbing, or persistence integration coverage.

| Quest | Exercises |
|-------|-----------|
| `test_timers.json` | schedule, due-at-exactly-now, multiple-due, **re-schedule replaces** old due time |
| `test_conditions.json` | nested `all`/`any`/`not`, all `var_*`, `objective_state`, global read, **type-aware equality** (bool≠number) |
| `test_dialogue.json` | `when`-gated choice (hidden then shown), `goto` chain, `end`, **cancel** (EOF/non-numeric), `objective_completed` + `dialogue_end` triggers |
| `test_reset.json` | `reset_quest` loop: quest vars reset to initial, **globals survive** and count across loops |
| `test_termination.json` | `complete_quest` stops the action chain **and** all subsequent triggers/timers (SPEC §3.1.3 regression) |
| `test_async_message.json` | `deliver_message` routed to adapter; `message_ack` advances via `key` filter; filter selects the right trigger |
| `test_malformed.json` | 15 distinct structural errors; `validate()` reports all with precise paths; `--strict` refuses to run; **no crash** |

The canonical demo scenario (`printf 'time 48\n1\ncast\n1\nstate\nquit\n' |
./build/cli/qe_cli`) remains byte-for-byte behaviorally identical and validates
clean.

---

## 3. What's solid vs still open

**Solid in the historical feature-line audit:**
- Historical non-random SPEC §4.1–4.3 vocabulary + §2.4 globals + §4.2 schedule/reset_quest matched the then-current SPEC.
- §4.5 async-message extension boundary correct.
- State-machine semantics §3.1–3.4 (lifecycle, dialogue flow incl. resumable/cancel/reset-in-dialogue, trigger ordering, choice filtering, termination).
- Crash-free on malformed input; precise `[quest-id] path: message` errors; offline `validate()` pass (SPEC §7) + `--strict`.
- Schema matches the implemented core vocabulary.

**Still open (deferred by design, not regressions):**
- **`random` runtime wiring** — current main has no quest runtime. A future
  evaluator must derive the exact RFC 6901/explicit site key, supply the restored
  save-wide seed, call `DeterministicRandom`, and validate malformed input.
- **Skyrim seed lifecycle** — the co-save adapter must generate `master_seed`
  once, serialize it at system level, and restore it before runtime construction;
  this needs DLL integration and in-game verification.
- **`priority` arbitration (§8)** — field accepted/validated but not enforced (SPEC marks enforcement provisional).
- **Source-file split** (`Conditions/Actions/Triggers.{h,cpp}`, DESIGN §6) — still folded into `QuestEngine.cpp`; fine for now.

---

## 4. Public-interface boundary

The original audit added `validate()` but did not change existing signatures.
The 2026-08-24 primitive does not import or modify those runtime interfaces.

1. **`validate()` is additive.** This new public method
   (no existing signature changed, `Deps` untouched), so it does not break the
   parallel agent: code that doesn't call it is unaffected. If the other agent
   considers even additive surface off-limits, `validate()` could instead live
   as a free function `std::vector<std::string> validate(const json&, const
   GlobalStore*)`. I judged the additive method safe and within the constraint
   ("do not *change* the public methods"); flagging it here for visibility.
2. **Persistence port remains deferred.** The historical feature core exposes JSON
   `exportProgress()`/`importProgress()` and global helpers. A game-specific
   `PersistenceBackend` port is still intentionally absent; the host owns the
   blob and save-wide `master_seed` lifecycle.
3. **Resolved-entity arguments remain deferred.** SPEC §5.2/
   §5.3 say the core should hand the port the *resolved entities* alongside the
   verb/params. The current signatures pass only `(verb/key, params)`. Adding an
   `EntityResolver` and threading resolved handles through would change these
   signatures — recommended for the Skyrim adapter phase, NOT applied.

---

## 5. 2026-08-24 PRF primitive provenance and scope

The branch starts from pinned `origin/main` commit `ea6bfeb`, which has the
spec/schema/audit but no quest runtime. No file from `feature/court-wizard` was
imported. That branch was consulted only to confirm the historical stub and the
absence of seed/path wiring.

New code is exactly `src/core/DeterministicRandom.{h,cpp}`, the standalone
`tools/quest_prf_test.cpp`, and its PowerShell compile/run wrapper. No
`QuestEngine`, ports/state, demo, historical fixtures, CLI harness,
`src/skyrim/`, plugin/CMake DLL registration, procgen, or in-game code is
included. Therefore the achieved scope is contract + minimum pure-function
seam, not runtime `random` support. There is no `CODE_MAP.md` at the pinned
base, so README provides the path/status routing instead of creating an index.
