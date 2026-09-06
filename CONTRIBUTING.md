# Contributing

Keep changes within the current milestone. Milestones 1A/1B cover builds and
control/safety hardening; hardware acceptance, the simulator and strategies are
separate work. Read [SECURITY.md](SECURITY.md), the
[developer guide](docs/developer-guide.md) and [serial protocol](docs/protocol.md).

## Coding conventions

- Use the Arduino framework and the existing component boundaries. Keep peripheral
  APIs in the CAN driver; enforce safety at the send boundary.
- C++17 is the current build baseline. Follow `.clang-format` (clang-format
  20.1.8), `.editorconfig` and `.gitattributes`: two-space C/C++ indentation, four-space Python
  indentation, UTF-8 and LF. Do not format framework/vendor/generated code.
- Use `PascalCase` for types, `camelCase` for functions/variables, `kConstantName`
  for constants and a trailing underscore for private members. Include units in
  names such as `durationMs` and `bitrate` where ambiguity matters.
- Initialize state explicitly; use fixed-width unsigned types for wire values and
  timestamps, `size_t` for buffer sizes and scoped enums for state. Check bounds
  and overflow before arithmetic or copying. Use unsigned elapsed-time arithmetic
  for the bounded `millis()` intervals; do not compare absolute deadlines across wrap.
- Use braces for control-flow bodies. Keep checks and side effects readable; avoid
  multiple independent statements on one line. Comments explain intent, contracts
  and limitations, rather than narrating obvious statements.
- Firmware hot paths must have bounded work and storage. Do not add dynamic
  allocation, recursion, unbounded retry loops or blocking delays there. Host test
  doubles may use STL containers. Document unavoidable framework blocking behavior.
- Check API results against the pinned implementation; do not infer semantics from
  another board/library. Distinguish queued/accepted work from hardware completion.
- Do not weaken safety gates, limits or fault latching to make a test pass. Cover
  rejection paths and cancellation, not only valid inputs. Never add automatic
  transmission on boot or reconnect.

## File headers and API documentation

Project-owned C/C++ files use a short header:

```cpp
// SPDX-License-Identifier: MIT
/**
 * @file
 * @brief One sentence describing this file's responsibility.
 */
```

Headers use `#pragma once`. Python files use a license comment and a module
docstring. Keep existing copyright/attribution notices. Do not invent an author,
copyright holder or dates, and do not apply the project license to third-party
code. Preserve upstream notices when importing code and record its source/version.
SPDX labels do not replace the root LICENSE or dependency notices; see
[SPDX guidance](https://spdx.dev/learn/handling-license-info/).

Document public interfaces in their declarations with Doxygen-compatible `@brief`,
`@param`, `@return`, `@pre`, `@post` or `@note` tags when useful. State units,
ownership/lifetime, permitted states, failure side effects and blocking behavior.
Do not duplicate the complete contract in both declaration and implementation.
These comments are readable in source; generated Doxygen documentation is not
currently configured or claimed as validated.

Use `TODO(issue-reference): reason` for tracked deferred work; create/link the
issue when one exists rather than inventing an issue number. A safety defect is
not resolved by labeling it TODO. Keep unresolved assumptions in the developer
guide/hardware acceptance list. Do not add per-file `@author`, `@date`, `@version`
or edit-history blocks; Git and the changelog hold that history.

## Review and validation

For firmware changes, run and record:

```text
pio run -e uno_r4_wifi -t clean
pio run -e uno_r4_wifi -e sniffer
pio test -e host
git diff --check
```

Use `clang-format --dry-run --Werror` on the changed project-owned `.h`, `.cpp`
and `.ino` files; use `clang-format -i` to apply formatting. Build artifacts and
portable tools belong in ignored `.pio/`, not in source control. The host suite
uses warnings as errors; the embedded framework may suppress warnings in its own
build flags. Do not claim a warning-free firmware audit from a successful build.

Review diffs for credentials, captures identifying real systems, unexpected
dependencies, changed safety behavior and accidental hardware changes. `.gitignore`
is not a secret scanner and does not remove previously committed data. Do not
disable TLS checks to download dependencies. Use official sources, pinned versions
and verify published artifact checksums/signatures where supplied; record provenance.
Version pins alone are not a dependency vulnerability audit.

Describe the problem, final behavior, tests and remaining limitations in the PR.
Update protocol documentation with command changes and CHANGELOG.md with meaningful
changes. Separate host-test evidence from physical bench evidence. No claim of
MISRA, CERT C++, functional-safety certification or production readiness is made
by adopting these local conventions.

## Commits and release tags

Use focused imperative commit subjects, optionally `fix(parser): reject overflowing
arguments`, `test(safety): cover fault cancellation`, or `docs: clarify stop limits`.
Commit messages explain the reason when it is not clear from the diff.

Use annotated `vMAJOR.MINOR.PATCH` Git tags for deliberate releases, with optional
prerelease suffixes such as `v0.1.0-rc.1`. Follow
[Semantic Versioning 2.0.0](https://semver.org/spec/v2.0.0.html); identify the serial
protocol and documented firmware interfaces as the compatibility surface. Before
1.0, explicitly document any breaking changes. Milestone names are planning labels,
not evidence that a release has passed hardware acceptance.

Before tagging, review the exact commit, move applicable changelog entries out of
Unreleased, record tool versions/test results and identify unperformed acceptance
checks. Do not move a published release tag; issue a new version. Sign tags when a
maintainer signing identity is configured; do not fabricate an identity or claim a
signature was verified. This policy creates no tag or release automatically.
