# chat-media-references-dao golden stand

Parity pin for `data/local/ChatMediaReferencesDao.kt`: the four bounded media-reference reads
inherited by the sole `ChatDao` — the per-conversation attachment pages, the combined keyset page
that dispatches to them, and the keyed conversation draft reference.

## Sources that compile verbatim

| Source | Why |
| --- | --- |
| `model/ChatMessage.kt` and its model dependencies | the enum types behind the converters |

## The platform boundary

| Path | Purpose |
| --- | --- |
| `platform/schema.sql` | the Room-generated DDL: all 15 `ChatEntity` columns, all 26 `MessageEntity` columns with both foreign keys and three indices, the `runs` reference table, and the 6-column `new_chat_persist` singleton. `embeddings` is not reachable from this DAO and is omitted. |

## What is pinned

All four members are operations, so no query text goes unpinned.

* **`getConversationDraftAttachmentReference`** — a keyed read, not a sweep: a JSON draft, a NULL
  draft (the row comes back with a null column), an empty-string draft, an object draft and the
  legacy `\|\|\|` format; an unknown id returns `null`.
* **`getConversationAttachmentFirstPage` / `getConversationAttachmentNextPage`** — identity pages
  over `messages` with no eligibility predicate: every row of the conversation comes back, empty
  image lists and missing metas included, ordered by `id` with the caller's `LIMIT`; the legacy
  `\|\|\|` image encoding surfaces as a two-element list through the app's converter.
* **`getConversationMessageAttachmentReferencesPage`** — the Kotlin interface default: a null
  cursor seeks the first page, a present cursor (including the empty string, which matches every
  id) seeks the next one; conversation scoping comes first (`c-1` vs `c-2` vs an unknown id).
* **Read-only**: the trailing table dump re-renders `conversations(id, draftAttachments)`,
  `messages(id, conversationId, images, attachmentMeta, toolCallJson)` and the full
  `new_chat_persist` row for every trace.

## Scenario format (`scenarios/*.json`)

```json
{"seed": {"conversations": [{"id": "c-1"}],
          "runs": [{"id": "run-c-1", "conversationId": "c-1"}],
          "messages": [{"id": "m-01", "conversationId": "c-1", "runId": "run-c-1",
                        "images": ["a.png"]}],
          "newChatPersist": []},
 "steps": [{"op": "getConversationAttachmentFirstPage", "conversationId": "c-1", "limit": 100}]}
```

Message fields default like the stand driver: `text=''`, `images='[]'`, `participant=MODEL`,
`status=SUCCESS`, `timestamp=0`, `runSequence=-1`. `agora_scenarios chat-media-references-dao`
writes four scenarios over purpose-built seeds.

## Trace format (`golden/*.trace`)

One JSON document per scenario: `steps` (each `{op, result}`, where a result is a projection
array, a projection object, `null`, or `{error, message}`) followed by `tables`, serialized exactly
like `Json.encodeToString`.

## Regenerating

