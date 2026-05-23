#!/usr/bin/env bash
# Conformance / regression harness for the portable quest engine core.
# Builds the headless CLI (scripts/build_cli.sh) then drives a set of scripted
# scenarios through it, asserting on the combined stdout+stderr. Each scenario
# checks one SPEC behavior; a failed assertion prints the captured output.
#
# Usage: scripts/test_quests.sh        (build + run all)
#        scripts/test_quests.sh --no-build
#
# Exit code 0 = all passed, 1 = at least one failed.
set -uo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CLI="$ROOT/build/cli/qe_cli"
Q="$ROOT/config/quests"

PASS=0
FAIL=0

if [ "${1:-}" != "--no-build" ]; then
    "$ROOT/scripts/build_cli.sh" >/tmp/qe_build.log 2>&1 || { echo "BUILD FAILED:"; cat /tmp/qe_build.log; exit 1; }
fi
[ -x "$CLI" ] || { echo "missing $CLI (build first)"; exit 1; }

# run <quest> <stdin-script> -> captures combined output into $OUT
run() {
    local quest="$1"; shift
    local script="$1"; shift
    OUT="$(printf '%b' "$script" | "$CLI" "$quest" "$@" 2>&1)"
}

# expect <label> <substring...> : every substring MUST appear in $OUT
expect() {
    local label="$1"; shift
    local ok=1
    for needle in "$@"; do
        if ! grep -qF -- "$needle" <<<"$OUT"; then
            ok=0
            echo "FAIL: $label"
            echo "      missing: $needle"
        fi
    done
    if [ "$ok" = 1 ]; then echo "PASS: $label"; PASS=$((PASS+1)); else FAIL=$((FAIL+1)); echo "----- output -----"; echo "$OUT"; echo "------------------"; fi
}

# refute <label> <substring> : substring MUST NOT appear in $OUT
refute() {
    local label="$1"; local needle="$2"
    if grep -qF -- "$needle" <<<"$OUT"; then
        echo "FAIL: $label"
        echo "      unexpected: $needle"
        echo "----- output -----"; echo "$OUT"; echo "------------------"
        FAIL=$((FAIL+1))
    else
        echo "PASS: $label"; PASS=$((PASS+1))
    fi
}

# count_eq <label> <n> <substring> : substring MUST appear exactly <n> times in $OUT
count_eq() {
    local label="$1"; local want="$2"; local needle="$3"
    local got
    got="$(grep -cF -- "$needle" <<<"$OUT")"
    if [ "$got" = "$want" ]; then
        echo "PASS: $label"; PASS=$((PASS+1))
    else
        echo "FAIL: $label"
        echo "      expected $want occurrence(s) of: $needle (got $got)"
        echo "----- output -----"; echo "$OUT"; echo "------------------"
        FAIL=$((FAIL+1))
    fi
}

echo "== quest-engine conformance =="

# ---- 1. canonical demo (byte-identical scenario from the task) ----
run "$Q/demo_court_wizard.json" 'time 48\n1\ncast\n1\nstate\nquit\n'
expect "demo: letter arrives at 48h"          "一封來自龍臨堡的信送到了。"
expect "demo: brief dialogue opens"           "你能解此厄嗎？"
expect "demo: spawn_character routed to adapter" "[adapter action] spawn_character"
expect "demo: cast completes objective -> thanks dialogue" "詛咒已解！"
expect "demo: reward gold routed to adapter"  "[adapter action] give_gold"
expect "demo: reset_quest re-runs on_start"   "（你成為白漫的客座大法師，回去等候領主差遣。）"
expect "demo: global survives reset"          "global.whiterun_tasks_done=1"
expect "demo: quest var reset to initial"     "delivered=false"
refute "demo: no validation problems"         "validation:"

# ---- 2. timers: schedule, due-at-now, multiple-due, re-schedule ----
run "$Q/test_timers.json" 'time 5\ntime 5\ntime 10\nstate\nquit\n'
expect "timers: A/B/C all fire at due time"   "timer A due" "timer B due" "timer C (absolute at=10) due"
expect "timers: re-scheduled key fires at new time" "RESCHEDULED timer fired"
expect "timers: fired counter = 4"            "fired=4"
# the re-scheduled timer must NOT have fired at the original 5h (only after 20h)
run "$Q/test_timers.json" 'time 5\nstate\nquit\n'
refute "timers: re-schedule replaced old due time" "RESCHEDULED timer fired"

# ---- 3. conditions: nested all/any/not, type-aware equality ----
run "$Q/test_conditions.json" 'quit\n'
expect "cond: ALL branch passes"              "ALL passed"
expect "cond: ANY branch passes"              "ANY passed"
expect "cond: NOT branch passes"              "NOT(count==0) passed"
expect "cond: nested NOT(ANY) AND var_neq"    "nested NOT(ANY(...)) AND var_neq passed"
expect "cond: global var read"                "global.runs == 0 passed"
refute "cond: bool!=number (no false match)"  "BUG if shown"

# ---- 4. dialogue: when-gated choice, goto chain, objective + dialogue_end triggers ----
run "$Q/test_dialogue.json" '1\n1\n3\n1\nstate\nquit\n'
expect "dialogue: gated choice hidden at trust 0 then shown" "[3] [Only if trusted]"
expect "dialogue: secret node reached"        "the gold is buried under the old oak"
expect "dialogue: objective_completed trigger" "Objective complete: you learned the secret."
expect "dialogue: dialogue_end trigger"       "turns back to his wares"
expect "dialogue: vars updated"               "told_secret=true" "learn_secret=done"
# threaten path ends early, objective stays active
run "$Q/test_dialogue.json" '2\nstate\nquit\n'
expect "dialogue: threaten -> early end"      "I don't deal with brutes"
expect "dialogue: objective untouched on early end" "learn_secret=active"
# cancel mid-dialogue (EOF / non-numeric) ends the dialogue
run "$Q/test_dialogue.json" 'x\nstate\nquit\n'
expect "dialogue: cancel ends dialogue"       "turns back to his wares"

# ---- 5. reset_quest: quest vars reset, globals survive across loops ----
run "$Q/test_reset.json" 'fire objective_completed objective do_loop\nfire objective_completed objective do_loop\nstate\nquit\n'
expect "reset: loop re-runs on_start"         "loop finished; resetting." "loop started"
expect "reset: local var back to initial"     "local_counter=1"
expect "reset: global counts across loops"    "global.runs=3"

# ---- 5a-M1. reset_quest inside a dialogue choice whose re-start opens a DIFFERENT
# dialogue must NOT be wiped by the post-choice guard (REVIEW_FINDINGS M1). The
# MENU choice resets the quest; the reset's quest_start opens BRIEF; the guard must
# preserve BRIEF (active dialogue now != the menu we were resolving). With the bug,
# BRIEF's state is cleared so ending it fires dialogue_end with an empty id and the
# "BRIEF ENDED CLEANLY" marker never shows.
run "$Q/test_reset_dialogue.json" '1\n1\nstate\nquit\n'
expect "M1: reset opens a fresh different dialogue" "BRIEF DIALOGUE OPENED BY RESET"
expect "M1: fresh dialogue survives the choice guard" "BRIEF ENDED CLEANLY"
expect "M1: global loop counter advanced past reset" "global.runs=2"

# ---- 5b. termination: complete_quest stops the chain AND all triggers (§3.1.3) ----
run "$Q/test_termination.json" 'fire ping\ntime 100\nstate\nquit\n'
expect "term: quest terminated"               "terminated: yes"
refute "term: action after complete_quest skipped" "action after complete_quest ran"
refute "term: trigger ignored after termination"   "terminated quest responded to a trigger"
refute "term: timer ignored after termination"     "terminated quest fired a timer"
expect "term: n stayed 0 (no post-terminate writes)" "n=0"

# ---- 6. async message: deliver_message routed, message_ack advances ----
run "$Q/test_async_message.json" 'fire message_ack key summons\nstate\nquit\n'
expect "async: deliver_message routed to adapter" "[adapter action] deliver_message"
expect "async: message_ack advances quest"    "You read the summons."
expect "async: acked flag set"                "acked=true"
refute "async: filter key selects right trigger" "wrong message key matched"

# ---- 7. malformed input: validation reports all problems, no crash ----
run "$Q/test_malformed.json" 'quit\n'
expect "malformed: missing title flagged"     "title: missing or non-string required field 'title'"
expect "malformed: bad objective state"       "objectives.obj1.state: must be inactive|active|done|failed"
expect "malformed: type mismatch"             "type mismatch: var 'n' is number"
expect "malformed: missing var key"           "set_var: needs {var:string, value}"
expect "malformed: unknown objective ref"     "references unknown objective 'ghost_objective'"
expect "malformed: unknown dialogue ref"      "references unknown dialogue 'ghost_dialogue'"
expect "malformed: schedule missing timing"   "needs numeric 'at' or 'after_hours'"
expect "malformed: multi-verb action"         "action must be an object with exactly one verb key"
expect "malformed: trigger missing on"        "missing or non-string required 'on'"
expect "malformed: bad entry node"            "'entry' points to missing node 'missing_entry_node'"
expect "malformed: bad goto target"           "points to missing node 'nonexistent_node'"
expect "malformed: choice missing goto/end"   "choice needs either 'goto' or 'end:true'"
expect "malformed: undeclared var in cond"    "references undeclared var 'undeclared'"
expect "malformed: node missing choices/end"  "node needs either 'choices' or 'end:true'"
# --strict must refuse to run a broken quest
run "$Q/test_malformed.json" 'quit\n' --strict
expect "malformed: --strict refuses to run"   "(--strict) refusing to run."

# ---- 8. co-save persistence (SPEC §6): export -> reconstruct engine -> import ----
# 8a. pending timers survive a reload at ABSOLUTE due time (§6/§8). Save right
# after start (a/b/c due at 10h, reschedule_me at 20h), reload into a FRESH
# engine, then advance: the staged timers must still fire after the reload.
run "$Q/test_timers.json" 'save\nreload\ntime 10\nstate\ntime 10\nstate\nquit\n'
expect "persist: blob carries absolute timer due times" '"timers":{"a":10.0,"b":10.0,"c":10.0,"reschedule_me":20.0}'
expect "persist: timers fire after reload"    "timer A due" "timer B due" "timer C (absolute at=10) due"
expect "persist: re-scheduled timer survives reload" "RESCHEDULED timer fired"
expect "persist: fired counter restored + advanced" "fired=4"
# 8b. quest vars + objective state survive a reload; the quest then continues
# from the restored state (cast completes the restored-active objective, the
# thanks dialogue rewards + reset_quest fires, and the GLOBAL survives the reset).
run "$Q/demo_court_wizard.json" 'time 48\n1\nsave\nreload\nstate\ncast\n1\nstate\nquit\n'
expect "persist: objective state in blob"     '"objectives":{"lift_curse":"active"}' '"delivered":true'
expect "persist: state restored after reload" "delivered=true" "lift_curse=active"
expect "persist: quest continues post-reload" "詛咒已解！" "[adapter action] give_gold"
expect "persist: global survives reload+reset" "global.whiterun_tasks_done=1"
# 8c. globals round-trip independently of any quest var of the same shape.
run "$Q/test_reset.json" 'fire objective_completed objective do_loop\nsave\nreload\nstate\nquit\n'
expect "persist: global value restored"       "global.runs=2"
# 8d-M2. Restoring saved progress must NOT re-fire on_start side effects: the
# engine starts ONCE (intro message), then a save+reload RECONSTRUCTS the engine
# and importProgress()-restores progress WITHOUT re-running on_start. So the demo
# intro show_message appears EXACTLY ONCE across a save+reload, not twice
# (REVIEW_FINDINGS M2 — the adapter mirrors this by building at kDataLoaded,
# start() only on kNewGame, importProgress on kPostLoadGame).
run "$Q/demo_court_wizard.json" 'save\nreload\nstate\nquit\n'
count_eq "M2: intro on_start fires once (not re-fired by importProgress)" 1 "（你成為白漫的客座大法師，回去等候領主差遣。）"

echo
echo "== $PASS passed, $FAIL failed =="
[ "$FAIL" = 0 ]
