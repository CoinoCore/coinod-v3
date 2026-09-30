// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_KEY_H
#define BITCOIN_KEY_H

#include <stdexcept>
#include <vector>

#include "allocators.h"
#include "serialize.h"
#include "uint256.h"
#include "hash.h"

// libsecp256k1 — заменяет OpenSSL для ECDSA
#include <secp256k1.h>
#include <secp256k1_recovery.h>

class key_error : public std::runtime_error
{
public:
    explicit key_error(const std::string& str) : std::runtime_error(str) {}
};

/** A reference to a CKey: the Hash160 of its serialized public key */
class CKeyID : public uint160
{
public:
    CKeyID() : uint160(0) { }
    CKeyID(const uint160 &in) : uint160(in) { }
};

/** A reference to a CScript: the Hash160 of its serialization */
class CScriptID : public uint160
{
public:
    CScriptID() : uint160(0) { }
    CScriptID(const uint160 &in) : uint160(in) { }
};

/** An encapsulated public key. */
class CPubKey {
private:
    std::vector<unsigned char> vchPubKey;
    friend class CKey;

public:
    CPubKey() { }
    CPubKey(const std::vector<unsigned char> &vchPubKeyIn) : vchPubKey(vchPubKeyIn) { }

    friend bool operator==(const CPubKey &a, const CPubKey &b) { return a.vchPubKey == b.vchPubKey; }
    friend bool operator!=(const CPubKey &a, const CPubKey &b) { return a.vchPubKey != b.vchPubKey; }
    friend bool operator<(const CPubKey &a, const CPubKey &b) { return a.vchPubKey < b.vchPubKey; }

    IMPLEMENT_SERIALIZE(
        READWRITE(vchPubKey);
    )

    CKeyID GetID() const {
        return CKeyID(Hash160(vchPubKey));
    }

    uint256 GetHash() const {
        return Hash(vchPubKey.begin(), vchPubKey.end());
    }

    bool IsValid() const {
        return vchPubKey.size() == 33 || vchPubKey.size() == 65;
    }

    bool IsCompressed() const {
        return vchPubKey.size() == 33;
    }

    std::vector<unsigned char> Raw() const {
        return vchPubKey;
    }

    /** Verify a signature against this public key. */
    bool Verify(const uint256 &hash, const std::vector<unsigned char>& vchSig) const;
};

// secure_allocator is defined in allocators.h
typedef std::vector<unsigned char, secure_allocator<unsigned char> > CPrivKey;
typedef std::vector<unsigned char, secure_allocator<unsigned char> > CSecret;

/** An encapsulated secp256k1 key (public and/or private). */
class CKey
{
protected:
    // 32-byte secret key (empty if pubkey-only)
    std::vector<unsigned char, secure_allocator<unsigned char> > vch;

    // The public key
    CPubKey pubkey;

    // Has this key been initialized?
    bool fValid;

    // Is the public key compressed?
    bool fCompressed;

    void SetCompressedPubKey();

public:

    void Reset();

    CKey();
    CKey(const CKey& b);
    CKey& operator=(const CKey& b);
    ~CKey();

    bool IsNull() const;
    bool IsCompressed() const;

    void MakeNewKey(bool fCompressed);
    bool SetPrivKey(const CPrivKey& vchPrivKey);
    bool SetSecret(const CSecret& vchSecret, bool fCompressed = false);
    CSecret GetSecret(bool &fCompressed) const;
    CPrivKey GetPrivKey() const;
    bool SetPubKey(const CPubKey& vchPubKey);
    CPubKey GetPubKey() const;

    bool Sign(uint256 hash, std::vector<unsigned char>& vchSig);

    bool SignCompact(uint256 hash, std::vector<unsigned char>& vchSig);
    bool SetCompactSignature(uint256 hash, const std::vector<unsigned char>& vchSig);
    bool Verify(uint256 hash, const std::vector<unsigned char>& vchSig);
    bool VerifyCompact(uint256 hash, const std::vector<unsigned char>& vchSig);
    bool IsValid();

    static bool CheckSignatureElement(const unsigned char *vch, int len, bool half);
    static bool ReserealizeSignature(std::vector<unsigned char>& vchSig);
};

// libsecp256k1 lifecycle — called from init.cpp
void ECC_Start();
void ECC_Stop();

#endif
