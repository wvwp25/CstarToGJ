#!/usr/bin/env bash

dispatch_workspace_couplings() {
  local runner="$1"
  shift
  local -a requested=() passthrough=() couplings=()
  local value rc status=0 help_requested=0

  while [ "$#" -gt 0 ]; do
    case "$1" in
      -f|--coupling)
        shift
        if [ "$#" -eq 0 ] || [[ "$1" == -* ]]; then
          echo "Missing value after -f/--coupling" >&2
          return 2
        fi
        while [ "$#" -gt 0 ] && [[ "$1" != -* ]]; do
          requested+=("$1")
          shift
        done
        ;;
      -h|--help)
        help_requested=1
        passthrough+=("$1")
        shift
        ;;
      *)
        passthrough+=("$1")
        shift
        ;;
    esac
  done

  if [ "${#requested[@]}" -eq 0 ]; then
    if [ "$help_requested" -eq 1 ]; then
      requested=(f1p0)
    else
      # A single explicit card/workspace already identifies its coupling.
      for value in "${passthrough[@]}"; do
        if [[ "$value" =~ _(f1p0|f0p5|f0p1)(\.txt|\.root)$ ]]; then
          requested=("${BASH_REMATCH[1]}")
          break
        fi
      done
      if [ "${#requested[@]}" -eq 0 ]; then
        requested=(f1p0 f0p5 f0p1)
      fi
    fi
  fi
  for value in "${requested[@]}"; do
    case "$value" in
      all|f\*) couplings=(f1p0 f0p5 f0p1); break ;;
      f1p0|f0p5|f0p1)
        if [[ " ${couplings[*]} " != *" $value "* ]]; then
          couplings+=("$value")
        fi
        ;;
      *)
        echo "Unsupported coupling: $value" >&2
        echo "Use one or more of: f1p0 f0p5 f0p1, or all" >&2
        return 2
        ;;
    esac
  done

  for value in "${couplings[@]}"; do
    if [ "$help_requested" -eq 0 ]; then
      echo "===== Coupling $value ====="
    fi
    CSTAR_SINGLE_COUPLING_RUN=1 "$runner" -f "$value" "${passthrough[@]}"
    rc=$?
    if [ "$rc" -ne 0 ]; then status=$rc; fi
  done
  return "$status"
}
