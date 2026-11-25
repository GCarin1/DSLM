# Change 0016-tiny1m-in-the-package — tiny1m in the package

- **Status:** applied
- **Applied:** 2026-11-25
- **Date:** 2026-11-25
- **Owner:**
- **Lane:** chore (confident; signals: readme)
- **Affects specs:** pc-toolchain, fixed-point-inference

<!--
Optional, and usually absent. The closing docs gate reads COMMAND and FLAG
names out of the prose below and asks for documentation when it finds any.
It cannot tell a change from a mention: explaining an effect, or writing a
Scope boundaries line about what this deliberately does NOT touch, names
things just as loudly as changing them would.

When that happens, say so on the record instead of forcing the close:

- **Documented surface:** n/a — names two commands to explain an effect; alters neither

`none` reads the same as `n/a`, and a BARE one silences nothing — the
reason is the declaration.
-->

## Why

Ship shibatch/tiny1m in the official package: pinned in tools/models.json, quantized to int8, its own golden outputs and drift bound, staged and packaged next to stories260K, documented in README and docs, released as v1.1.0

## What

The owner tested shibatch/tiny1m on the DS through the model picker and asked to ship it. tools/models.json now pins it (fp32 source 3.8 MB, sha256 80ba15bd..., the same tok512 tokenizer) with author and license for every model; the toolchain quantizes, stages and packages all packaged models (tiny1m int8 only, since its fp32 does not fit); THIRD_PARTY.txt lists each. tiny1m gets its own goldens; the drift bound against runq.c becomes per model (64 for stories260K, 6 for tiny1m, whose measured minimum is 7). README and the user guide explain both models. Released as v1.1.0; stories260K stays the default.

## Scope boundaries

No engine change. LittleKedi is a separate milestone.

## Verification

<!--
How you will know the change is correctly applied. Use checkboxes: every
box here is a claim that must be PROVEN before the change is done.
`doctrina change archive` refuses to archive while any box below is
unchecked (pass --force to archive anyway and record the gap). Distinguish
"task marked done" from "verification passed" — link the evidence.
-->

- [x] Automated checks pass (`doctrina verify`, or the project's typecheck/test/build).
- [x] The affected spec's acceptance criteria are met and cite their evidence (`doctrina coverage`).

## Open questions

- Default model: stories260K (speed) or tiny1m (quality); kept stories260K, one line in DEFAULT_INI and ini.c to change.

<!-- doctrina:applied-deltas specs/fixed-point-inference/delta.md=d90b5a5b,fa95133c,ff365ae8,9a870797 specs/pc-toolchain/delta.md=d90b5a5b,970f1b04,66b8a956 -->
