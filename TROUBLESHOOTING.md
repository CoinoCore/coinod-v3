# Troubleshooting Guide

This document covers common issues encountered when running a Coinod v3 node
and how to resolve them.

If you run into a problem not listed here, please open an issue with:

- Your OS and version
- Node version (from `getinfo` → `version`)
- Relevant log excerpts (from `debug.log` or `-printtoconsole` output)
- Output of `getblockcount` and `getconnectioncount`

---

## Table of Contents

1. [Node stuck at a fixed height with "check proof-of-stake failed"](#1-node-stuck-at-a-fixed-height-with-check-proof-of-stake-failed)
2. [RPC returns 401 Unauthorized](#2-rpc-returns-401-unauthorized)
3. [RPC does not respond / connection refused](#3-rpc-does-not-respond--connection-refused)
4. [Node cannot connect to any peers](#4-node-cannot-connect-to-any-peers)
5. [How to find the correct config file](#5-how-to-find-the-correct-config-file)
6. [Running the node on macOS via Wine / Highball](#6-running-the-node-on-macos-via-wine--highball)
7. [How to safely back up the wallet](#7-how-to-safely-back-up-the-wallet)
8. [Full resync from scratch](#8-full-resync-from-scratch)
9. [General "do not do this" list](#9-general-do-not-do-this-list)

---

## 1. Node stuck at a fixed height with "check proof-of-stake failed"

### Symptoms

- `getblockcount` returns the same height indefinitely (hours, days).
- Log repeats:
  ```
  WARNING: ProcessBlock(): check proof-of-stake failed for block <hash>
  ```
- `getpeerinfo` shows peers at height 817,000+ while your node is at 395,000.
- Connections are alive (10–11 peers), but height does not advance.
- After a restart the node may jump +500 / +1000 blocks, then stall again.

### Root cause

**A stale `peers.dat`.** The node keeps retrying dead or forked peers that
serve blocks from the wrong chain. Those blocks fail proof-of-stake
validation against your local chain, so the node rejects them and stays put.

This is **not** a peer problem. The peers are on the canonical chain.
The problem is local: the node cannot find the correct branch because it
holds onto outdated peer addresses.

### Fix (step by step)

#### 1. Back up the wallet (mandatory if `wallet.dat` holds coins)

```bash
cp "$DATADIR/wallet.dat" "$HOME/wallet-backup-$(date +%Y%m%d-%H%M).dat"
```

#### 2. Stop the node

```bash
pkill -9 -f 'Coinod-v3'
pkill -9 -f wineserver   # macOS/Wine only
```

#### 3. Move `peers.dat` aside

```bash
mv "$DATADIR/peers.dat" "$DATADIR/peers.dat.bak-$(date +%Y%m%d-%H%M)"
```

#### 4. Restart the node

Windows:
```
Coinod-v3.0.3-win64.exe -datadir=<path-to-data> -printtoconsole
```

macOS via Wine (variables `$WINE` and `$COINOD` are defined in
[section 6](#6-running-the-node-on-macos-via-wine--highball)):
```bash
"$WINE" "$COINOD" \
  -datadir='Z:\Users\<user>\<path>\data' \
  -printtoconsole
```

#### 5. Watch the log

You should see:
```
received block <hash>
ProcessBlock: ACCEPTED
SetBestChain: new best=<hash> height=399051
```

### How to verify the node is actually making progress

Compare height over time:

```bash
while true; do
  H=$(curl -s --user "$RPCUSER:$RPCPASS" \
    --data-binary '{"jsonrpc":"1.0","id":"1","method":"getblockcount","params":[]}' \
    -H 'content-type: text/plain;' "http://127.0.0.1:$RPCPORT/" \
    | python3 -c "import sys,json;print(json.load(sys.stdin)['result'])")
  echo "$(date +%H:%M:%S)  height=$H"
  sleep 60
done
```

If the height advances every minute — the node is syncing.
If it stays the same for more than 10 minutes — see above.

### If it does not work on the first try

**Repeat steps 2–4 up to 3–4 times.** The node needs to cycle through
peers until it lands on a healthy set.

Real-world observed behaviour (macOS + Highball + Wine):

| Stage | Action | Result |
|-------|--------|--------|
| 1 | Node stuck at 395,240; PoS failures | Sync dead |
| 2 | Restart node | +500 blocks → 395,740, stalled |
| 3 | Remove `peers.dat` + restart | +1000 blocks → 396,740, stalled |
| 4 | Repeat cycles + RPC checks | **Sync resumed: 399,050 → 403,000+** |

The key insight: **`peers.dat` may need to be cleared several times**
before the node finds healthy peers on the canonical chain.

### If the fix still does not help — `invalidateblock`

Roll the chain back 100–500 blocks below the stall point:

```bash
# Current height
H=$(curl -s --user "$RPCUSER:$RPCPASS" \
  --data-binary '{"jsonrpc":"1.0","id":"1","method":"getblockcount","params":[]}' \
  -H 'content-type: text/plain;' "http://127.0.0.1:$RPCPORT/" \
  | python3 -c "import sys,json;print(json.load(sys.stdin)['result'])")

# Hash of the block 100 below current height
HASH=$(curl -s --user "$RPCUSER:$RPCPASS" \
  --data-binary "{\"jsonrpc\":\"1.0\",\"id\":\"1\",\"method\":\"getblockhash\",\"params\":[$((H-100))]}" \
  -H 'content-type: text/plain;' "http://127.0.0.1:$RPCPORT/" \
  | python3 -c "import sys,json;print(json.load(sys.stdin)['result'])")

# Invalidate the block
curl -s --user "$RPCUSER:$RPCPASS" \
  --data-binary "{\"jsonrpc\":\"1.0\",\"id\":\"1\",\"method\":\"invalidateblock\",\"params\":[\"$HASH\"]}" \
  -H 'content-type: text/plain;' "http://127.0.0.1:$RPCPORT/"
```

Then clear `peers.dat` again and restart.

### If that does not help — full resync

See [section 8](#8-full-resync-from-scratch).

---

## 2. RPC returns 401 Unauthorized

### Symptoms

```
< HTTP/1.0 401 Authorization Required
< Server: Coino-json-rpc/CNO-v2.0.0.3-g-bdb-gcc
< WWW-Authenticate: Basic realm="jsonrpc"
```

### Cause

The `rpcuser` / `rpcpassword` you are sending do not match what the
running node has in memory.

The most common reason: **you are reading the wrong config file.**
A node started with `-datadir=<path>` reads `Coino.conf` from that
directory only — not from `%APPDATA%\CoinoV2\` or any other location.

### Fix

#### Find the actual config

Look at the running process command line:

```bash
# Linux/macOS
ps auxww | grep -i 'Coinod\|CoinoCore' | grep -v grep
```

Look for `-datadir=` or `-conf=`. The config file is in that directory:

```bash
grep -vE 'rpcpassword' "<datadir>/Coino.conf"
```

#### Use the credentials in `curl`

```bash
export CONF="<datadir>/Coino.conf"
export RPCUSER="$(grep -E '^rpcuser=' "$CONF" | cut -d= -f2- | tr -d '[:space:]')"
export RPCPASS="$(grep -E '^rpcpassword=' "$CONF" | cut -d= -f2- | tr -d '[:space:]')"
export RPCPORT="$(grep -E '^rpcport=' "$CONF" | cut -d= -f2- | tr -d '[:space:]')"

curl -s --user "$RPCUSER:$RPCPASS" \
  --data-binary '{"jsonrpc":"1.0","id":"1","method":"getinfo","params":[]}' \
  -H 'content-type: text/plain;' "http://127.0.0.1:$RPCPORT/"
```

If the password contains shell-special characters (`$`, `!`, `&`, `#`),
wrap the whole `user:pass` pair in **single quotes**:

```bash
curl -s --user 'myuser:pa$$word!' ...
```

---

## 3. RPC does not respond / connection refused

### Symptoms

- `curl` returns nothing.
- `nc -z -v 127.0.0.1 29299` → connection refused.
- Python JSON parser errors out with `Expecting value: line 1 column 1 (char 0)`.

### Causes

1. The node is not running.
2. RPC is disabled in `Coino.conf` (`server=0`).
3. The port is not what you think it is.

### Fix

#### Verify the node is running

```bash
ps auxww | grep -i 'Coinod\|CoinoCore' | grep -v grep
```

#### Verify the port is listening

```bash
# Linux
sudo ss -tlnp | grep -E '2929[0-9]'

# macOS
sudo lsof -i -P -n | grep -E '2929[0-9]'
```

#### Verify `server=1` in the config

```bash
grep '^server=' "<datadir>/Coino.conf"
```

If `server=0`, edit the file and change it to `server=1`, then restart the
node. On restart it will pick up the new setting.

#### Always confirm the port from the config

```bash
grep -E '^rpcport=' "<datadir>/Coino.conf"
```

Default in the sample config shipped with v3.0.3 is **29299**.

---

## 4. Node cannot connect to any peers

### Symptoms

- `getconnectioncount` returns `0`.
- Log repeats:
  ```
  connection timeout
  trying connection <ip>:29293 lastseen=X.Xhrs
  ```

### Cause

Same root cause as issue #1 — a stale `peers.dat` full of dead addresses.

### Fix

Same procedure as [section 1](#1-node-stuck-at-a-fixed-height-with-check-proof-of-stake-failed):
stop the node, move `peers.dat` aside, restart.

Additionally, make sure the P2P port is open on your firewall and
reachable from the internet if you want inbound connections:

- Default P2P port: **29293** (from `Coino.conf` → `port=`)
- If the node has not received a version message from any peer in
  `getpeerinfo`, it is not connected to the network at all — check DNS
  resolution and outbound firewall rules.

---

## 5. How to find the correct config file

The config file is always in the directory passed via `-datadir=`.

On Windows:
```
<datadir>\Coino.conf
```

On macOS via Wine / Highball, the `Z:` drive maps to your home directory:
```
-datadir=Z:\Users\<user>\<path>\data
→ real path: /Users/<user>/<path>/data/Coino.conf
```

To find it from the shell:

```bash
find "$HOME" -name 'Coino.conf' 2>/dev/null
```

To find the datadir of the **running** process:

```bash
ps auxww | grep -i 'Coinod\|CoinoCore' | grep -v grep
# look for -datadir=... in the output
```

Do **not** assume the config in `AppData/Roaming/CoinoV2/` is the one
in use. If the process was started with an explicit `-datadir=`, that
path wins.

---

## 6. Running the node on macOS via Wine / Highball

This is a **supported setup for testing only**, not for storing real
coins.

### Requirements

- macOS (Intel or Apple Silicon)
- [Highball](https://github.com/HighballApp/Highball) with a Wine engine
  (tested: `x64-sikarugir10.0_6-r26`)

### Start the node from the terminal

```bash
export WINEPREFIX="$HOME/Library/Application Support/Highball/bottles/Games"
export WINE="$HOME/Library/Application Support/Highball/engines/x64-sikarugir10.0_6-r26/engine/bin/wine"
export COINOD="$HOME/COINOM2Windows/CoinoCore-v3.0.3/Coinod-v3.0.3-win64.exe"
export DATADIR='Z:\Users\<user>\COINOM2Windows\CoinoCore-v3.0.3\data'

"$WINE" "$COINOD" -datadir="$DATADIR" -printtoconsole
```

### Stop the node

```bash
pkill -9 -f 'Coinod-v3'
pkill -9 -f wineserver
```

### Notes

- Keep the terminal open while `-printtoconsole` is active. Closing the
  terminal sends SIGHUP and the node exits.
- The RPC endpoint is bound to `127.0.0.1:29299` by default, which is
  correct — do not expose it to the internet.
- If you see an unexpected `.exe` process opening listening sockets on
  ports other than the P2P port (`29293`) and the RPC port (`29299`),
  stop the node immediately and check the binary.

### Log file location

If you are **not** running with `-printtoconsole`, the log is written to:

- Windows: `<datadir>\debug.log`
- macOS via Wine: `<real-path-to-datadir>/debug.log`

Tail it live:

```bash
tail -f "$DATADIR/debug.log"
```

---

## 7. How to safely back up the wallet

Before any operation that touches the datadir — including
`invalidateblock`, `peers.dat` cleanup, or a full resync — copy the
wallet to a location outside the datadir:

```bash
cp "$DATADIR/wallet.dat" "$HOME/wallet-backup-$(date +%Y%m%d-%H%M).dat"
```

If the wallet holds real value:

1. Make the backup on a machine you trust.
2. Consider restoring it on a **clean, offline** system and moving
   funds to a fresh wallet — do not leave funds on a wallet that has
   been opened on a machine with an unverified binary.

---

## 8. Full resync from scratch

Use this only if `peers.dat` cleanup and `invalidateblock` did not help.

```bash
# 1. Back up the wallet
cp "$DATADIR/wallet.dat" "$HOME/wallet-backup-$(date +%Y%m%d-%H%M).dat"

# 2. Stop the node
pkill -9 -f 'Coinod-v3'
pkill -9 -f wineserver

# 3. Move the blockchain data aside
cd "$DATADIR"
mkdir -p backup-$(date +%Y%m%d)
mv blk0001.dat blkindex.dat peers.dat database backup-$(date +%Y%m%d)/
```

Restart the node. It will begin syncing from block 0. Expect hours to
days, depending on network conditions and CPU.

---

## 9. General "do not do this" list

- ❌ **Do not ban peers** that report a higher block height than you.
  They are on the canonical chain; you are not. Banning them only
  disconnects you from the network.
- ❌ **Do not delete `blk0001.dat` / `blkindex.dat` first.** That is a
  full resync (hours/days). Try the lighter fixes first.
- ❌ **Do not ignore `peers.dat`.** It is the primary source of the
  "stuck at height" problem.
- ❌ **Do not assume the config in `%APPDATA%` is the one in use.**
  Always check the `-datadir=` argument of the running process.
- ❌ **Do not run untrusted binaries on a machine with real wallets.**
  If you did, treat the wallet as compromised and move funds.
- ❌ **Do not run the node as root / with `sudo`.** The node does not
  need elevated privileges, and running as root exposes the whole
  system if the binary is compromised.

---

## Confirmed test environment

- Client: `CNO-v2.0.0.3-g-bdb-gcc`
- OS: macOS (Apple Silicon) + Wine via Highball (engine `x64-sikarugir10.0_6-r26`)
- Outcome: node resumed from height **395,240** and synced past **403,000**
  with `ProcessBlock: ACCEPTED` on every block and no PoS errors.

---

*Last updated: 2026-10-13*
