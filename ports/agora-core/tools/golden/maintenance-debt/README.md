# maintenance-debt golden stand

Parity pin for `data/local/MaintenanceDebt.kt`: the debt queue behind every background
maintenance pass — enqueue/claim/complete/release with compare-and-set leases, the orphan
sweeping queries, the run-branch conversation paging, and the run-branch projection repair.

`MaintenanceDebt.kt`, `data/local/RunBranchSelectionIntegrity.kt`, `data/local/RunEntity.kt` and
`model/RuntimeIdentity.kt` compile **verbatim from the app tree**. No extraction is needed: the
debt file carries its own entity.

## The platform boundary

Room's annotation processor is the replaced platform, exactly as in the semantic-index-ledger
stand:

| Path | Purpose |
| --- | --- |
| `platform/schema.sql` | The generated DDL: `maintenance_debt` (Migration27To28), `runs` (Migration16To17 — both foreign keys, the unique `(conversationId, activeSlot)` index where SQLite permits multiple nulls), the `embeddings`/`messages` orphan surface, and the documented `conversations` subset |

## Two documented boundaries in the traces

* **`System.currentTimeMillis()`** inside `repairRunBranches` — the value it writes into
  `conversations.dataChangedAt` is masked as `<clock>` when it falls in the wall-clock window,
  and the port takes the timestamp as a parameter. Scenario `11` seeds a
  `dataChangedAt` above any plausible clock, so the `MAX(dataChangedAt + 1, :at)` branch is
  pinned verbatim instead of masked.
* **`ChatEntity`** is the stand-local conversation subset (`id`, `selectedRunBranchesJson`)
  plus the `dataChangedAt` column the compare-and-set query writes.

## Layout

| Path | Purpose |
| --- | --- |
| `scenarios/*.json` | 13 scenarios, 97 steps |
| `golden/*.trace` | Frozen Kotlin output, one per scenario |
| `agora_scenarios maintenance-debt <stand>` | Deterministic scenario generator |

## What is pinned

* **Entity validation**: blank kind/identity, unknown state, `revision <= 0`, and the
  claim/claimedAt consistency pair all fail with `IllegalArgumentException("Failed requirement.")`.
* **`enqueue`**: insert-then-renew handshake — an ignored insert (`-1`) must find the row to
  renew, and the ledger must be readable afterwards.
* **`claim`**: candidate order `(updatedAt, kind, identity)`, stale-lease revival without a
  revision bump, the batch limit `1..100`, and the in-transaction re-read checks
  (`"Maintenance debt claim changed inside its transaction"`).
* **`complete` / `release`**: `requireClaim` plus revision/claimId compare-and-set, so a lease
  invalidated by a newer revision (`sql` op bumps it mid-scenario) completes nothing.
* **Orphan sweeping**: `compact_%` ids and missing message rows are both orphans; the id-cursor
  paging and the empty-id list behave like Room's `IN` expansion.
* **`repairRunBranches`**: missing conversation and null JSON return false; malformed JSON
  decodes to null and repairs to `{}`; invalid edges are dropped in input order (including the
  `"null"` key round trip and duplicate-key last-wins-first-position map semantics); an unchanged
  projection never writes (`RunBranchSelectionIntegrity` is the already-ported port).
* **`RunEntity`**: enum columns resolve by constant name (`"No enum constant
  com.newoether.agora.model.RunStatus.BOGUS"`) and the init requires fire on corrupt rows read
  back from the database.

## Scenario format (`scenarios/*.json`)

```json
{"seed": {"conversations": [{"id": "c1", "selectedRunBranchesJson": "{\"r1\":\"r2\"}",
                             "dataChangedAt": 100}]},
 "steps": [{"op": "enqueue", "kind": "RUN_BRANCHES", "identity": "c1", "updatedAt": 5}]}
```

`seed` fills `conversations`, `runs`, `messages` and `embeddings` (runs accept exactly the
column shapes `RunEntity` declares; invalid rows are allowed on purpose so the read path pins
the validation). Steps cover every DAO method; `sql` runs a raw statement (it is also how the
scenario simulates a concurrent revision bump).

## Trace format (`golden/*.trace`)

One JSON document per scenario: `steps` (each `{op, result}`) followed by `tables`, a full dump
of `maintenance_debt`, `conversations`, `runs`, `embeddings` and `messages` in declared column
order, serialized exactly like `Json.encodeToString`.

## Regenerating

```bash
../../../build/agora_scenarios maintenance-debt .   # rewrite scenarios
```

