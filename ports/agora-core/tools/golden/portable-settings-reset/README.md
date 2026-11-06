# portable-settings-reset golden stand

Parity pin for `data/PortableSettingsReset.kt` — the portable allowlist a settings reset clears.

`PortableSettingsReset.kt` and `data/SettingsPreferenceSchema.kt` compile **verbatim from the app
tree**. The only platform substitute is `androidx.datastore.preferences.core` (the in-memory
preference store), which is the documented leaf stub of the `native-backup-secrets` stand.

## Layout

| Path | Purpose |
| --- | --- |
| `scenarios/*.json` | 1 case: the full settings schema, seeded and reset |
| `golden/*.trace` | Frozen Kotlin output |
| `agora_scenarios schema-keys <stand>` | Regenerates `SchemaKeys.kt` from `SettingsPreferenceSchema.kt` |
| `agora_scenarios gen-reset-keys <stand>` | Regenerates the C++ reset key list from `PortableSettingsReset.kt` |

## What is pinned

* **The portable allowlist**: the 94 preferences the reset removes, in the Kotlin source order.
* **The boundary**: the 37 preferences that survive — secrets, device-local model and font paths,
  sandbox state, lifecycle metadata, auto-backup configuration, per-conversation overrides and the
  embedding/ask-user state. A key that silently moved between the two lists would change what a
  backup carries, so the exact split is pinned.
* **The rewrite**: `model_provider_names_json` is not removed but rewritten to `{}`.
* The trace also carries the schema order (the removed keys as the schema lists them) so the two
  orders cannot be confused.

## Boundaries

* The reset writes through the preference-name contract, not through the typed DataStore keys; the
  names are the schema's names.
* The C++ list is **generated from the Kotlin source** (`agora_scenarios gen-reset-keys`), so the
  port cannot drift from the policy: the tool fails if a removed constant is not a schema key or is removed
  twice.
* The schema key list is generated as well (`agora_scenarios schema-keys`); run both modes after
  the schema or the policy changes.

## Running

```bash
# add --freeze to regenerate golden/
```
