# Changelog

All notable changes to this project.

## [v3.0.3] — 2026-10-12

### Added
- Windows x86-64 build via MXE (MinGW-w64 GCC 11.5.0)
- Coinod-v3.0.3-win64.exe (12.0 MB, PE32+, fully static)
- Dependencies: Berkeley DB 18.1, OpenSSL 4.0.3, Boost 1.92, libsecp256k1

### Changed
- CLIENT_VERSION_BUILD: 2 -> 3 (client version 2.0.0.3)
- DISPLAY_VERSION_BUILD: 2 -> 3 (display CNO-v2.0.0.3-...)
- BUILD_DATE fix: use __DATE__ instead of empty GIT_COMMIT_DATE

### Fixed
- Cryptographic RNG in key.cpp (inherited from v3.0.2):
  - rand() -> RAND_bytes()
  - Correct recovery id (recid = (nV - 27) & 3)
  - Proper fCompressedSig handling
  - Error checking on secp256k1_ec_pubkey_serialize

## [v3.0.2] — 2026-10-09

### Fixed
- Cryptographic RNG in key.cpp:
  - rand() -> RAND_bytes() (OpenSSL CSPRNG)
  - Added error handling
  - Correct recovery id computation

## [v3.0.1] — 2026-09-30

### Fixed
- BN_num_bits crash (OpenSSL 3.x) — migrated to libsecp256k1

## [v3.0.0] — 2026-09-28

### Added
- Initial release
- libsecp256k1 migration
