# CoinoCore V3 — Coino (CNO) v3.0.5

Мультиплатформенная сборка Coino (CNO) — Linux, Windows, macOS.

## 📥 Windows GUI Release

Download the latest GUI wallet:  
👉 [CoinoCore V3.0.4 — Windows GUI](https://github.com/CoinoCore/coinod-v3/releases/tag/v3.0.4)

🇬🇧 [English version](README.en.md)

## 📥 Скачать

| Платформа | Версия | Файл | Ссылка |
|---|---|---|---|
| **Windows** (GUI) | v3.0.4 | `Coino-Qt-v3.0.4-win64.exe` (37 МБ) | [Releases](https://github.com/CoinoCore/coinod-v3/releases/tag/v3.0.4) |
| **macOS** (GUI, Apple Silicon) | v3.0.5 | `CoinoCore-mac-v3.0.5-arm64.dmg` (42 МБ) | [Releases](https://github.com/CoinoCore/coinod-v3/releases/tag/v3.0.5) |
| **Linux** (демон) | v3.0.3 | сборка из исходников | [Инструкция](#сборка-из-исходников) |

**Проверка целостности:** см. `SHA256SUMS` в каждом релизе.

## ✨ Возможности

### GUI-кошелёк (Windows, macOS)
- Тёмная тема (macOS-inspired)
- Автолокализация (RU/EN)
- PoS-майнинг из интерфейса
- QR-коды для адресов
- Мультиподпись, шифрование кошелька
- Отладчик с консолью и сетевым графиком

### Демон (Linux)
- Полная нода Coino
- JSON-RPC API
- Поддержка Docker
- Совместимость с OpenSSL 3.x

## 🐧 Linux / Docker

### Запуск через Docker

    git clone https://github.com/CoinoCore/coinod-v3.git
    cd coinod-v3/docker
    mkdir -p data
    cp Coino.conf.example data/Coino.conf
    # отредактируйте data/Coino.conf: rpcuser / rpcpassword
    docker compose up -d
    docker compose logs -f

### Сборка из исходников

    cd coinod-v3/docker
    docker build -f Dockerfile.build -t coinod-v3:latest .

### Проверка

    docker exec coinod-v3 /usr/local/bin/Coinod -?

Ожидаем: `Coino version CNO-v2.0.0.2-g-bdb-gcc`

## 🔧 Что исправлено (история)

- Краш `BN_num_bits` (OpenSSL 3.x) — миграция на `libsecp256k1`
- Застревание на блоке **395 740** — пересборка с `-std=c++11` + `-reindex`
- Статическая линковка `libsecp256k1` — Windows-билд без DLL
- Qt5 статически собран через MXE (кросс-компиляция)
- Нативный macOS-билд (Qt6, arm64)

## 🐛 Сообщить о проблеме

[github.com/CoinoCore/coinod-v3/issues](https://github.com/CoinoCore/coinod-v3/issues)

## 🔐 Безопасность

Никогда не публикуйте:
- `wallet.dat`
- `Coino.conf` с реальными `rpcuser`/`rpcpassword`
- `debug.log`, `blk*.dat`, `chainstate/`

## 📜 Лицензия

MIT.
