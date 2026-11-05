#!/bin/sh
# Installs the `wren` command from Wren's releases:
#
#   curl -fsSL https://wrenbrowser.com/runtime/cli/install.sh | sh
#
# That address redirects to this file in MultiboxLabs/wren-runtime.
#
# It installs the newest release's `wren`, which works with every release.
# WREN_VERSION=0.3.0 installs that release's instead. It goes in
# ~/.local/bin, or in WREN_INSTALL_DIR when that is set. WREN_RELEASES_URL
# fetches from a mirror of the releases page instead of GitHub.
#
# Everything is in main, which runs on the last line, so a download cut off
# part way runs nothing.

set -eu

say() {
  printf '%s\n' "$*"
}

fail() {
  printf 'wren install: %s\n' "$*" >&2
  exit 1
}

main() {
  releases="${WREN_RELEASES_URL:-https://github.com/MultiboxLabs/wren-runtime/releases}"
  directory="${WREN_INSTALL_DIR:-$HOME/.local/bin}"
  if [ -n "${WREN_VERSION:-}" ]; then
    from="$releases/download/v${WREN_VERSION#v}"
  else
    from="$releases/latest/download"
  fi

  if [ "$(uname -s)" != Darwin ] || [ "$(uname -m)" != arm64 ]; then
    fail "Wren runs only on macOS on Apple silicon."
  fi

  scratch="$(mktemp -d)"
  trap 'rm -rf "$scratch"' EXIT

  say "Downloading wren from $from"
  curl -fsSL -o "$scratch/wren" "$from/wren" ||
    fail "Could not download $from/wren"

  # Releases before 0.4.0 list only their zips in SHA256SUMS.
  expected=""
  if curl -fsSL -o "$scratch/SHA256SUMS" "$from/SHA256SUMS" 2>/dev/null; then
    expected="$(awk '$2 == "wren" || $2 == "*wren" { print $1 }' "$scratch/SHA256SUMS")"
  fi
  if [ -n "$expected" ]; then
    actual="$(shasum -a 256 "$scratch/wren" | awk '{ print $1 }')"
    [ "$actual" = "$expected" ] ||
      fail "The download does not match SHA256SUMS (its SHA-256 is $actual). Nothing was installed."
  else
    say "This release's SHA256SUMS does not list wren, so it was not checked."
  fi

  chmod +x "$scratch/wren"
  mkdir -p "$directory"
  mv -f "$scratch/wren" "$directory/wren"
  say "Installed $("$directory/wren" --version) in $directory"

  case ":$PATH:" in
    *":$directory:"*) ;;
    *)
      say ""
      say "$directory is not on your PATH. Add it, then open a new terminal:"
      say ""
      say "  echo 'export PATH=\"$directory:\$PATH\"' >> ~/.zprofile"
      ;;
  esac
}

main "$@"
