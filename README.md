# CoinoCore V3 — Coino (CNO) v2.0.0.2

Обновлённая сборка Coino CNO для современных Linux-систем (Ubuntu 24.04+).

🇬🇧 [English version](README.en.md)

## Что исправлено

- Краш `BN_num_bits` (OpenSSL 3.x) — миграция на `libsecp256k1`
- Застревание на блоке **395 740** — пересборка с `-std=c++11` + `-reindex`
- Совместимость с OpenSSL 3.x через `OPENSSL_API_COMPAT`
- Синхронизация с сетью Coino

## Запуск через Docker

```bash
git clone https://github.com/<username>/coinocore.git
cd coinocore/docker
mkdir -p data
cp Coino.conf.example data/Coino.conf
# отредактируйте data/Coino.conf: rpcuser / rpcpassword
docker compose up -d
docker compose logs -f


# CoinoCore V3 — Coino (CNO) v2.0.0.2

Обновлённая сборка Coino CNO для современных Linux-систем (Ubuntu 24.04+).

## Что исправлено

- Краш BN_num_bits (OpenSSL 3.x) — миграция на libsecp256k1
- Застревание на блоке 395 740 — пересборка с -std=c++11 + -reindex
- Синхронизация с сетью Coino

## Запуск через Docker

    git clone https://github.com/<username>/coinocore.git
    cd coinocore/docker
    mkdir -p data
    cp Coino.conf.example data/Coino.conf
    # отредактируйте data/Coino.conf: rpcuser / rpcpassword
    docker compose up -d
    docker compose logs -f

## Сборка из исходников

    cd coinocore/docker
    docker build -f Dockerfile.build -t coinod-v3:latest .

## Проверка

    docker exec coinod-v3 /usr/local/bin/Coinod -?

Ожидаем: Coino version CNO-v2.0.0.2-g-bdb-gcc

## Готовые бинарники

См. Releases.

## Безопасность

Никогда не публикуйте:
- wallet.dat
- Coino.conf с реальными rpcuser/rpcpassword
- debug.log, blocks/, chainstate/

## Лицензия

MIT.
