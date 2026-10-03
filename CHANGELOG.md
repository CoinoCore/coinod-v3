# Changelog

All notable changes to this project.

## [v3.0.2] — 2026-10-03

### Fixed
- **Stuck at block 395 240** — `read txPrev failed`
- Added PIVX-style fallback in `CheckProofOfStake`:
  - Fallback to `GetTransaction` when `ReadFromDisk` fails
  - Block lookup via `mapBlockIndex`
  - Read block fully (not by `txindex.pos`)
  - Search `txPrev` in `block.vtx`
  - Compute `nTxPos` by serializing block transactions

### Changed
- `CheckProofOfStake` no longer calls `DoS(1)` on missing `txPrev` — just returns `false` (block is deferred)

### Build
- Compiler: g++ 13.3.0 (Ubuntu 24.04)
- `libsecp256k1` v0.4.1 with `--enable-module-recovery`
- OpenSSL 3.0.13

## [v3.0.1] — 2026-09-30

### Fixed
- `BN_num_bits` crash (OpenSSL 3.x) — migrated to `libsecp256k1`

### Known issues
- **Stuck at block 395 240** during sync from scratch
- Workaround: copy `database/` from a synced node

## [v3.0.0] — 2026-09-28

### Added
- Initial release
- `libsecp256k1` migration
