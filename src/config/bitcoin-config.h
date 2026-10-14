/* Minimal bitcoin-config.h for MXE cross-compile (CoinoCore V3)
 * В оригинале генерируется ./configure — у нас MXE, поэтому дефайны
 * заданы в makefile.mingw.mxe и coino-qt.pro.mxe.
 * Здесь только то, что требует notificator.h.
 */
#ifndef BITCOIN_CONFIG_H
#define BITCOIN_CONFIG_H

#define PACKAGE_NAME    "CoinoCore"
#define PACKAGE_VERSION "3.0.4"
#define PACKAGE_STRING  "CoinoCore 3.0.4"
#define PACKAGE_BUGREPORT "serhii@coinocore.io"

#endif /* BITCOIN_CONFIG_H */
