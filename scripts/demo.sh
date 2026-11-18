#!/usr/bin/env bash
# k_v_db live demo: the whole story in one command.
#
#   scripts/demo.sh            pauses before every step (for presenting)
#   scripts/demo.sh --no-pause runs straight through (for a quick check)
#
# Starts real nodes on this machine ([::1], data ports 9001-9004, DHT ports
# 4001-4004), talks to them with kvcli, and stops everything at the end,
# also on Ctrl+C or an error. Each node's log goes to demo-logs/<port>.log.
#
#   1. Three nodes find each other through OpenDHT (no peer lists)
#   2. Write data: every key is stored on two nodes
#   3. A node crashes: reads still work, served by the replica
#   4. A node freezes: heartbeats notice it within a second
#   5. The crashed node restarts empty and gets its keys back (recovery)
#   6. A fourth node joins: keys move to it automatically (rebalancing)

set -uo pipefail

cd "$(dirname "$0")/.." || exit 1

BUILD="${BUILD:-build}"
NODE="$BUILD/k_v_db"
CLI="$BUILD/kvcli"
LOGS="demo-logs"
PAUSE=1
[[ "${1:-}" == "--no-pause" ]] && PAUSE=0

KEYS=(student101 student102 alice bob course123)
declare -A PID=()  # data port -> process id

# ── Output helpers ──────────────────────────────────────────────────────

if [[ -t 1 ]]; then
    BOLD=$'\e[1m'; DIM=$'\e[2m'; GREEN=$'\e[32m'; RED=$'\e[31m'; CYAN=$'\e[36m'; RESET=$'\e[0m'
else
    BOLD=""; DIM=""; GREEN=""; RED=""; CYAN=""; RESET=""
fi

step()  { printf '\n%s━━━ %s ━━━%s\n' "$BOLD$CYAN" "$*" "$RESET"; }
say()   { printf '%s\n' "$*"; }
ok()    { printf '%s✔ %s%s\n' "$GREEN" "$*" "$RESET"; }
fail()  { printf '%s✘ %s%s\n' "$RED" "$*" "$RESET"; }

# Shows a command, then runs it. kvcli exits 1 for NOT_FOUND / UNAVAILABLE,
# which is often the point of the step, so failures don't stop the script.
run() {
    printf '%s$ %s%s\n' "$DIM" "$*" "$RESET"
    "$@"
}

pause() {
    (( PAUSE )) || return 0
    read -rp "${DIM}   ⏎ Enter to continue${RESET}" _ </dev/tty
}

# ── Node control ────────────────────────────────────────────────────────

# start_node <data port> <dht port> [bootstrap dht port]
start_node() {
    local port=$1 dht=$2 boot=${3:-}
    local args=(--listen "[::1]:$port" --dht "$dht")
    [[ -n "$boot" ]] && args+=(--bootstrap "[::1]:$boot")
    # stdbuf -oL: line-buffered output, so the log is up to date even if the
    # node is killed (a redirected stdout is otherwise block-buffered).
    stdbuf -oL "$NODE" "${args[@]}" >"$LOGS/$port.log" 2>&1 &
    PID[$port]=$!
    printf '%s$ k_v_db %s   (pid %s, log %s)%s\n' "$DIM" "${args[*]}" "${PID[$port]}" "$LOGS/$port.log" "$RESET"
}

# Clean shutdown. SIGTERM, not SIGINT: bash starts background jobs of a
# script with SIGINT *ignored*, and an ignored signal is discarded before it
# can reach the node's signalfd. The node handles SIGTERM the same way.
stop_node() {
    local port=$1
    [[ -n "${PID[$port]:-}" ]] || return 0
    kill -TERM "${PID[$port]}" 2>/dev/null
    wait "${PID[$port]}" 2>/dev/null
    unset "PID[$port]"
}

# A crash: no shutdown at all, memory lost.
crash_node() {
    local port=$1
    kill -KILL "${PID[$port]}" 2>/dev/null
    wait "${PID[$port]}" 2>/dev/null
    unset "PID[$port]"
}

cleanup() {
    for port in "${!PID[@]}"; do
        kill -CONT "${PID[$port]}" 2>/dev/null  # a frozen node can't handle SIGTERM
        kill -TERM "${PID[$port]}" 2>/dev/null
    done
    wait 2>/dev/null
}
trap cleanup EXIT
trap 'printf "\ninterrupted\n"; exit 130' INT TERM

# How many peers the node on <port> sees as CONNECTED (-w: not DISCONNECTED).
connected() { "$CLI" ::1 "$1" status 2>/dev/null | grep -cw CONNECTED; }

# wait_connected <port> <expected peers> <timeout seconds>
wait_connected() {
    local port=$1 want=$2 deadline=$((SECONDS + $3))
    while (( SECONDS < deadline )); do
        (( $(connected "$port") == want )) && return 0
        sleep 0.2
    done
    fail "node $port still sees $(connected "$port")/$want peers after $3 s (see $LOGS/$port.log)"
    return 1
}

# Lines from a node's log, e.g. show_log 9003 "recovery:"
show_log() {
    say "${DIM}── $LOGS/$1.log ($2) ──${RESET}"
    grep -- "$2" "$LOGS/$1.log" | sed 's/^/   /'
}

# ── Preconditions ───────────────────────────────────────────────────────

for bin in "$NODE" "$CLI"; do
    [[ -x "$bin" ]] || { fail "$bin not found: build first (cmake --build $BUILD)"; exit 1; }
done
if pgrep -x k_v_db >/dev/null; then
    fail "k_v_db nodes are already running (pgrep -af k_v_db): stop them first"
    exit 1
fi
rm -rf "$LOGS" && mkdir -p "$LOGS"

# ════════════════════════════════════════════════════════════════════════

step "1. Three nodes find each other through OpenDHT"
say "No peer lists: node 9001 starts the DHT, the others only know one bootstrap address."
pause
start_node 9001 4001
sleep 0.5
start_node 9002 4002 4001
start_node 9003 4003 4001
say "Waiting for discovery and the TCP control connections..."
for p in 9001 9002 9003; do wait_connected "$p" 2 20 || exit 1; done
ok "all three nodes connected to each other"
run "$CLI" ::1 9001 status
pause

step "2. Write data: every key is stored on two nodes"
for k in "${KEYS[@]}"; do run "$CLI" ::1 9001 put "$k" "v-$k"; done
say
say "Each key has a primary and a replica, chosen by XOR distance:"
run "$CLI" ::1 9001 locate student101
say
for p in 9001 9002 9003; do "$CLI" ::1 "$p" status | head -1; done
say "${DIM}(5 keys x 2 copies = 10 entries across the cluster)${RESET}"
pause

step "3. A node crashes: reads still work"
say "Killing 9003 with SIGKILL: no clean shutdown, its memory is gone."
say "9003 is the primary of student101, alice and course123."
pause
crash_node 9003
sleep 1.5
run "$CLI" ::1 9001 status
say
say "Reading keys whose primary is dead, through 9002 (which holds neither copy):"
for k in student101 alice course123; do run "$CLI" ::1 9002 get "$k"; done
ok "served by the replica on 9001"
say
say "Writes need both copies, so a write to a key of the dead primary is refused:"
run "$CLI" ::1 9001 put student101 changed
pause

step "4. A node freezes: heartbeats notice"
say "SIGSTOP pauses 9002 without closing its sockets: TCP alone would never notice."
say "Every node sends PING every 250 ms; 4 missed PONGs = down."
pause
kill -STOP "${PID[9002]}"
sleep 1.6
run "$CLI" ::1 9001 status
show_log 9001 "no heartbeat"
kill -CONT "${PID[9002]}"
say "Unfrozen. Waiting for it to reconnect..."
wait_connected 9001 1 10 && ok "9002 is back"
pause

step "5. The crashed node restarts empty and gets its keys back"
start_node 9003 4003 4001
for p in 9001 9002 9003; do wait_connected "$p" 2 20 || exit 1; done
sleep 0.5
run "$CLI" ::1 9003 status
show_log 9003 "recovery:"
say
for k in student101 alice course123; do run "$CLI" ::1 9001 get "$k"; done
ok "the restarted primary has its data again"
pause

step "6. A fourth node joins: keys move to it automatically"
start_node 9004 4004 4001
for p in 9001 9002 9003 9004; do wait_connected "$p" 3 20 || exit 1; done
sleep 0.5
ok "four nodes, all connected"
run "$CLI" ::1 9004 status
show_log 9004 "recovery:"
say
say "bob now belongs to the new node:"
run "$CLI" ::1 9004 locate bob
say
say "Every key is readable through the new node:"
for k in "${KEYS[@]}"; do run "$CLI" ::1 9004 get "$k"; done
pause

step "Done"
say "Stopping all nodes. Full logs: $LOGS/"
for p in "${!PID[@]}"; do stop_node "$p"; done
ok "demo finished"
