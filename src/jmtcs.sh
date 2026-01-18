#!/usr/bin/env bash
# `jmtcs.sh` now exposes a single public function `jmtcsLoop`.
# All core logic is in `jmtcs_core.sh` which is executed (not sourced),
# so helper functions do not leak into the interactive shell.

jmtcsLoop() {
    # Locate the core script relative to this file.
    local _dir
    _dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    local _core="${_dir}/jmtcs_core.sh"

    if [ ! -x "${_core}" ]; then
        # If core isn't executable, try to run it with bash.
        if [ -f "${_core}" ]; then
            bash "${_core}" "$@"
            return $?
        else
            echo "jmtcs core script not found: ${_core}" >&2
            return 127
        fi
    fi

    "${_core}" "$@"
}