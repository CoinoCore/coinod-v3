# CoinoCore V3 v3.0.3 — Windows Build

Первый официальный релиз CoinoCore V3 под Windows x86-64.

## Что нового

### Added

- Windows x86-64 build — Coinod-v3.0.3-win64.exe
- Cross-compiled via MXE (MinGW-w64 GCC 11.5.0)
- Fully static linking (no runtime DLL dependencies)
- Berkeley DB 18.1, OpenSSL 4.0.3, Boost 1.92, libsecp256k1

### Changed

- CLIENT_VERSION_BUILD: 2 -> 3 (клиентская версия 2.0.0.3)
- DISPLAY_VERSION_BUILD: 2 -> 3 (строка CNO-v2.0.0.3-...)
- BUILD_DATE теперь корректно подставляется

### Fixed (унаследовано из v3.0.2)

- Cryptographic RNG in key generation (src/key.cpp):
  - rand() -> RAND_bytes() (OpenSSL CSPRNG)
  - Added error handling via throw std::runtime_error
  - Correct recovery id: recid = (nV - 27) & 3
  - Separate fCompressedSig handling in SetCompactSignature
  - Check secp256k1_ec_pubkey_serialize return value
  - Reset state only after successful serialization

## Артефакты

| Файл | Размер | SHA256 |
|---|---|---|
| Coinod-v3.0.3-win64.exe | 12.0 МБ | f7960856f033d8933f9ee7a45620a975c9f03760c090470d6354acd82b303a5f |

Build date: 2026-10-12 15:06:45 UTC

Version string: CNO-v2.0.0.3-g-bdb-gcc

## Установка (Windows)

1. Скачать Coinod-v3.0.3-win64.exe из этого релиза.
2. Поместить в удобную папку, например C:\Coino\.
3. Создать C:\Coino\Coino.conf:

   rpcuser=ваш_логин
   rpcpassword=ваш_пароль
   rpcport=29299
   port=29293
   listen=1
   server=1
   txindex=1

   Примечание 1: daemon=1 на Windows не поддерживается (нет fork()).
   Используйте start /B для запуска в фоне.

   Примечание 2: txindex=1 требует полной переиндексации блокчейна
   при первом запуске (может занять несколько часов).

4. Запустить в отдельном окне CMD:

   Coinod-v3.0.3-win64.exe -datadir=C:\Coino

   Или в фоне:

   start /B Coinod-v3.0.3-win64.exe -datadir=C:\Coino > C:\Coino\debug.log 2>&1

5. Проверить:

   Coinod-v3.0.3-win64.exe -datadir=C:\Coino getblockcount

   Проверить версию:

   Coinod-v3.0.3-win64.exe -?

   Ожидаемо:

   Coino version CNO-v2.0.0.3-g-bdb-gcc (Oct 12 2026, 15:06:45)

## Проверка SHA256 (Windows PowerShell)

   Get-FileHash .\Coinod-v3.0.3-win64.exe -Algorithm SHA256

   Ожидаемо:

   f7960856f033d8933f9ee7a45620a975c9f03760c090470d6354acd82b303a5f

## Windows SmartScreen

При первом запуске SmartScreen может показать предупреждение.
Это нормально для неподписанных бинарников. Нажмите
"Подробнее" -> "Выполнить в любом случае".

Альтернатива: правый клик -> Свойства -> галочка "Разблокировать".

## Заменяет

- v3.0.2 (только Linux)
- v3.0.1 (только Linux, баг с BN_num_bits)
- v3.0.0 (только Linux)

## Известные ограничения

- Только CLI/демон, GUI (Qt) для Windows пока не собран.
- Требуется Windows 10 или новее (x86-64).
- Anti-virus может ругаться на неподписанный .exe.
