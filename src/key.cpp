// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <map>

#include "key.h"

using namespace std;

// ──────────────────────────────────────────────────────────────────────
// libsecp256k1 context — единый на весь процесс
// ──────────────────────────────────────────────────────────────────────
static secp256k1_context* secp256k1_context_sign = NULL;

void ECC_Start()
{
    assert(secp256k1_context_sign == NULL);

    secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_NONE);

    // Рандомизация контекста (защита от side-channel)
    unsigned char seed[32];
    for (int i = 0; i < 32; i++)
        seed[i] = (unsigned char)(rand() & 0xFF);

    if (secp256k1_context_randomize(ctx, seed) == 0) {
        secp256k1_context_destroy(ctx);
        throw std::runtime_error("secp256k1_context_randomize failed");
    }

    secp256k1_context_sign = ctx;
}

void ECC_Stop()
{
    if (secp256k1_context_sign) {
        secp256k1_context_destroy(secp256k1_context_sign);
        secp256k1_context_sign = NULL;
    }
}

// ──────────────────────────────────────────────────────────────────────
// Сравнение big-endian чисел
// ──────────────────────────────────────────────────────────────────────
static int CompareBigEndian(const unsigned char *c1, size_t c1len,
                            const unsigned char *c2, size_t c2len)
{
    while (c1len > c2len) {
        if (*c1) return 1;
        c1++; c1len--;
    }
    while (c2len > c1len) {
        if (*c2) return -1;
        c2++; c2len--;
    }
    while (c1len > 0) {
        if (*c1 > *c2) return 1;
        if (*c2 > *c1) return -1;
        c1++; c2++; c1len--;
    }
    return 0;
}

// Order of secp256k1's generator minus 1.
static const unsigned char vchMaxModOrder[32] = {
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFE,
    0xBA,0xAE,0xDC,0xE6,0xAF,0x48,0xA0,0x3B,
    0xBF,0xD2,0x5E,0x8C,0xD0,0x36,0x41,0x40
};

static const unsigned char vchMaxModHalfOrder[32] = {
    0x7F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x5D,0x57,0x6E,0x73,0x57,0xA4,0x50,0x1D,
    0xDF,0xE9,0x2F,0x46,0x68,0x1B,0x20,0xA0
};

static const unsigned char *vchZero = NULL;

// ──────────────────────────────────────────────────────────────────────
// Helper: DER-экспорт приватного ключа (совместимо с OpenSSL)
// Заменяет i2d_ECPrivateKey.
// ──────────────────────────────────────────────────────────────────────
static int ec_privkey_export_der(const secp256k1_context *ctx,
                                 unsigned char *privkey, size_t *privkeylen,
                                 const unsigned char *key, int compressed)
{
    secp256k1_pubkey pubkey;
    size_t pubkeylen = 0;
    if (!secp256k1_ec_pubkey_create(ctx, &pubkey, key))
        return 0;

    size_t pos = 0;

    // SEQUENCE
    privkey[pos++] = 0x30;

    // Reserve 1 byte for length, fill later
    size_t seq_start = pos++;
    size_t seq_len = 0;

    // INTEGER 1 (version)
    privkey[pos++] = 0x02;
    privkey[pos++] = 0x01;
    privkey[pos++] = 0x01;
    seq_len += 3;

    // OCTET STRING (secret)
    privkey[pos++] = 0x04;
    privkey[pos++] = 0x20;
    memcpy(&privkey[pos], key, 32);
    pos += 32;
    seq_len += 34;

    // [0] { OID ecPublicKey, OID secp256k1 }
    privkey[pos++] = 0xA0;
    privkey[pos++] = 0x0A;
    privkey[pos++] = 0x06;
    privkey[pos++] = 0x07;
    privkey[pos++] = 0x2A; privkey[pos++] = 0x86;
    privkey[pos++] = 0x48; privkey[pos++] = 0xCE;
    privkey[pos++] = 0x3D; privkey[pos++] = 0x02;
    privkey[pos++] = 0x01;
    privkey[pos++] = 0x06;
    privkey[pos++] = 0x05;
    privkey[pos++] = 0x2B; privkey[pos++] = 0x81;
    privkey[pos++] = 0x04; privkey[pos++] = 0x00;
    privkey[pos++] = 0x0A;
    seq_len += 16;

    // [1] { BIT STRING pubkey }
    unsigned char pubkey_buf[65];
    size_t pubkey_len = 65;
    secp256k1_ec_pubkey_serialize(ctx, pubkey_buf, &pubkey_len, &pubkey,
        compressed ? SECP256K1_EC_COMPRESSED : SECP256K1_EC_UNCOMPRESSED);

    privkey[pos++] = 0xA1;
    privkey[pos++] = (unsigned char)(pubkey_len + 3);
    privkey[pos++] = 0x03;
    privkey[pos++] = (unsigned char)(pubkey_len + 1);
    privkey[pos++] = 0x00;
    memcpy(&privkey[pos], pubkey_buf, pubkey_len);
    pos += pubkey_len;
    seq_len += 3 + pubkey_len + 2;

    // Fill length
    privkey[seq_start] = (unsigned char)seq_len;

    *privkeylen = pos;
    return 1;
}

// ──────────────────────────────────────────────────────────────────────
// Helper: DER-импорт приватного ключа
// Заменяет d2i_ECPrivateKey.
// ──────────────────────────────────────────────────────────────────────
static int ec_privkey_import_der(const secp256k1_context *ctx,
                                 unsigned char *out32,
                                 const unsigned char *privkey,
                                 size_t privkeylen)
{
    const unsigned char *end = privkey + privkeylen;

    // SEQUENCE
    if (privkey + 2 > end) return 0;
    if (privkey[0] != 0x30) return 0;
    privkey++; // skip tag
    unsigned char seq_len = *privkey++;
    const unsigned char *seq_end = privkey + seq_len;
    if (seq_end > end) return 0;

    // INTEGER 1
    if (privkey + 3 > seq_end) return 0;
    if (privkey[0] != 0x02) return 0;
    if (privkey[1] != 0x01) return 0;
    if (privkey[2] != 0x01) return 0;
    privkey += 3;

    // OCTET STRING
    if (privkey + 2 > seq_end) return 0;
    if (privkey[0] != 0x04) return 0;
    unsigned char octet_len = privkey[1];
    privkey += 2;
    if (privkey + octet_len > seq_end) return 0;
    if (octet_len != 32) return 0;

    memcpy(out32, privkey, 32);

    return secp256k1_ec_seckey_verify(ctx, out32);
}

// ──────────────────────────────────────────────────────────────────────
// CKey — методы
// ──────────────────────────────────────────────────────────────────────

void CKey::SetCompressedPubKey()
{
    fCompressed = true;
    // Recompute public key if secret is set
    if (!vch.empty()) {
        secp256k1_pubkey pk;
        if (!secp256k1_ec_pubkey_create(secp256k1_context_sign, &pk, &vch[0]))
            return;
        unsigned char pubkey_buf[33];
        size_t pubkey_len = 33;
        secp256k1_ec_pubkey_serialize(secp256k1_context_sign, pubkey_buf, &pubkey_len, &pk,
            SECP256K1_EC_COMPRESSED);
        pubkey = CPubKey(std::vector<unsigned char>(pubkey_buf, pubkey_buf + pubkey_len));
    }
}

void CKey::Reset()
{
    vch.clear();
    pubkey = CPubKey();
    fValid = false;
    fCompressed = false;
}

CKey::CKey()
{
    Reset();
}

CKey::CKey(const CKey& b)
{
    vch = b.vch;
    pubkey = b.pubkey;
    fValid = b.fValid;
    fCompressed = b.fCompressed;
}

CKey& CKey::operator=(const CKey& b)
{
    if (this == &b)
        return *this;
    vch = b.vch;
    pubkey = b.pubkey;
    fValid = b.fValid;
    fCompressed = b.fCompressed;
    return *this;
}

CKey::~CKey()
{
    // vector и CPubKey сами освободят память
}

bool CKey::IsNull() const
{
    return !fValid;
}

bool CKey::IsCompressed() const
{
    return fCompressed;
}

bool CKey::CheckSignatureElement(const unsigned char *vch, int len, bool half)
{
    return CompareBigEndian(vch, len, vchZero, 0) > 0 &&
           CompareBigEndian(vch, len, half ? vchMaxModHalfOrder : vchMaxModOrder, 32) <= 0;
}

bool CKey::ReserealizeSignature(std::vector<unsigned char>& vchSig)
{
    if (vchSig.empty())
        return false;

    secp256k1_ecdsa_signature sig;
    if (!secp256k1_ecdsa_signature_parse_der(secp256k1_context_sign, &sig, &vchSig[0], vchSig.size()))
        return false;

    vchSig.resize(72);
    size_t nSigLen = 72;
    if (!secp256k1_ecdsa_signature_serialize_der(secp256k1_context_sign, &vchSig[0], &nSigLen, &sig))
        return false;

    vchSig.resize(nSigLen);
    return true;
}

void CKey::MakeNewKey(bool fCompressed)
{
    unsigned char buf[32];
    do {
        for (int i = 0; i < 32; i++)
            buf[i] = (unsigned char)(rand() & 0xFF);
    } while (!secp256k1_ec_seckey_verify(secp256k1_context_sign, buf));

    vch.assign(buf, buf + 32);
    fValid = true;
    fCompressed = false;
    if (fCompressed)
        SetCompressedPubKey();

    // Derive pubkey
    secp256k1_pubkey pk;
    secp256k1_ec_pubkey_create(secp256k1_context_sign, &pk, &vch[0]);
    unsigned char pubkey_buf[65];
    size_t pubkey_len = 65;
    secp256k1_ec_pubkey_serialize(secp256k1_context_sign, pubkey_buf, &pubkey_len, &pk,
        fCompressed ? SECP256K1_EC_COMPRESSED : SECP256K1_EC_UNCOMPRESSED);
    pubkey = CPubKey(std::vector<unsigned char>(pubkey_buf, pubkey_buf + pubkey_len));
}

bool CKey::SetPrivKey(const CPrivKey& vchPrivKey)
{
    unsigned char buf[32];
    if (!ec_privkey_import_der(secp256k1_context_sign, buf, &vchPrivKey[0], vchPrivKey.size()))
    {
        Reset();
        return false;
    }

    vch.assign(buf, buf + 32);
    fValid = true;

    // Derive pubkey
    secp256k1_pubkey pk;
    if (!secp256k1_ec_pubkey_create(secp256k1_context_sign, &pk, &vch[0])) {
        Reset();
        return false;
    }

    unsigned char pubkey_buf[65];
    size_t pubkey_len = 65;
    secp256k1_ec_pubkey_serialize(secp256k1_context_sign, pubkey_buf, &pubkey_len, &pk,
        fCompressed ? SECP256K1_EC_COMPRESSED : SECP256K1_EC_UNCOMPRESSED);
    pubkey = CPubKey(std::vector<unsigned char>(pubkey_buf, pubkey_buf + pubkey_len));

    return true;
}

bool CKey::SetSecret(const CSecret& vchSecret, bool fCompressed)
{
    if (vchSecret.size() != 32)
        throw key_error("CKey::SetSecret() : secret must be 32 bytes");

    if (!secp256k1_ec_seckey_verify(secp256k1_context_sign, &vchSecret[0]))
        return false;

    vch = vchSecret;
    fValid = true;
    fCompressed = fCompressed;

    secp256k1_pubkey pk;
    if (!secp256k1_ec_pubkey_create(secp256k1_context_sign, &pk, &vch[0])) {
        Reset();
        return false;
    }

    unsigned char pubkey_buf[65];
    size_t pubkey_len = 65;
    secp256k1_ec_pubkey_serialize(secp256k1_context_sign, pubkey_buf, &pubkey_len, &pk,
        fCompressed ? SECP256K1_EC_COMPRESSED : SECP256K1_EC_UNCOMPRESSED);
    pubkey = CPubKey(std::vector<unsigned char>(pubkey_buf, pubkey_buf + pubkey_len));

    return true;
}

CSecret CKey::GetSecret(bool &fCompressed_out) const
{
    CSecret vchRet;
    vchRet.resize(32);
    if (!vch.empty())
        memcpy(&vchRet[0], &vch[0], 32);
    fCompressed_out = fCompressed;
    return vchRet;
}

CPrivKey CKey::GetPrivKey() const
{
    CPrivKey vchPrivKey;
    vchPrivKey.resize(279);
    size_t privkeylen = 279;

    if (!ec_privkey_export_der(secp256k1_context_sign,
                               (unsigned char*)&vchPrivKey[0], &privkeylen,
                               &vch[0],
                               fCompressed ? SECP256K1_EC_COMPRESSED : SECP256K1_EC_UNCOMPRESSED))
        throw key_error("CKey::GetPrivKey() : export failed");

    vchPrivKey.resize(privkeylen);
    return vchPrivKey;
}

bool CKey::SetPubKey(const CPubKey& vchPubKey)
{
    // Проверка: pubkey должен быть валидным
    if (!vchPubKey.IsValid())
        return false;

    vch.clear();
    pubkey = vchPubKey;
    fCompressed = vchPubKey.IsCompressed();
    fValid = true;
    return true;
}

CPubKey CKey::GetPubKey() const
{
    return pubkey;
}

bool CKey::Sign(uint256 hash, std::vector<unsigned char>& vchSig)
{
    if (!fValid || vch.empty())
        return false;

    vchSig.resize(72);
    size_t nSigLen = 72;
    secp256k1_ecdsa_signature sig;

    int ret = secp256k1_ecdsa_sign(
        secp256k1_context_sign,
        &sig,
        hash.begin(),
        &vch[0],
        secp256k1_nonce_function_rfc6979,
        NULL);

    if (!ret)
        return false;

    ret = secp256k1_ecdsa_signature_serialize_der(
        secp256k1_context_sign,
        &vchSig[0],
        &nSigLen,
        &sig);

    if (!ret)
        return false;

    vchSig.resize(nSigLen);
    return true;
}

bool CKey::SignCompact(uint256 hash, std::vector<unsigned char>& vchSig)
{
    if (!fValid || vch.empty())
        return false;

    secp256k1_ecdsa_recoverable_signature sig;
    if (!secp256k1_ecdsa_sign_recoverable(
            secp256k1_context_sign, &sig,
            hash.begin(), &vch[0],
            secp256k1_nonce_function_rfc6979, NULL))
        return false;

    vchSig.resize(65);
    int recid = 0;
    if (!secp256k1_ecdsa_recoverable_signature_serialize_compact(
            secp256k1_context_sign, &vchSig[1], &recid, &sig))
        return false;

    vchSig[0] = recid + 27 + (fCompressed ? 4 : 0);
    return true;
}

bool CKey::SetCompactSignature(uint256 hash, const std::vector<unsigned char>& vchSig)
{
    if (vchSig.size() != 65)
        return false;

    unsigned char nV = vchSig[0];
    if (nV < 27 || nV >= 35)
        return false;

    secp256k1_ecdsa_recoverable_signature sig;
    if (!secp256k1_ecdsa_recoverable_signature_parse_compact(
            secp256k1_context_sign, &sig, &vchSig[1], nV - 27))
        return false;

    secp256k1_pubkey pubkey_raw;
    if (!secp256k1_ecdsa_recover(secp256k1_context_sign, &pubkey_raw, &sig, hash.begin()))
        return false;

    // Reset state
    vch.clear();
    fValid = true;
    fCompressed = (nV >= 31);

    unsigned char pubkey_buf[65];
    size_t pubkey_len = 65;
    secp256k1_ec_pubkey_serialize(secp256k1_context_sign, pubkey_buf, &pubkey_len, &pubkey_raw,
        fCompressed ? SECP256K1_EC_COMPRESSED : SECP256K1_EC_UNCOMPRESSED);
    pubkey = CPubKey(std::vector<unsigned char>(pubkey_buf, pubkey_buf + pubkey_len));

    return true;
}

bool CKey::Verify(uint256 hash, const std::vector<unsigned char>& vchSig)
{
    if (vchSig.empty() || !pubkey.IsValid())
        return false;

    std::vector<unsigned char> raw = pubkey.Raw();
    secp256k1_pubkey pk;
    if (!secp256k1_ec_pubkey_parse(secp256k1_context_sign, &pk, &raw[0], raw.size()))
        return false;

    secp256k1_ecdsa_signature sig;
    if (!secp256k1_ecdsa_signature_parse_der(secp256k1_context_sign, &sig, &vchSig[0], vchSig.size()))
        return false;

    secp256k1_ecdsa_signature normalized;
    secp256k1_ecdsa_signature_normalize(secp256k1_context_sign, &normalized, &sig);

    return secp256k1_ecdsa_verify(secp256k1_context_sign, &normalized, hash.begin(), &pk) == 1;
}

bool CKey::VerifyCompact(uint256 hash, const std::vector<unsigned char>& vchSig)
{
    CKey key;
    if (!key.SetCompactSignature(hash, vchSig))
        return false;
    return GetPubKey() == key.GetPubKey();
}

bool CKey::IsValid()
{
    if (!fValid)
        return false;

    if (!vch.empty()) {
        if (!secp256k1_ec_seckey_verify(secp256k1_context_sign, &vch[0]))
            return false;
    }

    return true;
}

// ──────────────────────────────────────────────────────────────────────
// CPubKey::Verify
// ──────────────────────────────────────────────────────────────────────
bool CPubKey::Verify(const uint256 &hash, const std::vector<unsigned char>& vchSig) const
{
    if (!IsValid() || vchSig.empty())
        return false;

    secp256k1_pubkey pubkey;
    if (!secp256k1_ec_pubkey_parse(secp256k1_context_sign, &pubkey, &vchPubKey[0], vchPubKey.size()))
        return false;

    secp256k1_ecdsa_signature sig;
    if (!secp256k1_ecdsa_signature_parse_der(secp256k1_context_sign, &sig, &vchSig[0], vchSig.size()))
        return false;

    secp256k1_ecdsa_signature normalized;
    secp256k1_ecdsa_signature_normalize(secp256k1_context_sign, &normalized, &sig);

    return secp256k1_ecdsa_verify(secp256k1_context_sign, &normalized, (const unsigned char*)&hash, &pubkey) == 1;
}
