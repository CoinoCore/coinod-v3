# CoinoCore V3 — Coino (CNO) v3.0.5

Cross-platform Coino (CNO) build — Linux, Windows, macOS.

📥 Windows GUI Release

Download the latest GUI wallet:  
👉 [CoinoCore V3.0.4 — Windows GUI](https://github.com/CoinoCore/coinod-v3/releases/tag/v3.0.4)

🇷🇺 [Русская версия](README.md)

## 📥 Download

| Platform | Version | File | Link |
|---|---|---|---|
| **Windows** (GUI) | v3.0.4 | `Coino-Qt-v3.0.4-win64.exe` (37 MB) | [Releases](https://github.com/CoinoCore/coinod-v3/releases/tag/v3.0.4) |
| **macOS** (GUI, Apple Silicon) | v3.0.5 | `CoinoCore-mac-v3.0.5-arm64.dmg` (42 MB) | [Releases](https://github.com/CoinoCore/coinod-v3/releases/tag/v3.0.5) |
| **Linux** (daemon) | v3.0.3 | build from source | [Instructions](#build-from-source) |

**Integrity check:** see `SHA256SUMS` in each release.

## ✨ Features

### GUI Wallet (Windows, macOS)
- Dark theme (macOS-inspired)
- Auto-localization (RU/EN)
- PoS mining from the GUI
- QR codes for addresses
- Multisig, wallet encryption
- Debug window with console and network graph

### Daemon (Linux)
- Full Coino node
- JSON-RPC API
- Docker support
- OpenSSL 3.x compatible

## 🐧 Linux / Docker

### Run via Docker

    git clone https://github.com/CoinoCore/coinod-v3.git
    cd coinod-v3/docker
    mkdir -p data
    cp Coino.conf.example data/Coino.conf
    # edit data/Coino.conf: rpcuser / rpcpassword
    docker compose up -d
    docker compose logs -f

### Build from source

    cd coinod-v3/docker
    docker build -f Dockerfile.build -t coinod-v3:latest .

### Verify

    docker exec coinod-v3 /usr/local/bin/Coinod -?

Expected: `Coino version CNO-v2.0.0.2-g-bdb-gcc`

## 🔧 Changelog (history)

- `BN_num_bits` crash (OpenSSL 3.x) — migrated to `libsecp256k1`
- Stuck at block **395 740** — rebuilt with `-std=c++11` + `-reindex`
- Static linking of `libsecp256k1` — Windows build without DLLs
- Qt5 statically built via MXE (cross-compilation)
- Native macOS build (Qt6, arm64)

## 🐛 Report an Issue

[github.com/CoinoCore/coinod-v3/issues](https://github.com/CoinoCore/coinod-v3/issues)

## 🔐 Security

Never publish `wallet.dat`, `Coino.conf` with real credentials, or `debug.log`.

## 📜 License

MIT.
