// filecryptor.h -- file encryption/decryption
//
// Copyright (C) 2017 Farid Hajji <farid@hajji.name>. All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// 
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef _S_FILECRYPTOR_H_
#define _S_FILECRYPTOR_H_

#include "key.h"
#include "nonce.h"
#include "cryptoraead.h"

#include <sodium.h>

namespace Sodium {

class FileCryptor {
 public:

  /**
   * Each block of plaintext will be encrypted to a block of the
   * same size of ciphertext, combined with a MAC of size MACSIZE.
   * Note that the total blocksize of the mac+ciphertext will be
   * MACSIZE + plaintext.size() for each block.
   **/
  constexpr static std::size_t MACSIZE = CryptorAEAD::MACSIZE;

  /**
   * We can compute the hash of the (MAC+)ciphertext of the whole
   * file using a key of so many bytes:
   *   - HASHKEYSIZE is the recommended number of key bytes
   *   - HASHKEYSIZE_MIN is the minimum number of key bytes
   *   - HASHKEYSIZE_MAX is the maximum number of key bytes
   **/
  constexpr static std::size_t HASHKEYSIZE     = Key::KEYSIZE_HASHKEY;
  constexpr static std::size_t HASHKEYSIZE_MIN = Key::KEYSIZE_HASHKEY_MIN;
  constexpr static std::size_t HASHKEYSIZE_MAX = Key::KEYSIZE_HASHKEY_MAX;

  /**
   * The hash can be stored in so many bytes:
   *   - HASHSIZE is the minimum recommended number of bytes
   *   - HASHSIZE_MIN is the minimum number of bytes
   *   - HASHSIZE_MAX is the maximum number of bytes
   **/

  constexpr static std::size_t HASHSIZE     = crypto_generichash_BYTES;
  constexpr static std::size_t HASHSIZE_MIN = crypto_generichash_BYTES_MIN;
  constexpr static std::size_t HASHSIZE_MAX = crypto_generichash_BYTES_MAX;
  
  /**
   * Encrypt/Decrypt a file using a key, an initial nonce, and a
   * fixed blocksize, using the algorithm of Sodium::StreamCryptor:
   *
   * Each block is encrypted individually, using the key and a running
   * nonce initialized with the initial nonce; and an authenticated
   * MAC is appended to the ciphertext to prevent individual tampering
   * of the blocks. The running nonce is incremented after each block,
   * so that swapping of ciphertext blocks is detected.
   *
   * To detect truncating of whole blocks of ciphertext at the end
   * (and to further detect tampering in the midst of the file),
   * generic hashing with a hashing key (preferably HASHSIZE bytes,
   * but no less than HASHSIZE_MIN and no more than HASHSIZE_MAX
   * bytes) is applied to the whole ciphertext+MACs, and that hash is
   * appended to the end of the file.
   *
   * The size of the hash can be selected by the user. It is perferably
   * HASHSIZE bytes, but should be no less than HASHSIZE_MIN and no
   * more than HASHSIZE_MAX bytes. To decrypt the file, the size of
   * the hash MUST be the same as the one given here.
   **/

  FileCryptor(const Key &key,
	      const Nonce<NONCESIZE_AEAD> &nonce,
	      const std::size_t blocksize,
	      const Key &hashkey,
	      const std::size_t hashsize) :
  key_ {key}, nonce_ {nonce}, header_ {}, blocksize_ {blocksize},
  hashkey_ {hashkey}, hashsize_ {hashsize} {
      // some sanity checks, before we start
      if (key.size() != Key::KEYSIZE_AEAD)
	throw std::runtime_error {"Sodium::FileCryptor(): wrong key size"};
      if (blocksize < 1)
	throw std::runtime_error {"Sodium::FileCryptor(): wrong blocksize"};
      if (hashkey.size() < HASHKEYSIZE_MIN)
	throw std::runtime_error {"Sodium::FileCryptor(): hash key too small"};
      if (hashkey.size() > HASHKEYSIZE_MAX)
	throw std::runtime_error {"Sodium::FileCryptor(): hash key too big"};

  }

  /**
   * Encrypt the input stream ISTR in a blockwise fashion, using the
   * algorithm described in Sodium::CryptorAEAD, and write the result
   * in output stream OSTR.
   * 
   * At the same time, compute a generic hash over the resulting
   * ciphertexts and MACs, and when reaching the EOF of ISTR, write
   * that hash at the end of OSTR. The hash is authenticated with the
   * key HASHKEY and will have a size HASHSIZE (bytes).
   **/
  
  void encrypt(std::istream &istr, std::ostream &ostr);

  /**
   * Decrypt the input _file_ stream IFS in a blockwise fashion, using
   * the algorithm described in Sodium::CrytorAEAD, and write the result
   * in output _stream_ OSTR.
   *
   * At the same time, compute a generic authenticated hash of
   * HASHSIZE (bytes) over the input ciphertexts and MACs, using the
   * key HASHKEY. Compare that hash with the HASHSIZE bytes stored at
   * the end of IFS.
   *
   * If the the decryption fails for whatever reason:
   *   - the decryption itself fails
   *   - one of the MACs doesn't verify
   *   - the reading or writing fails
   *   - the verification of the authenticated hash fails
   * this function throws a std::runtime_error. It doesn't provide
   * a strong guarantee: some data may already have been written
   * to OSTR prior to throwing.
   *
   * To be able to decrypt a file, a user must provide:
   *   the key, the initial nonce, the blocksize,
   *   the authentication key for the hash (with the right number of bytes),
   *   the hashsize, i.e. the number of bytes of the hash at the end.
   * Failing to provide all those informations, decryption will fail.
   **/
  
  void decrypt(std::ifstream &ifs, std::ostream &ostr);

 private:
  Key                   key_, hashkey_;
  Nonce<NONCESIZE_AEAD> nonce_;
  data_t                header_;
  std::size_t           blocksize_, hashsize_;
  
  CryptorAEAD           sc_aead_;
};
  
} // namespace Sodium

#endif // _S_FILECRYPTOR_H_