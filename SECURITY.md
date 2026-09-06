# Security policy

## Scope and current status

CAN-Chaos-Fuzzer is an educational/research project for isolated, authorized CAN
benches. The current implementation is a low-rate known-frame demo with explicit
ARM and START, bounded execution and latched FAULT. It is not a certified safety
device. No released-version support window or security response SLA is established.
Reports about the current development code are welcome; older revisions may need
retesting against the current source.

The serial interface is not authenticated. Access to the host/serial port is a
trust boundary; do not expose it as an unauthenticated network service. CAN payloads
are untrusted input. Application SAFE prohibits new application-frame writes, not
controller acknowledgments or an already accepted transmission. See the
[documented timing and hardware limitations](docs/developer-guide.md).

## Reporting a vulnerability

Do not put sensitive reports, credentials, real-system captures or operational
exploit details in a public issue. If GitHub's **Security → Report a vulnerability**
option is enabled for this repository, use it for a private report. Its availability
has not been verified from this checkout. If it is unavailable, open an issue asking
only for a private reporting channel, without vulnerability details, and wait for
the maintainer to provide one. No private email address is published in this policy.

Include the affected commit/version, component, expected versus observed behavior,
impact, tool versions and a minimal host-test or isolated-bench description. Redact
identifiers and secrets. Maintainers should coordinate disclosure and fixes with
the reporter; this document does not promise a response deadline.

## Maintainer actions and release checks

- Configure a monitored private reporting route before advertising one. GitHub
  documents [how to enable private reporting](https://docs.github.com/en/code-security/how-tos/report-and-fix-vulnerabilities/configure-vulnerability-reporting/configure-for-a-repository).
- Review repository access and branch protections, and enable available secret
  scanning/push protection. These remote settings are not verified or changed by
  adding this file.
- Keep dependency updates reviewable and pinned; review upstream advisories and
  license notices before releasing. Host tooling executes code too.
- Keep credentials, private keys and sensitive captures out of commits. If a secret
  is exposed, revoke/rotate it; merely deleting the file is insufficient.
- Require the build, safety tests and relevant independent hardware evidence for
  safety changes. Preserve fault evidence and document residual limitations.

The implementation/test evidence and remaining physical acceptance checks are
recorded in the [developer guide](docs/developer-guide.md). A passing host suite is
not a penetration test, dependency audit or proof of hardware safety.
