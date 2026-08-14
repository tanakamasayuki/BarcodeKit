# Documentation guide

> 日本語: [README.ja.md](README.ja.md)

Which document to read, and in what order.

**Language policy has three tiers. Japanese is the source of truth.**

| Tier | Languages | Documents |
| --- | --- | --- |
| For users | JA + EN | [../README.md](../README.md), [../examples/README.md](../examples/README.md), [../tests/README.md](../tests/README.md) |
| Settled specification | JA + EN | [FORMATS.md](FORMATS.md) |
| Internal notes | Japanese only | `REQUIREMENTS.ja.md`, `CORE_DESIGN.ja.md`, `DECISIONS.ja.md`, `TEST_PLAN.ja.md`, `DEVELOPMENT_PLAN.ja.md`, `MANUAL_TEST.ja.md` |

## Start here

| Goal | Document |
| --- | --- |
| Learn what the library does and see a working sketch | [../README.md](../README.md) |
| **Choose a format; look up characters, lengths, check digits** | **[FORMATS.md](FORMATS.md)** |
| Find a sketch for your hardware | [../examples/README.md](../examples/README.md) |
| Run the tests | [../tests/README.md](../tests/README.md) |

## Documents

**For users**

- [FORMATS.md](FORMATS.md) — Supported formats reference: accepted characters, lengths, check digit handling, widths, recommended quiet zones and required buffer sizes. The document users reach for most.

**Design (Japanese only)**

1. `REQUIREMENTS.ja.md` — What the library is for and where its responsibility ends. Target environments, target users, non-goals.
2. `CORE_DESIGN.ja.md` — API shape, memory model, output model, per-format implementation notes, drawing helpers.
3. `DECISIONS.ja.md` — Ledger of settled design decisions, with the reasoning and the options that were rejected.

**Process (Japanese only)**

- `TEST_PLAN.ja.md` — Test strategy, directory layout, the sketch↔pytest output protocol, provenance of known vectors.
- `MANUAL_TEST.ja.md` — Manual verification on real hardware: the one thing that is not automated is whether a real scanner can read the output.
- `DEVELOPMENT_PLAN.ja.md` — Current state, v0.1.0 goals, implementation order, open questions.

**Archive**

- `archive/memo.ja.md` — The original requirements draft (Japanese). What changed from it is recorded in `DECISIONS.ja.md` §3.
