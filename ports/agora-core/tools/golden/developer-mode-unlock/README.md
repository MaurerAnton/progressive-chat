# developer-mode-unlock golden stand

Parity pin for `ui/settings/DeveloperModeUnlockPolicy.kt` — the Android-style release unlock on
the About screen: seven taps, a remaining-taps hint for the last three, and a no-op once developer
mode is already on. The source compiles **verbatim from the app tree** (zero imports); the
canonical trace is frozen under `golden/` and the shipped port must reproduce every trace byte for
byte (`core/tests/developer_mode_unlock_parity_test.cpp`).

## What the port is

`agora::core::developer_mode_unlock` (`core/src/developer_mode_unlock.cpp`):

| Kotlin | C++ under test |
| --- | --- |
| `REQUIRED_TAPS` | `developer_mode_unlock::kRequiredTaps` |
| `advance(currentTapCount, alreadyEnabled)` | `developer_mode_unlock::advance` |
| `DeveloperUnlockResult.toString()` | `developer_mode_unlock::describe` |

## What is pinned

* **already enabled** — any tap count returns `(0, 0, ALREADY_ENABLED)`.
* **counter clamp** — the current count is coerced into `[0, 6]` before the increment, so negative
  counts behave like `0` (`-5 → 1`, `Int.MIN_VALUE → 1`) and counts of six or more unlock
  immediately (`6 → ENABLED`, `7 → ENABLED`, `Int.MAX_VALUE → ENABLED`).
* **success reset** — the seventh tap returns `(0, 0, ENABLED)`.
* **feedback threshold** — taps that leave more than three taps show `NONE` (`current 2 → remaining
  4`), exactly three or fewer show `REMAINING_TAPS` (`current 3 → remaining 3`, down to
  `current 5 → remaining 1`).
* **constants** — `REQUIRED_TAPS` is pinned directly.

## Running


`core/build/agora_scenarios developer-mode-unlock core/tools/golden/developer-mode-unlock` rewrites `scenarios/` deterministically (16 cases); rerun
