# CoinoCore V3 — Coino (CNO) v2.0.0.2

Updated Coino CNO build for modern Linux systems (Ubuntu 24.04+).

🇷🇺 [Русская версия](README.md)

## What's fixed

- `BN_num_bits` crash (OpenSSL 3.x) — migrated to `libsecp256k1`
- Stuck at block **395 740** — rebuilt with `-std=c++11` + `-reindex`
- OpenSSL 3.x compatibility via `OPENSSL_API_COMPAT`
- Network synchronization with Coino

## Run via Docker

    git clone https://github.com/<username>/coinocore.git
    cd coinocore/docker
    mkdir -p data
    cp Coino.conf.example data/Coino.conf
    docker compose up -d
    docker compose logs -f

## Build from source

    cd coinocore/docker
    docker build -f Dockerfile.build -t coinod-v3:latest .

## Verify

    docker exec coinod-v3 /usr/local/bin/Coinod -?

Expected: Coino version CNO-v2.0.0.2-g-bdb-gcc

## RPC

    curl -s --user <rpcuser>:<rpcpassword> \
      --data-binary '{"jsonrpc":"1.0","id":"c","method":"getblockcount","params":[]}' \
      -H 'Content-Type: application/json' \
      http://127.0.0.1:29399/

## Prebuilt binaries

See Releases.

## Security

Never publish wallet.dat, Coino.conf with real credentials, debug.log.

## License

MIT.
