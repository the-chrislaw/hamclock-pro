#!/usr/bin/env bash
# Offline tests for the Pi image updater (packaging/pi/.../hamclock-pro-update), issue #546.
# Runs the real script against fake release metadata, fake dpkg/apt/systemctl/curl on PATH and a
# throwaway Ed25519 key generated here. No network, no root, no real package installs.
# Exit 77 (ctest SKIP) when no OpenSSL 3 is available to make and check signatures.
set -euo pipefail

here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/.." && pwd)
files="$root/packaging/pi/stage-hamclock/00-install/files"
updater="$files/hamclock-pro-update"
repo=the-chrislaw/hamclock-pro
dl="https://github.com/$repo/releases/download"

find_openssl() {
  local c
  for c in "$(command -v openssl || true)" /opt/homebrew/opt/openssl@3/bin/openssl /usr/local/opt/openssl@3/bin/openssl; do
    [ -n "$c" ] && [ -x "$c" ] && "$c" version 2>/dev/null | grep -Eq '^OpenSSL ([3-9]|[1-9][0-9])\.' && { echo "$c"; return 0; }
  done
  return 1
}
openssl_bin=$(find_openssl) || { echo "SKIP: needs OpenSSL 3 for Ed25519"; exit 77; }
command -v jq >/dev/null || { echo "SKIP: needs jq"; exit 77; }

work=$(mktemp -d "${TMPDIR:-/tmp}/hcp-updater-test.XXXXXX")
trap 'rm -rf "$work"' EXIT
failures=0
pass() { echo "ok   - $1"; }
fail() { echo "FAIL - $1"; failures=$((failures + 1)); }

# ---- fake system tools ------------------------------------------------------------------------
bin="$work/bin"; mkdir -p "$bin"
ln -s "$openssl_bin" "$bin/openssl"
if ! command -v sha256sum >/dev/null; then
  printf '#!/bin/sh\nexec shasum -a 256 "$@"\n' > "$bin/sha256sum"; chmod +x "$bin/sha256sum"
fi

cat > "$bin/curl" <<'EOF'
#!/usr/bin/env bash
# Serves $FAKE_NET/<url with scheme stripped>; logs every call.
echo "curl $*" >> "$FAKE_LOG"
out=; url=
while [ $# -gt 0 ]; do
  case "$1" in
    -o) out=$2; shift 2 ;;
    -A|--proto|--proto-redir|--max-time|--retry|--max-filesize|--connect-timeout|-H) shift 2 ;;
    -*) shift ;;
    *) url=$1; shift ;;
  esac
done
case "$url" in https://*) ;; *) echo "fake curl: non-https $url" >&2; exit 1 ;; esac
src="$FAKE_NET/${url#https://}"
[ -f "$src" ] || { echo "fake curl: 404 $url" >&2; exit 22; }
if [ -n "$out" ]; then cp "$src" "$out"; else cat "$src"; fi
EOF

cat > "$bin/dpkg" <<'EOF'
#!/usr/bin/env bash
case "$1" in
  --print-architecture) echo "${FAKE_ARCH:-arm64}" ;;
  --compare-versions)
    a=$2; op=$3; b=$4
    [ "$a" = "$b" ] && { case "$op" in eq|ge|le) exit 0 ;; *) exit 1 ;; esac; }
    hi=$(printf '%s\n%s\n' "$a" "$b" | sort -V | tail -n1)
    case "$op" in gt|ge) [ "$hi" = "$a" ] ;; lt|le) [ "$hi" = "$b" ] ;; *) exit 1 ;; esac ;;
  *) echo "fake dpkg: unsupported $*" >&2; exit 2 ;;
esac
EOF

cat > "$bin/dpkg-deb" <<'EOF'
#!/usr/bin/env bash
# Fake .deb files are plain "Field: value" text.
[ "$1" = "-f" ] || { echo "fake dpkg-deb: unsupported $*" >&2; exit 2; }
grep -q '^Package: ' "$2" || { echo "fake dpkg-deb: not a deb" >&2; exit 2; }
sed -n "s/^$3: //p" "$2"
EOF

cat > "$bin/dpkg-query" <<'EOF'
#!/usr/bin/env bash
[ -f "$FAKE_STATE/installed" ] || exit 1
cat "$FAKE_STATE/installed"
EOF

cat > "$bin/apt-get" <<'EOF'
#!/usr/bin/env bash
echo "apt-get $*" >> "$FAKE_LOG"
[ "$1" = update ] && exit 0
deb=${*: -1}
grep -q '^FailInstall: yes' "$deb" && exit 100
sed -n 's/^Version: //p' "$deb" > "$FAKE_STATE/installed"
EOF

cat > "$bin/systemctl" <<'EOF'
#!/usr/bin/env bash
echo "systemctl $*" >> "$FAKE_LOG"
case "$1" in
  restart) exit 0 ;;
  is-active)
    [ -f "$FAKE_STATE/kiosk-down" ] && exit 3
    v=$(cat "$FAKE_STATE/installed" 2>/dev/null || true)
    grep -qx "$v" "$FAKE_STATE/crashy" 2>/dev/null && exit 3
    exit 0 ;;
  *) exit 0 ;;
esac
EOF
chmod +x "$bin"/*

# ---- keys -------------------------------------------------------------------------------------
"$openssl_bin" genpkey -algorithm ed25519 -out "$work/good.key" 2>/dev/null
"$openssl_bin" pkey -in "$work/good.key" -pubout -out "$work/good.pub"
"$openssl_bin" genpkey -algorithm ed25519 -out "$work/evil.key" 2>/dev/null

# ---- scenario builder -------------------------------------------------------------------------
# new_case <name>: fresh state, installed 1.0.0, kiosk up, release v1.1.0 signed with good key.
new_case() {
  case_dir="$work/$1"; mkdir -p "$case_dir/net" "$case_dir/state" "$case_dir/sys"
  export FAKE_NET="$case_dir/net" FAKE_STATE="$case_dir/sys" FAKE_LOG="$case_dir/log" FAKE_ARCH=arm64
  : > "$FAKE_LOG"
  echo 1.0.0 > "$FAKE_STATE/installed"
  printf 'NAME="Debian"\nVERSION_CODENAME=trixie\n' > "$case_dir/os-release"
  cp "$work/good.pub" "$case_dir/pubkey"
  tag=v1.1.0; version=1.1.0; pkg=hamclock-pro; arch=arm64
  deb_name="hamclock-pro_1.1.0_trixie_arm64.deb"; extra_assets=(); prerelease=false
  sign_key="$work/good.key"; sig=yes; sums=yes; deb_extra=
}

# publish: write release JSON + assets for the current scenario variables.
publish() {
  local adir="$FAKE_NET/github.com/$repo/releases/download/$tag"
  mkdir -p "$adir" "$FAKE_NET/api.github.com/repos/$repo/releases"
  printf 'Package: %s\nVersion: %s\nArchitecture: %s\n%s' "$pkg" "$version" "$arch" "${deb_extra:-}" > "$adir/$deb_name"
  (cd "$adir" && sha256sum "$deb_name" > SHA256SUMS)
  [ -n "${tamper_deb:-}" ] && echo "tampered" >> "$adir/$deb_name"
  [ "$sig" = yes ] && "$openssl_bin" pkeyutl -sign -inkey "$sign_key" -rawin -in "$adir/SHA256SUMS" -out "$adir/SHA256SUMS.sig"
  [ -n "${tamper_sums:-}" ] && echo "0000  hamclock-pro_9.9.9_trixie_arm64.deb" >> "$adir/SHA256SUMS"
  [ "$sums" = yes ] || rm -f "$adir/SHA256SUMS"
  local assets=() u
  assets+=("$dl/$tag/$deb_name")
  [ "$sums" = yes ] && assets+=("$dl/$tag/SHA256SUMS")
  [ "$sig" = yes ] && assets+=("$dl/$tag/SHA256SUMS.sig")
  for u in ${extra_assets[@]+"${extra_assets[@]}"}; do assets+=("$u"); done
  printf '%s\n' "${assets[@]}" \
    | jq -R . | jq -s --arg tag "$tag" --argjson pre "$prerelease" \
      '{tag_name: $tag, prerelease: $pre, draft: false, body: "see https://evil.example/x_trixie_arm64.deb",
        assets: map({name: (split("/") | last), browser_download_url: .})}' \
      > "$FAKE_NET/api.github.com/repos/$repo/releases/latest"
}

run_updater() {
  set +e
  PATH="$bin:/usr/bin:/bin:/usr/sbin:/sbin" \
    HCP_UPDATE_PUBKEY="$case_dir/pubkey" HCP_UPDATE_STATE_DIR="$case_dir/state" \
    HCP_UPDATE_OS_RELEASE="$case_dir/os-release" HCP_UPDATE_HEALTH_WAIT=0 \
    bash "$updater" > "$case_dir/out" 2>&1
  rc=$?
  set -e
}

installed() { cat "$FAKE_STATE/installed"; }
installs() { grep -c '^apt-get install' "$FAKE_LOG" || true; }
expect_refused() {  # <label> <message regex>
  if [ "$rc" -ne 0 ] && [ "$(installs)" = 0 ] && [ "$(installed)" = 1.0.0 ] && grep -Eqi "$2" "$case_dir/out"; then
    pass "$1"
  else
    fail "$1 (rc=$rc installs=$(installs) installed=$(installed))"; sed 's/^/    | /' "$case_dir/out"
  fi
}

# ---- source-level checks ----------------------------------------------------------------------
head -n1 "$updater" | grep -q '^#!/bin/bash' && pass "updater runs under bash" || fail "updater shebang must be /bin/bash"
grep -q '^set -euo pipefail' "$updater" && pass "updater uses set -euo pipefail" || fail "updater lacks set -euo pipefail"
grep -q "proto '=https'" "$updater" && grep -q "proto-redir '=https'" "$updater" \
  && pass "curl restricted to https (incl. redirects)" || fail "curl must use --proto =https and --proto-redir =https"
grep -q 'release-signing.pub' "$root/packaging/pi/stage-hamclock/00-install/01-run.sh" \
  && pass "image installs the release public key" || fail "01-run.sh must install release-signing.pub"
for p in openssl jq; do
  grep -qx "$p" "$root/packaging/pi/stage-hamclock/00-install/00-packages" \
    && pass "image ships $p" || fail "00-packages must list $p"
done
grep -q 'RELEASE_SIGNING_KEY' "$root/.github/workflows/release.yml" && grep -q 'SHA256SUMS.sig' "$root/.github/workflows/release.yml" \
  && pass "release workflow signs SHA256SUMS when the key exists" || fail "release.yml must sign SHA256SUMS with RELEASE_SIGNING_KEY"

# The committed key must be either a real Ed25519 public key or the fail-closed placeholder.
committed="$files/release-signing.pub"
if [ ! -f "$committed" ]; then
  fail "release-signing.pub missing"
elif grep -q 'BEGIN PUBLIC KEY' "$committed"; then
  "$openssl_bin" pkey -pubin -in "$committed" -noout -text 2>/dev/null | grep -qi '^ED25519 Public-Key' \
    && pass "committed release key is Ed25519" || fail "committed release key is not a valid Ed25519 public key"
else
  grep -q 'PLACEHOLDER' "$committed" && pass "committed release key is the fail-closed placeholder" \
    || fail "release-signing.pub is neither a key nor the placeholder"
fi

# ---- behaviour --------------------------------------------------------------------------------
new_case happy; publish; run_updater
if [ "$rc" -eq 0 ] && [ "$(installed)" = 1.1.0 ] && grep -q 'systemctl restart hamclock-kiosk.service' "$FAKE_LOG" \
   && [ -f "$case_dir/state/current.deb" ]; then pass "installs a correctly signed newer release and restarts the kiosk"
else fail "happy path (rc=$rc installed=$(installed))"; sed 's/^/    | /' "$case_dir/out"; fi
if grep '^curl' "$FAKE_LOG" | grep -v -- "--proto =https" | grep -q .; then fail "every curl call passes --proto =https"
else pass "every curl call passes --proto =https"; fi
if grep '^curl' "$FAKE_LOG" | awk '{print $NF}' | grep -Ev "^https://(api\.github\.com/repos/$repo/releases/latest|github\.com/$repo/releases/download/v1\.1\.0/)" | grep -q .; then
  fail "only contacts the repo's release API and release assets"
else pass "only contacts the repo's release API and release assets"; fi
if find "$case_dir/state" -name 'tmp.*' | grep -q .; then fail "temp dir cleaned up"; else pass "temp dir cleaned up"; fi
perm=$(stat -c %a "$case_dir/state" 2>/dev/null || stat -f %Lp "$case_dir/state")
[ "$perm" = 700 ] && pass "state dir is private (0700)" || fail "state dir mode is $perm, want 700"

new_case placeholder; publish; cp "$committed" "$case_dir/pubkey"
if grep -q 'BEGIN PUBLIC KEY' "$committed"; then printf '# PLACEHOLDER\n' > "$case_dir/pubkey"; fi
run_updater; expect_refused "placeholder public key fails closed" "no release signing key"

new_case nokey; publish; rm -f "$case_dir/pubkey"; run_updater
expect_refused "missing public key fails closed" "no release signing key"

new_case unsigned; sig=no; publish; run_updater
expect_refused "release without SHA256SUMS.sig is refused" "SHA256SUMS.sig"

new_case nosums; sums=no; sig=no; publish; run_updater
expect_refused "release without SHA256SUMS is refused" "SHA256SUMS"

new_case wrongkey; sign_key="$work/evil.key"; publish; run_updater
expect_refused "signature from another key is refused" "signature"

new_case tamperedsums; tamper_sums=1; publish; unset tamper_sums; run_updater
expect_refused "SHA256SUMS altered after signing is refused" "signature"

new_case tampereddeb; tamper_deb=1; publish; unset tamper_deb; run_updater
expect_refused ".deb that does not match the signed checksum is refused" "checksum"

new_case downgrade; echo 1.2.0 > "$FAKE_STATE/installed"; publish; run_updater
if [ "$rc" -ne 0 ] && [ "$(installs)" = 0 ] && [ "$(installed)" = 1.2.0 ] && grep -qi downgrade "$case_dir/out"; then
  pass "older release is refused as a downgrade"
else fail "downgrade (rc=$rc installs=$(installs))"; sed 's/^/    | /' "$case_dir/out"; fi

new_case same; echo 1.1.0 > "$FAKE_STATE/installed"; publish; run_updater
if [ "$rc" -eq 0 ] && [ "$(installs)" = 0 ] && ! grep -q 'systemctl restart' "$FAKE_LOG"; then pass "same version is a quiet no-op"
else fail "same version (rc=$rc)"; sed 's/^/    | /' "$case_dir/out"; fi

new_case wrongpkg; pkg=evil-tool; publish; run_updater
expect_refused "unexpected package name is refused" "package"

new_case wrongarch; arch=armhf; publish; run_updater
expect_refused "package for another architecture is refused" "architecture"

new_case versiontag; version=1.1.1; publish; run_updater
expect_refused "package version that differs from the release tag is refused" "tag"

new_case prerelease; prerelease=true; publish; run_updater
expect_refused "pre-release is refused" "pre-release"

new_case twodebs; publish; extra_assets=("$dl/$tag/hamclock-pro_1.1.0-x_trixie_arm64.deb"); publish; run_updater
expect_refused "more than one matching .deb is refused" "exactly one"

new_case offsite; publish
jq '.assets[0].browser_download_url = "https://evil.example/hamclock-pro_1.1.0_trixie_arm64.deb"' \
  "$FAKE_NET/api.github.com/repos/$repo/releases/latest" > "$case_dir/j" && mv "$case_dir/j" "$FAKE_NET/api.github.com/repos/$repo/releases/latest"
run_updater; expect_refused "asset URL outside the repo's github.com releases is refused" "unexpected"
grep -q 'evil.example' "$FAKE_LOG" && fail "never contacts the off-site URL" || pass "never contacts the off-site URL"

new_case badname; deb_name='hamclock-pro_1.1.0;rm_trixie_arm64.deb'; publish; run_updater
expect_refused "asset name with unexpected characters is refused" "no stable release|unexpected"

new_case badtag; tag='v1.1.0/../x'; publish; run_updater
expect_refused "malformed tag is refused" "tag"

new_case installfail; deb_extra=$'FailInstall: yes\n'; publish
mkdir -p "$case_dir/state"; printf 'Package: hamclock-pro\nVersion: 1.0.0\nArchitecture: arm64\n' > "$case_dir/state/current.deb"
run_updater
if [ "$rc" -ne 0 ] && [ "$(installed)" = 1.0.0 ] && grep -q "apt-get install.*$case_dir/state/current.deb" "$FAKE_LOG" \
   && grep -q 'systemctl restart hamclock-kiosk.service' "$FAKE_LOG"; then
  pass "failed install rolls back to the previous package and restarts the kiosk"
else fail "install failure rollback (rc=$rc installed=$(installed))"; sed 's/^/    | /' "$case_dir/out"; fi

new_case unhealthy; echo 1.1.0 > "$FAKE_STATE/crashy"; publish
printf 'Package: hamclock-pro\nVersion: 1.0.0\nArchitecture: arm64\n' > "$case_dir/state/current.deb"
run_updater
if [ "$rc" -ne 0 ] && [ "$(installed)" = 1.0.0 ] && grep -qi 'not running' "$case_dir/out"; then
  pass "new version whose kiosk does not come up is rolled back"
else fail "health-check rollback (rc=$rc installed=$(installed))"; sed 's/^/    | /' "$case_dir/out"; fi

new_case kioskoff; touch "$FAKE_STATE/kiosk-down"; publish; run_updater
if [ "$rc" -eq 0 ] && [ "$(installed)" = 1.1.0 ] && ! grep -q 'systemctl restart' "$FAKE_LOG"; then
  pass "does not start a kiosk that was not running"
else fail "kiosk not running (rc=$rc)"; sed 's/^/    | /' "$case_dir/out"; fi

if command -v shellcheck >/dev/null; then
  shellcheck "$updater" && pass "shellcheck clean" || fail "shellcheck"
else
  echo "skip - shellcheck not installed"
fi

echo
[ "$failures" -eq 0 ] && { echo "all updater checks passed"; exit 0; }
echo "$failures updater check(s) failed"; exit 1
