# ports

Code taken from somewhere else whole, kept in this tree as it was found.

## agora-core

The portable C++ half of
[Agora](https://github.com/MaurerAnton/agora) -- `core/` as it stood at
`fe2821cc`: the logic layer Agora's Android program reads through JNI, with its
own parity binary (`tests/`, `agora_core_tests`) and the golden traces that
binary reads (`tools/golden/`). It is GPLv3 by the same author as this
AGPL-3.0 tree, and the two are distributed together under AGPL-3.0.

What was changed for this tree, and nothing else:

  - the four fixtures that lived in agora's *app* tree (the `o200k`
    vocabulary and its reference, the codec resource) moved under
    `fixtures/`, and `tests/CMakeLists.txt` points there;
  - curl and SQLite3 are found quietly, so a host without either builds
    everything that does not need it;
  - the POSIX submission receivers answer either spelling this repository
    uses for a phone (`ANDROID`, or CMAKE_SYSTEM_NAME as Agora's own build
    says it) and stay out of an Emscripten build, where nothing listens;
  - the X25519 field arithmetic picks its words by the architecture: the
    native 128-bit type where `__SIZEOF_INT128__` says there is one, the
    portable pair written in this file where there is not, with
    `AGORA_CORE_FORCE_PORTABLE_U128` and `agora_portable_crypto_test`
    making a host read that branch too.

It is taken forward as a whole snapshot (`git archive <sha> core` out of the
agora repository). Until this program's modules import it, what changes in
agora is not this tree's history -- only another snapshot lands here.
