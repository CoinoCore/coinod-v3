# 🍎 macOS Build Journal — CoinoCore V3.0.5 (arm64)

**Дата:** 2026-10-14
**Автор:** Serhii (coino.cno@gmail.com)
**Контекст:** Первый нативный macOS GUI-релиз CoinoCore V3
**Артефакт:** CoinoCore-mac-v3.0.5-arm64.dmg (42 МБ)
**SHA256:** e0bdfd294cc15fd71328986fd16bb112be6fab23e9b4b31bf17f6593df029698

---

## 🎯 Итог

- **Платформа:** macOS 14+ (Apple Silicon only — M1/M2/M3/M4)
- **UI:** Qt 6.11.2 (нативный, arm64)
- **Сборка:** Homebrew + qmake + macdeployqt
- **Размер .app:** 92 МБ (все зависимости внутри)
- **Размер .dmg:** 42 МБ (сжатый)
- **Готово к запуску на чистом Mac** — без Homebrew

---

## 📦 Что упаковано внутрь .app

### Qt 6 (framework)
- QtCore, QtGui, QtWidgets, QtNetwork, QtDBus, QtOpenGL
- QtQml, QtQuick, QtQmlMeta, QtQmlModels, QtQmlWorkerScript (лишние — тянет macdeployqt)

### Системные библиотеки
- OpenSSL 3.6.4 (libssl.3.dylib, libcrypto.3.dylib)
- Berkeley DB 5.3.28 (libdb_cxx-5.3.dylib)
- Boost (libboost_filesystem, libboost_program_options, libboost_thread, libboost_atomic, libboost_chrono, libboost_container, libboost_date_time, libboost_regex)
- qrencode 4.1.1 (libqrencode.4.dylib)

### Транзитивные зависимости (автоматически через macdeployqt)
- ICU 78 (libicudata, libicui18n, libicuuc)
- glib (libglib-2.0, libgobject-2.0, libgio-2.0, libgmodule-2.0, libgthread-2.0)
- harfbuzz, freetype, png, pcre2, bz2, brotli, zstd, jpeg-turbo, lcms2, mng, tiff, webp, graphite2, md4c, double-conversion, b2

---

## 🔴 КРИТИЧЕСКИЕ ГРАБЛИ

### 1. install_name_tool инвалидирует подпись macOS

**Симптом:**
```
Termination Reason: Namespace CODESIGNING
Exception: EXC_BAD_ACCESS (SIGKILL (Code Signature Invalid))
```

**Причина:** каждое изменение через `install_name_tool` (даже успешное) ломает встроенную ad-hoc подпись. macOS это видит и убивает процесс.

**Решение:** после ВСЕХ `install_name_tool` — переподписать снизу вверх:

1. Все `.dylib` (в `Contents/Frameworks/`)
2. Все framework-бинарники (`QtCore.framework/Versions/A/QtCore`)
3. Все плагины (`Contents/PlugIns/**/*.dylib`)
4. Главный бинарник (`Contents/MacOS/Coino-Qt`)
5. Сам `.app` (`codesign --force --sign - CoinoCore.app`)

```bash
find CoinoCore.app/Contents/Frameworks -maxdepth 1 -name "*.dylib" -exec codesign --force --sign - {} \;
for fw in CoinoCore.app/Contents/Frameworks/*.framework; do
  fwname=$(basename "$fw" .framework)
  codesign --force --sign - "$fw/Versions/A/$fwname"
done
find CoinoCore.app/Contents/PlugIns -name "*.dylib" -exec codesign --force --sign - {} \;
codesign --force --sign - CoinoCore.app/Contents/MacOS/Coino-Qt
codesign --force --sign - CoinoCore.app
```

---

### 2. macdeployqt не упаковывает НЕ-Qt-зависимости

**Что делает:** копирует только Qt framework + плагины + их прямые зависимости.

**Что НЕ делает:**
- boost (`libboost_*.dylib`) — кроме тех, что тянут Qt (у нас — вручную копировали)
- OpenSSL, BDB, qrencode (если бинарник линкуется к ним напрямую)
- Транзитивные boost-зависимости (`libboost_atomic`, `libboost_chrono`)

**Симптом:** `dyld: Library not loaded: @loader_path/libboost_atomic.dylib`

**Решение:** вручную скопировать недостающие и переписать ссылки:

```bash
for lib in libboost_atomic libboost_chrono libboost_container libboost_date_time libboost_regex; do
  cp /opt/homebrew/opt/boost/lib/${lib}.dylib \
     CoinoCore.app/Contents/Frameworks/
  install_name_tool -id "@loader_path/${lib}.dylib" \
     CoinoCore.app/Contents/Frameworks/${lib}.dylib
done
```

---

### 3. otool -L показывает собственный ID библиотеки (ложные срабатывания)

**Симптом:** при проверке «внешних ссылок» видим:

```
QtDBus.framework/Versions/A/QtDBus:
    @loader_path/../Frameworks/QtDBus.framework/Versions/A/QtDBus    ← ID (self)
    /opt/homebrew/opt/qtbase/lib/QtDBus.framework/Versions/A/QtDBus  ← тоже self!
```

Вторая строка — это **тот же QtDBus, ссылающийся на себя** (результат `install_name_tool -id`). **Не мешает загрузке** — dyld использует первую строку (`@loader_path`), а не ID.

**Как проверить реально:** игнорировать **первую** строку (ID), смотреть только остальные:

```bash
otool -L libbrotlicommon.1.dylib | tail -n +2 | grep "/opt/homebrew"
```

**Правило:** `otool -L` первая строка = self-ID, не зависимость.

---

### 4. SIGKILL (Code Signature Invalid) после install_name_tool

**Симптом:**
```bash
./CoinoCore.app/Contents/MacOS/Coino-Qt
zsh: killed
```

**Краш-лог:**
```
Termination Reason: Namespace CODESIGNING
Exception: EXC_BAD_ACCESS (SIGKILL (Code Signature Invalid))
```

**Причина:** подпись стала невалидной после правок через `install_name_tool`.

**Решение:** переподпись (см. граблю №1).

---

### 5. macdeployqt усекает бинарник

**Симптом:**
- Оригинал: 7 675 592 байт
- После `macdeployqt`: 6 068 600 байт (урезан на 1.6 МБ)
- `dyld` падает на `mach_o::Universal::isUniversal` — читает за пределами файла

**Причина:** `macdeployqt` делает strip + переписывает load commands. Иногда усекает файл при нехватке места.

**Решение:**
1. Восстановить оригинал:
   ```
   cp ~/Desktop/Coino-Qt-FINAL-20260917.app/Contents/MacOS/Coino-Qt \
      CoinoCore.app/Contents/MacOS/Coino-Qt
   ```
2. Переписать ссылки через `install_name_tool`
3. Переподписать

**Правило:** перед `macdeployqt` — сделать бэкап бинарника.

---

### 6. xattr блокирует codesign

**Симптом:**
```
codesign: resource fork, Finder information, or similar detritus not allowed
```

**Причина:** `com.apple.FinderInfo`, `com.apple.macl`, `com.apple.fileprovider.*` — атрибуты macOS, мешающие подписи.

**Решение:**
```bash
sudo xattr -cr CoinoCore.app
sudo xattr -rd com.apple.macl CoinoCore.app
sudo xattr -rd com.apple.FinderInfo CoinoCore.app 2>/dev/null
```

---

### 7. zsh не принимает #-комментарии в heredoc/скриптах

**Симптом:**
```
zsh: event not found: )
zsh: parse error near `OK'
zsh: command not found: #
```

**Причина:** zsh интерпретирует `#` и `!` в интерактивном режиме как history expansion / globbing.

**Решение:**
- Не пиши комментарии `#` в интерактивных командах
- Сложные скрипты — через `bash`:
```bash
bash << 'SCRIPT'
...
SCRIPT
```

---

### 8. PATH может стать пустым после экспериментов

**Симптом:**
```
zsh: command not found: ls
zsh: command not found: sed
```

**Причина:** `.zshrc` / `.zprofile` содержит `export PATH="/something"` **без `:$PATH`** — перезаписывает всё.

**Решение:**
```bash
export PATH="/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin:/opt/homebrew/bin:/opt/homebrew/sbin"
```

**И проверить `.zshrc`:**
```bash
cat ~/.zshrc | tail -20
cat ~/.zprofile | tail -20
```

---

### 9. qt.conf и плагины

**macdeployqt** создаёт автоматически:
```
CoinoCore.app/Contents/Resources/qt.conf
```
Содержит путь к плагинам. Не трогать — работает.

**Плагины внутри `.app`:**
```
Contents/PlugIns/
├── platforms/libqcocoa.dylib       ← обязательно (GUI)
├── styles/libqmacstyle.dylib       ← macOS-стиль
├── imageformats/libqjpeg.dylib
├── imageformats/libqgif.dylib
├── tls/libqopensslbackend.dylib
└── networkinformation/libqglennetworkinformation.dylib
```

---

### 10. Проверка целостности перед выпуском

```bash
# 1. Никаких homebrew ссылок в главном бинарнике
otool -L CoinoCore.app/Contents/MacOS/Coino-Qt | grep -cE "/opt/homebrew"
# Ожидаем: 0

# 2. Все @executable_path ссылки ведут в существующие файлы
otool -L CoinoCore.app/Contents/MacOS/Coino-Qt | grep "@executable" | \
    while read line; do
      path=$(echo "$line" | awk '{print $1}')
      name=$(basename "$path")
      [ -f "CoinoCore.app/Contents/Frameworks/$name" ] && echo "OK: $name" || echo "MISSING: $name"
    done

# 3. Подпись валидна
codesign --verify --verbose=4 CoinoCore.app

# 4. Gatekeeper принимает
spctl -a -vvv -t exec CoinoCore.app
# Ожидаем: accepted, source=no usable signature

# 5. Карантина нет
xattr -l CoinoCore.app
# Ожидаем: пусто
```

---

## 📋 РЕЦЕПТ: ПОЛНАЯ СБОРКА С НУЛЯ

```bash
# 1. Собрать .app (из исходников COINOM5/coino-master)
cd ~/COINOM5/coino-master
/opt/homebrew/opt/qt@6/bin/qmake coino-qt.pro
make -j8

# 2. Скопировать в рабочий каталог
mkdir -p ~/Desktop/CoinoCore-v3.0.5-mac
cp -R Coino-Qt.app ~/Desktop/CoinoCore-v3.0.5-mac/CoinoCore.app

# 3. Запустить macdeployqt (Qt6 из qtbase)
/opt/homebrew/opt/qtbase/bin/macdeployqt CoinoCore.app -verbose=2

# 4. Скопировать недостающие boost-либы
cd CoinoCore.app/Contents/Frameworks
for lib in libboost_atomic libboost_chrono libboost_container libboost_date_time; do
  cp /opt/homebrew/opt/boost/lib/${lib}.dylib .
done

# 5. Переписать ВСЕ ссылки на @loader_path / @executable_path
# (см. скрипт в разделе граблей)

# 6. Переподписать снизу вверх
# (см. раздел граблей №1)

# 7. Снять xattr
sudo xattr -cr CoinoCore.app

# 8. Проверить
codesign --verify --verbose=4 CoinoCore.app
spctl -a -vvv -t exec CoinoCore.app

# 9. Создать .dmg
hdiutil create -volname "CoinoCore V3.0.5" \
    -srcfolder CoinoCore.app \
    -ov -format UDZO \
    CoinoCore-mac-v3.0.5-arm64.dmg

# 10. SHA256
shasum -a 256 CoinoCore-mac-v3.0.5-arm64.dmg
```

---

## 📚 Версии компонентов

| Компонент | Версия |
|---|---|
| macOS | 27.0.1 (build 26A434) |
| Xcode CLT | (поставляется с macOS) |
| Homebrew | /opt/homebrew |
| Qt | 6.11.2 |
| OpenSSL | 3.6.4 |
| BDB | 5.3.28 |
| Boost | актуальная из brew |
| qrencode | 4.1.1 |
| ICU | 78 |
| glib | 2.88.3 |
| harfbuzz | актуальная |

---

## 🔗 Полезные ссылки

- Релиз v3.0.5: https://github.com/CoinoCore/coinod-v3/releases/tag/v3.0.5
- Issues: https://github.com/CoinoCore/coinod-v3/issues
- Qt macdeployqt docs: https://doc.qt.io/qt-6/macos-deployment.html
- Apple codesign docs: https://developer.apple.com/documentation/security/updating_mac_software

---

## ⚠️ Известные ограничения v3.0.5

- **Только arm64** (Intel Mac не поддерживается — нужен отдельный x86_64 build)
- **Не нотаризован** (нужен Apple Developer $99/год для notarization)
- **Ad-hoc подпись** — при первом запуске может сработать Gatekeeper: правый клик → Open
- **ReacceptWalletTransactions** отключён (унаследовано от Windows v3.0.4)
- **QtQml/QtQuick** упакованы зря (macdeployqt их тянет автоматически) — можно убрать для экономии ~15 МБ

---

**Последнее обновление:** 2026-10-14 16:00 UTC
**Версия документа:** 1.0
