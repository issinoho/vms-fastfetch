#!/usr/bin/env bash
# run.sh <node> [fastfetch args...] - run the built FASTFETCH.EXE on <node> in
# an interactive login session on a pseudo-terminal, so SYS$OUTPUT is a
# terminal (FTAnnn:) and colours, terminal detection and the terminal
# driver behave as they do for a user.
#
# A one-shot "ssh host command" loses the tail of the output (the session
# closes before it drains), so the commands are typed into a full DCL
# session instead and the output is cut out between markers.  The markers
# are built by concatenation so the echo of the typed command never matches.
# LOGINOUT flushes typeahead, so nothing is typed until the login is done
# (RUN_LOGIN_WAIT seconds).  RUN_RAW=1 shows the whole session.
# Bounded by timeout (CLAUDE.md).
set -euo pipefail

top=$(cd "$(dirname "$0")/.." && pwd)
node=${1:?usage: run.sh <node> [args...]}
shift
read -r _ ARCH HOST PORT USER WORKDIR _ < <(awk -v n="$node" '$1==n' "$top/tools/nodes.conf") || true
[ -n "${HOST:-}" ] || { echo "run: unknown node '$node'" >&2; exit 2; }
exe="${WORKDIR%]}.BIN_${ARCH}]FASTFETCH.EXE"

type_commands() {
    sleep "${RUN_LOGIN_WAIT:-12}"
    printf '%s\r\n' \
        'write sys$output "FFRUN-" + "BEGIN"' \
        "mcr $exe $*" \
        'write sys$output "FFRUN-" + "STATUS=", $status' \
        'write sys$output "FFRUN-" + "END"'
    sleep "${RUN_WAIT:-8}"
    printf 'logout\r\n'
    sleep 3
}

out=$(type_commands "$@" |
      timeout "${RUN_TIMEOUT:-90}" ssh -tt -i "${VMS_SSH_KEY:-$HOME/.ssh/vms_ed25519}" \
        -o BatchMode=yes -o LogLevel=ERROR -p "$PORT" "$USER@$HOST" 2>&1 |
      tr -d '\r\000' || true)
if [ -n "${RUN_RAW:-}" ]; then printf '%s\n' "$out"; exit 0; fi
printf '%s\n' "$out" | awk '/^FFRUN-BEGIN$/{on=1; skip=1; next} /^FFRUN-END$/{exit} on && skip {skip=0; next} on'
