# Build Journal: CoinoCore V3 v3.0.3 — Windows x86-64

**Дата:** 2026-10-12
**Автор:** Serhii (serhii@coinocore.io)
**Сборочная машина:** Azure VM `CCIF`, Ubuntu 24.04 LTS, IP `74.235.109.156`
**Целевая платформа:** Windows 10+ x86-64
**Результат:** `Coinod-v3.0.3-win64.exe` (13.2 МБ, PE32+, полностью статический)

---

## ЦЕЛЬ

Первый Windows-билд CoinoCore V3. До этого собирался только под Linux.

---

## СБОРОЧНАЯ МАШИНА

| Параметр | Значение |
|---|---|
| ОС | Ubuntu 24.04 LTS (Azure) |
| RAM | 8 ГБ |
| Основной диск `/` | 30 ГБ (мало!) |
| **Второй диск `/mnt/data`** | **110 ГБ (критично)** |
| CPU | 2 ядра |

**Урок 1:** MXE + Boost + OpenSSL + сборка = ~20 ГБ. В `/` (30 ГБ) не влезет. Нужен `/mnt/data`.

```bash
# Монтирование второго диска
sudo parted /dev/nvme1n1 --script mklabel gpt
sudo parted /dev/nvme1n1 --script mkpart primary ext4 0% 100%
sudo mkfs.ext4 -L data /dev/nvme1n1p1
sudo mkdir -p /mnt/data
sudo mount /dev/nvme1n1p1 /mnt/data
sudo chown -R $USER:$USER /mnt/data
UUID=$(sudo blkid -s UUID -o value /dev/nvme1n1p1)
echo "UUID=$UUID /mnt/data ext4 defaults,nofail 0 2" | sudo tee -a /etc/fstab
sudo mount -a
```

---

## УСТАНОВКА MXE

### Шаг 1. Системные зависимости

```bash
sudo apt update
sudo apt install -y \
  build-essential cmake curl git wget xz-utils \
  autoconf automake autopoint bash bison bzip2 flex \
  gettext g++ gperf intltool libffi-dev libtool libtool-bin \
  libltdl-dev libssl-dev libxml-parser-perl lzip make \
  openssl p7zip-full patch perl pkg-config python3 ruby \
  scons sed unzip screen python3-mako python-is-python3
```

**Урок 2:** `python-is-python3` — обязателен (MXE вызывает `python`).
**Урок 3:** `libtool-bin` (не `libtool`) — содержит `libtoolize`.
**Урок 4:** `python3-mako` — MXE проверяет `mako-render`.
**Урок 5:** `screen` — сборка идёт 30–40 минут, без screen при обрыве SSH всё теряется.

### Шаг 2. Клонировать MXE в `/mnt/data`

```bash
cd /mnt/data
git clone --depth 1 https://github.com/mxe/mxe.git
cd /mnt/data/mxe

export MXE_TARGETS=x86_64-w64-mingw32.static
export PATH=/mnt/data/mxe/usr/bin:$PATH
```

### Шаг 3. Собрать Boost + OpenSSL + BDB

```bash
screen -S mxe
cd /mnt/data/mxe
make MXE_TARGETS='x86_64-w64-mingw32.static' \
  boost openssl db -j2 2>&1 | tee /mnt/data/mxe/build.log
# Ctrl+A, D — выйти
```

**Время:** ~30–40 минут.
**Урок 6:** BDB пакет называется **`db`**, не `berkeleydb`.
**Урок 7:** Boost 1.92 — `libboost_system` стала header-only, не линковать.
**Урок 8:** суффикс библиотек — `-mt-x64` (не `-mgw49-mt-s-1_57`).

### Шаг 4. Проверка

```bash
ls /mnt/data/mxe/usr/x86_64-w64-mingw32.static/lib/ | grep -iE "boost|ssl|crypto|db"
```

Ожидаемые файлы:
- `libboost_filesystem-mt-x64.a`
- `libboost_program_options-mt-x64.a`
- `libboost_thread-mt-x64.a`
- `libboost_chrono-mt-x64.a`
- `libssl.a`, `libcrypto.a`
- `libdb_cxx.a`, `libdb-18.1.a`
- `libz.a`

---

## ПЕРЕСБОРКА libsecp256k1 (СТАТИЧЕСКАЯ)

**Урок 9 (КРИТИЧНО):** при `--enable-shared` создаются `libsecp256k1.dll.a` + `libsecp256k1-2.dll`. Линкер mingw предпочитает `.dll.a` → `.exe` требует `.dll`. Решение — `--disable-shared --enable-static`.

```bash
cd ~/coino-work/repo/src/secp256k1

make distclean 2>/dev/null || true
rm -rf .libs obj
git restore include/secp256k1.h

./autogen.sh

./configure \
  --host=x86_64-w64-mingw32 \
  --disable-shared \
  --enable-static \
  --disable-tests \
  --disable-benchmark \
  --enable-module-recovery \
  --with-asm=x86_64 \
  CC=/mnt/data/mxe/usr/bin/x86_64-w64-mingw32.static-gcc \
  CXX=/mnt/data/mxe/usr/bin/x86_64-w64-mingw32.static-g++ \
  AR=/mnt/data/mxe/usr/bin/x86_64-w64-mingw32.static-ar \
  RANLIB=/mnt/data/mxe/usr/bin/x86_64-w64-mingw32.static-ranlib \
  STRIP=/mnt/data/mxe/usr/bin/x86_64-w64-mingw32.static-strip

make -j2
```

**Проверка:**
```bash
file .libs/libsecp256k1.a
# current ar archive (НЕ ELF)
ls .libs/ | grep -c dll
# 0 — нет DLL
```

---

## makefile.mingw.mxe

**Основа:** `makefile.linux-mingw`.

Ключевые отличия:
```makefile
CC      = /mnt/data/mxe/usr/bin/x86_64-w64-mingw32.static-gcc
CXX     = /mnt/data/mxe/usr/bin/x86_64-w64-mingw32.static-g++
AR      = /mnt/data/mxe/usr/bin/x86_64-w64-mingw32.static-ar
RANLIB  = /mnt/data/mxe/usr/bin/x86_64-w64-mingw32.static-ranlib
STRIP   = /mnt/data/mxe/usr/bin/x86_64-w64-mingw32.static-strip

MXE_DIR = /mnt/data/mxe/usr/x86_64-w64-mingw32.static
MXE_INC = $(MXE_DIR)/include
MXE_LIB = $(MXE_DIR)/lib

INCLUDEPATHS = -I"$(CURDIR)" -I"$(CURDIR)/obj" -I"$(MXE_INC)" \
               -I"$(CURDIR)/secp256k1/include"

LIBPATHS = -L"$(MXE_LIB)" -L"$(CURDIR)/secp256k1/.libs"

LIBS = \
 -lboost_filesystem-mt-x64 \
 -lboost_program_options-mt-x64 \
 -lboost_thread-mt-x64 \
 -lboost_chrono-mt-x64 \
 -ldb_cxx \
 -lssl \
 -lcrypto \
 -lsecp256k1 \
 -lz \
 -lcrypt32 \
 -lshlwapi -lws2_32 -lmswsock -liphlpapi \
 -lkernel32 -luser32 -lgdi32 -lcomdlg32 -lwinspool -lwinmm \
 -lshell32 -lcomctl32 -lole32 -loleaut32 -luuid -lrpcrt4 -ladvapi32

DEFS = -D_MT -DWIN32 -D_WINDOWS -DBOOST_THREAD_USE_LIB \
       -DBOOST_SPIRIT_THREADSAFE -D__STDC_FORMAT_MACROS \
       -D__STDC_LIMIT_MACROS -DSECP256K1_STATIC

CFLAGS  = -O2 -msse2 -mssse3 -w -Wall -Wextra \
          -Wno-ignored-qualifiers -Wformat -Wformat-security \
          -Wno-unused-parameter $(DEFS) $(INCLUDEPATHS)

LDFLAGS = -Wl,--dynamicbase -Wl,--nxcompat -static-libgcc -static-libstdc++

# ВАЖНО: -fpermissive для .c файлов
obj/%.o: %.c $(HEADERS)
	$(CXX) -c $(CFLAGS) -fpermissive -x c++ -o $@ $<
```

**Урок 10:** `-DSECP256K1_STATIC` обязателен — иначе заголовок включает `dllimport` и `.o` ждёт `__imp_secp256k1_*`.
**Урок 11:** `-lz` и `-lcrypt32` — ПОСЛЕ `-lcrypto` (порядок LIBS важен).
**Урок 12:** `-fpermissive` для `.c` файлов (`groestl.c` использует C-стиль `void*` → typed).

---

## ИСПРАВЛЕНИЯ В КОДЕ

### 1. db.cpp — DbEnv неоднозначен

```cpp
// Было:
DbEnv(0).remove(strPath.c_str(), 0);

// Стало:
DbEnv((u_int32_t)0).remove(strPath.c_str(), 0);
```

**Урок 13:** BDB 18.1 перегружает `DbEnv(u_int32_t)` и `DbEnv(DB_ENV*)`, `0` неоднозначен.

### 2. version.cpp — пустая дата сборки

```cpp
// Было:
#define GIT_ARCHIVE 1
#ifdef GIT_ARCHIVE
#    define GIT_COMMIT_ID ""
#    define GIT_COMMIT_DATE ""     // ← определён как пустая строка
#endif

#ifndef BUILD_DATE
#    ifdef GIT_COMMIT_DATE         // ← истина
#        define BUILD_DATE GIT_COMMIT_DATE  // ← пустая строка
#    else
#        define BUILD_DATE __DATE__ ", " __TIME__
#    endif
#endif

// Стало: закомментировать GIT_COMMIT_DATE
// #    define GIT_COMMIT_DATE ""
```

**Урок 14:** `#ifdef` проверяет **определён ли макрос**, не его значение.

### 3. clientversion.h — версия 2.0.0.3

```c
#define CLIENT_VERSION_MAJOR       2
#define CLIENT_VERSION_MINOR       0
#define CLIENT_VERSION_REVISION    0
#define CLIENT_VERSION_BUILD       3   // ← 2 → 3
```

### 4. version.h — display-версия

```c
#define DISPLAY_VERSION_MAJOR       2   // НЕ менять! (coinbase)
#define DISPLAY_VERSION_MINOR       0   // НЕ менять!
#define DISPLAY_VERSION_REVISION    0   // НЕ менять!
#define DISPLAY_VERSION_BUILD       3   // ← 2 → 3
```

**Урок 15:** `DISPLAY_VERSION_MAJOR/MINOR/REVISION` идут в coinbase (init.cpp:504) — изменение = форк.

---

## СБОРКА

```bash
cd ~/coino-work/repo/src
rm -f obj/*.o                       # очистка
make -f makefile.mingw.mxe -j2 2>&1 | tee /tmp/build-win.log
```

**Время:** 10–20 минут.

**Результат:** `Coinod.exe` (13.2 МБ, PE32+).

---

## ПРОВЕРКА БИНАРНИКА

```bash
cd ~/coino-work/repo/src

# Формат
file Coinod.exe
# PE32+ executable (console) x86-64 (stripped to external PDB), for MS Windows

# DLL-импорты (только системные!)
/mnt/data/mxe/usr/bin/x86_64-w64-mingw32.static-objdump -p Coinod.exe | grep "DLL Name"
# ADVAPI32.dll, CRYPT32.dll, KERNEL32.dll, msvcrt.dll,
# MSWSOCK.dll, SHELL32.dll, USER32.dll, WS2_32.dll

# Версия
strings Coinod.exe | grep "CNO-v"
# CNO-v2.0.0.3-g-bdb-gcc

# Дата
strings Coinod.exe | grep "Oct 12 2026"
# Oct 12 2026, 18:37:26

# SHA256
sha256sum Coinod.exe
```

---

## ТЕСТ ПОД WINE (macOS Apple Silicon)

**Урок 16:** Highball не подходит для консольных приложений (DXMT/DXVK — для игр). Запускать через CLI `wine`.

```bash
WINE="/Users/serhii2049/Library/Application Support/Highball/engines/x64-sikarugir10.0_6-r26/engine/bin/wine"

export WINEPREFIX="$HOME/.wine-coino-test"
export WINEDEBUG=-all

"$WINE" ~/COINOM2Windows/CoinoCore-v3.0.3/Coinod-v3.0.3-win64.exe "-?"
# Coino version CNO-v2.0.0.3-g-bdb-gcc (Oct 12 2026, 18:37:26)
```

**Урок 17:** в zsh `-?` — wildcard, брать в кавычки `"-?"`.

**Запуск демона:**
```bash
"$WINE" ~/COINOM2Windows/CoinoCore-v3.0.3/Coinod-v3.0.3-win64.exe \
  -datadir='Z:\Users\serhii2049\COINOM2Windows\CoinoCore-v3.0.3\data' \
  -printtoconsole
```

**Проверено:**
- ✅ Демон стартует
- ✅ RPC работает
- ✅ 11 пиров подключились
- ✅ Синхронизация идёт (blocks растут)
- ✅ Нет ошибки `libsecp256k1-2.dll not found`

---

## GIT + GITHUB RELEASE

```bash
cd ~/coino-work/repo

git add -A
git commit -m "v3.0.3: fix static linking of libsecp256k1"
git tag -a v3.0.3 -m "..."
git push origin main
git push origin v3.0.3 --force
```

**Урок 18:** `git push` не отправляет теги. Нужно **отдельно**: `git push origin v3.0.3`.
**Урок 19:** `git push --tags` пушит ВСЕ теги, включая локальные `wip-*`. Не делать.
**Урок 20:** для организации `CoinoCore` и личного `Serhio2049` нужен GitHub Personal Access Token (не пароль).

**GitHub Release:** браузер → https://github.com/CoinoCore/coinod-v3/releases/edit/v3.0.3

---

## ПОЛНАЯ ПЕРЕСБОРКА С НУЛЯ

```bash
# 1. Проверить MXE
ls /mnt/data/mxe/usr/bin/x86_64-w64-mingw32.static-g++ || echo "MXE НЕТ"

# 2. Проверить secp256k1
file ~/coino-work/repo/src/secp256k1/.libs/libsecp256k1.a
# current ar archive (не ELF!)

# 3. Очистить объектники
cd ~/coino-work/repo/src
rm -f obj/*.o

# 4. Собрать
make -f makefile.mingw.mxe -j2 2>&1 | tee /tmp/build-$(date +%Y%m%d-%H%M).log

# 5. Проверить
file Coinod.exe
strings Coinod.exe | grep -E "CNO-v|Oct"
/mnt/data/mxe/usr/bin/x86_64-w64-mingw32.static-objdump -p Coinod.exe | grep "DLL Name"
sha256sum Coinod.exe
```

---

## ВСЕ ГРАБЛИ В ОДНОМ СПИСКЕ

1. MXE не влезает в `/` — нужен `/mnt/data`.
2. `python-is-python3` — обязателен.
3. `libtool-bin` (не `libtool`).
4. `python3-mako` — MXE проверяет `mako-render`.
5. BDB пакет называется `db`.
6. Boost 1.92 — без `-lboost_system`.
7. `-lz -lcrypt32` — ПОСЛЕ `-lcrypto`.
8. `.c` файлы — `-fpermissive`.
9. `DbEnv((u_int32_t)0)` для BDB 18.1.
10. `version.cpp` — отключить `GIT_COMMIT_DATE`.
11. **secp256k1** — `--disable-shared --enable-static` + `-DSECP256K1_STATIC`.
12. Highball не для CLI — запускать через `wine` напрямую.
13. zsh `-?` — брать в кавычки.
14. `git push` не отправляет теги.
15. `DISPLAY_VERSION_MAJOR/MINOR/REVISION` — НЕ менять (coinbase).

---

## ССЫЛКИ

- Репозиторий: https://github.com/CoinoCore/coinod-v3
- Релиз v3.0.3: https://github.com/CoinoCore/coinod-v3/releases/tag/v3.0.3
- MXE: https://github.com/mxe/mxe
- libsecp256k1: https://github.com/bitcoin-core/secp256k1

---

**Дата последнего обновления:** 2026-10-12
**Версия:** v3.0.3
