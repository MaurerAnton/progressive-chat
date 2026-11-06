# conch-network golden stand

Parity pin for the shell transport policy in `util/ConchNetwork.kt` and the bounded HTTP reads in
`api/BoundedWireRead.kt`.

## Sources that compile verbatim

| Source | Why |
| --- | --- |

## Substituted platform halves

| Substitute | Contract |
| --- | --- |

Sockets, TLS, redirects, proxy selection, connection reuse and the actual OkHttp call stay on the
platform side. The core port owns the policy and consumes a `Transport`/`WireSource`; the app
adapter fills those with OkHttp/okio. `readBoundedWireText` returns the UTF-8 bytes; the adapter
decodes them like okio's `readUtf8` (the traces use valid UTF-8 bodies).

## What the traces pin

- the client policy: `retryOnConnectionFailure=false`, `followRedirects=false`,
  `followSslRedirects=false`, pool `5` / `45s`;
- GET: method, URL, caller headers after case-insensitive replacement, `timeoutMillis=30000`,
  no body/content type;
- POST: `application/json` request body, default `timeoutMillis=130000`, explicit timeouts, the
  `require(timeout > 0)` rejection before any call, and caller headers overriding the body media
  type header;
- response: 200..299 success, signature lookup case-insensitively with the last value winning,
  missing signature as `""`, and `Response.use` closing on both success and read failure;
- body limits: 64 KiB for GET success/failure and for any failure response, 40 MiB for POST
  success; at-limit acceptance and limit+1 rejection for both; the exact
  `Response exceeds the N-byte transport limit` message;
- `readBoundedWireText`: positive-limit requirement, at-limit acceptance, over-limit rejection,
  UTF-8 byte counting, `declaredSize` larger than the materialised body;
- `readBoundedWireLine`: clean-EOF `null`, LF and CRLF stripping, `Event exceeds the N-byte
  transport limit`, `Incomplete encrypted event line`, and the positive-limit requirement.

## Usage

```sh
```
