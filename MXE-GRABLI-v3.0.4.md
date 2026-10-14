# 🧱 MXE / Qt5 — Грабли и открытия (v3.0.4)

**Дата:** 2026-10-13
**Контекст:** Сборка Coino-Qt для Windows (cross-compile через MXE)
**Версии:** Ubuntu 24.04.5, MXE 8635de3, Qt 5.15.19, Boost 1.92, OpenSSL 4.0.3, BDB 18.1.40, gcc-target 11.5.0.

---

## 13. MXE .mk-пакеты живут в src/, не в pkg/

pkg/ — только tarball-ы. Makefile ищет правила через MXE_PLUGIN_DIRS:

    PKG_MAKEFILES = $(realpath $(sort $(wildcard $(addsuffix /$(1).mk, $(MXE_PLUGIN_DIRS)))))

Проверка: `find /mnt/data/mxe/src -name 'qt*.mk'`.

Не пугайся, если `ls pkg/*.mk` = 0 — это норма для этой сборки MXE.

---

## 14. Boost 1.92 использует суффикс -mt-x64

Старый -mt-s исчез. Реальные файлы:

    libboost_thread-mt-x64.a
    libboost_filesystem-mt-x64.a
    libboost_program_options-mt-x64.a
    libboost_chrono-mt-x64.a

В .pro: -lboost_thread-mt-x64 (не -mt-s).
Также добавить -DBOOST_ALL_NO_LIB в DEFINES.

---

## 15. Qt5 в MXE разбит на пакеты

qt5.mk НЕ существует. Правильный набор:

- qtbase   — ядро + qmake-qt5
- qttools  — lrelease / lupdate / linguist
- qtsvg    — SVG-иконки
- qrencode — опционально (QR)

Команда:
    make MXE_TARGETS='x86_64-w64-mingw32.static' qtbase qttools qtsvg -j2

---

## 16. host-glib падает на host-pcre2 (meson-fallback)

Симптом:
    Run-time dependency libpcre2-8 found: NO
    Looking for a fallback subproject for the dependency libpcre2-8
    Downloading pcre2 source from ... pcre2-10.46.tar.bz2
    <urlopen error ... name resolution>
    Don't use network from MXE build rules!

Причины:
- MXE изолирует env → MESON_PACKAGE_CACHE_DIR игнорируется
- MXE удаляет tmp-glib перед сборкой → подкладывать tarball бесполезно
- MXE блокирует сеть

Решение: поставить СИСТЕМНЫЙ pcre2-dev:

    sudo apt-get install -y libpcre2-dev
    pkg-config --modversion libpcre2-8    # → 10.42
    make MXE_TARGETS='x86_64-pc-linux-gnu' glib -j2   # ✅ [done] ~1m

---

## 17. main() определён в init.cpp → конфликт с Qt

В init.cpp есть `int main(...)`. В bitcoin.cpp тоже. При линковке: multiple definition of main.

Фикс через objcopy на копии:

    cd ~/coino-work/repo/src
    cp obj/init.o obj/init_qt.o
    objcopy --redefine-sym main=coinod_daemon_main obj/init.o obj/init_qt.o
    rm -f libcoinod_common.a
    ar rcs libcoinod_common.a $(ls obj/*.o | grep -vE '(noui|init)\.o$') obj/init_qt.o
    nm libcoinod_common.a | grep -wE "T main" || echo "OK: main() отсутствует"

---

## 18. libcoinod_common.a — линковать одним архивом

В .pro: -L$$SRC_DIR -lcoinod_common ПЕРВЫМ в LIBS (перед boost/ssl).

GNU ld одноразовый, слева направо. Порядок:

    LIBS += \
        -L$$MXE_PREFIX/lib \
        -L$$SRC_DIR/secp256k1/.libs \
        -L$$SRC_DIR \
        -lcoinod_common \
        -lsecp256k1 \
        -lboost_filesystem-mt-x64 \
        -lboost_program_options-mt-x64 \
        -lboost_thread-mt-x64 \
        -lboost_chrono-mt-x64 \
        -ldb_cxx \
        -lssl -lcrypto -lz -lcrypt32 \
        -lshlwapi -lws2_32 -lmswsock -liphlpapi \
        -lkernel32 -luser32 -lgdi32 -lcomdlg32 -lwinspool -lwinmm \
        -lshell32 -lcomctl32 -lole32 -loleaut32 -luuid -lrpcrt4 -ladvapi32

---

## 19. LevelDB в Bitcoin 0.8 — не отдельная либа

USE_LEVELDB := 0 → используется BDB (obj/txdb-bdb.o).
libleveldb.a НЕТ. Не добавлять -lleveldb, -L../leveldb в .pro.

---

## 20. Bash pipe | head -N убивает make через SIGPIPE

    make ... | head -20   # ❌ после 20 строк make падает с SIGPIPE

Правильно: make ... | tee log + tail -f log в другом окне.

---

## 21. | tail -N буферизует вывод — тишина до конца

    make ... | tail -40   # ничего не видно, пока make не завершится

Правильно:
    make ... 2>&1 | tee /tmp/build.log   # в screen
    tail -f /tmp/build.log                # в другом окне

---

## 22. Вложенные screen — источник путаницы

screen -S qt ВНУТРИ screen mxe создаёт вложенный.
screen -r <id> внутри того же screen → "no screen to be resumed".

Правило: один screen на задачу. Перед screen -S → Ctrl+A, D.

---

## 23. Проверка, где ты сейчас

    echo "STY=[$STY]"     # пусто → shell; иначе NNNNNN.name → screen
    tty                    # /dev/pts/N
    screen -ls

---

## 24. Static Qt5 требует явных плагинов

В .pro:
    QTPLUGIN += qwindows qjpeg qgif qico qsvg

Иначе GUI запустится без иконок, PNG, стиля.

---

## 25. Тёмная тема: Fusion + QPalette + QSS

- Fusion — единственный стиль, полностью подчиняющийся QPalette
- QPalette — базовые цвета
- QSS — тонкая настройка

Вызов — до создания QMainWindow, после QApplication:

    QApplication app(argc, argv);
    applyCoinoDarkTheme(app);   // Fusion + palette + :/images/coino.qss

---

## 26. git status --short в MXE

Если MXE — shallow-клон (grafted), pkg/*.mk может отсутствовать.
Проверка: git ls-tree HEAD pkg/ | wc -l.
Не пытаться "починить" — взять существующую src/-структуру.

---

## 27. Windows .exe с версией/иконкой — через RC-файл

В .pro:
    RC_FILE = $$QT_DIR/res/bitcoin-qt.rc

Без RC в "Свойствах" .exe нет ни версии, ни описания, ни иконки.

---

## 28. Порядок LIBS критичен

    -lcoinod_common     ← ядро первым
    -lsecp256k1
    -lboost_*
    -ldb_cxx
    -lssl -lcrypto      ← ssl до crypto
    -lz -lcrypt32       ← z после crypto, crypt32 — после z

Линкер GNU ld одноразовый, слева направо.

---

## 29. sed с \n в replacement — только GNU sed

BSD-sed (macOS) не поддерживает \n в replacement.
Сложные многострочные патчи — через python3.

---

## 30. Python regex + жадность = боль

re.sub(r'LIBS \+= ...(?:.*\\\n)*', ...) → backtracking, зависание.

Правильно — через s.index():

    start = s.index('LIBS +=')
    end = s.index('# --- Static Qt plugins')
    s = s[:start] + new_libs + s[end:]

---

## ОТЛОЖЕНО (v3.0.4)

- qrencode — не критично (QR встроен в qrcodedialog.cpp)
- Тест на реальной Windows — VM пока нет
- Code Signing — SmartScreen будет ругаться
- GitHub Actions CI — весь релиз руками
- .deb / .rpm — только Windows сейчас

---

## ОТКРЫТЫЕ ВОПРОСЫ

1. Почему make pcre2 не виден как таргет (MXE_PLUGIN_DIRS переопределён?)
2. Почему screen -S qt внутри screen mxe создаёт вложенный?
3. repo-snapshot-2026-10-09-1351/ — старая копия, удалять?
4. pcre2.mk версия 10.49, glib просит 10.46 в .wrap — почему?

---

## ПРОВЕРЕННЫЕ ВЕРСИИ

| Компонент | Версия |
|---|---|
| Ubuntu | 24.04.5 LTS |
| MXE коммит | 8635de3 (grafted) |
| Qt | 5.15.19 |
| Boost | 1.92 (-mt-x64) |
| OpenSSL | 4.0.3 |
| BDB | 18.1.40 |
| libsecp256k1 | static |
| gcc (target) | 11.5.0 |
| gcc (host) | 13 |
| pcre2 (host) | 10.42 (system) |
| pcre2 (target) | 10.49 (MXE) |

---

## КОМАНДЫ ДИАГНОСТИКИ

    # Где я
    echo "STY=[$STY]"; tty; screen -ls

    # Жив ли make
    ps -ef | grep -E "qt-build|make.*mxe" | grep -v grep

    # Прогресс Qt5
    grep -cE "^\[done\]" /mnt/data/mxe/qt-build.log
    grep -E "^\[done\].*(qtbase|qttools|qtsvg)" /mnt/data/mxe/qt-build.log

    # Ошибки
    grep -iE "error|failed|killed" /mnt/data/mxe/qt-build.log | tail -5

---

## РЕЦЕПТ: ПЕРЕСБОРКА QT5 С НУЛЯ

    # 1. Системные зависимости
    sudo apt-get install -y libpcre2-dev pkg-config

    # 2. Host-glib
    cd /mnt/data/mxe
    make MXE_TARGETS='x86_64-pc-linux-gnu' glib -j2

    # 3. Qt5 в screen
    screen -S qt2
    make MXE_TARGETS='x86_64-w64-mingw32.static' qtbase qttools qtsvg -j2 \
         2>&1 | tee -a /mnt/data/mxe/qt-build.log
    # Ctrl+A, D

    # 4. Утром — проверить
    grep -E "^\[done\].*(qtbase|qttools|qtsvg)" /mnt/data/mxe/qt-build.log

---

Последнее обновление: 2026-10-13 15:07 UTC
