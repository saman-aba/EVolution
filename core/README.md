# Core

`evolution_core` owns technology-neutral value types and semantic contracts. Public headers live under `include/evolution/core/`; implementation files and private details live under `src/`.

## Conventions

- All project types live in `namespace evolution` or a concept-owned nested namespace.
- Public type names use `PascalCase`; functions and variables use `snake_case`; constants use `snake_case`.
- `struct` represents transparent aggregates; `class` protects invariants or representation.
- Semantically distinct identifiers and values use strong types when interchange would be unsafe.
- Public API stability is marked with `EVOLUTION_STABLE_API` or `EVOLUTION_EXPERIMENTAL_API`; the current Core API is experimental.
- Expected operational failure uses `evolution::Result<T>` and immutable `evolution::Error` values.
- Public headers must not expose private implementation, synchronization, allocator, or vendor types.
