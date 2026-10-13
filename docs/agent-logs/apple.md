# Agent log: apple

Newest entries at the bottom. One entry per run: date UTC, what merged (PR links), what is next, anything blocked.

## 2026-10-13 (hourly run)
- Merged #60 (apple.yml: 45 min timeouts, Qt cache); macOS and iPad Simulator jobs green.
- F2 slice: `CredentialStore` interface, `FileCredentialStore` (0600 JSON, atomic write), `KeychainCredentialStore` (Security framework, `.mm`, Apple only), `makeDefaultCredentialStore()`. Linux tests cover the file store; the macOS job exercises the Keychain (skips if no usable keychain on the runner).
- Next: AppSettings wiring (coordinate with area A), then F3 (Info.plist, icon, universal build, dmg). Could not delete merged branch agent/apple-ci (API forbidden). #73 (other area) has merge conflicts; left alone.

## 2026-10-13 (second hourly run)
- No qa issues; no PRs of mine open (#95/#73 are other areas').
- F3 slice 1: `packaging/apple/Info.plist.in` (category, version from project, min macOS 12, hi-dpi) wired via `cmake/AppleBundle.cmake` (one include line in CMakeLists.txt, Apple-only); apple.yml lints the plist and builds an unsigned dmg with macdeployqt, uploaded as an artifact.
- Next: app icon, universal build, signing/notarization steps (secrets from #58, skip when absent).
