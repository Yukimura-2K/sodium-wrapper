// test_CryptorPK.cpp -- Test Sodium::CryptorPK
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

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Sodium::CryptorPK Test
#include <boost/test/included/unit_test.hpp>

#include <sodium.h>
#include "cryptorpk.h"
#include "keypair.h"
#include "nonce.h"
#include <string>

using Sodium::KeyPair;
using Sodium::Nonce;
using Sodium::CryptorPK;
using data_t = Sodium::data_t;

bool
test_of_correctness(const std::string &plaintext)
{
  CryptorPK               sc            {};
  KeyPair                 keypair_alice {};
  KeyPair                 keypair_bob   {};
  Nonce<CryptorPK::NSZPK> nonce         {};

  data_t plainblob {plaintext.cbegin(), plaintext.cend()};

  // 1. alice gets the public key from bob, and sends him a message
  
  data_t ciphertext_from_alice_to_bob =
    sc.encrypt(plainblob,
	       keypair_bob.pubkey(),
	       keypair_alice.privkey(),
	       nonce);

  // 2. bob gets the public key from alice, and decrypts the message
  
  data_t decrypted_by_bob_from_alice =
    sc.decrypt(ciphertext_from_alice_to_bob,
	       keypair_bob.privkey(),
	       keypair_alice.pubkey(),
	       nonce);

  // 3. if decryption (MAC or signature) fails, decrypt() would throw,
  // but we manually check anyway.
  
  BOOST_CHECK(plainblob == decrypted_by_bob_from_alice);

  // 4. bob echoes the messages back to alice. Remember to increment nonce!

  nonce.increment(); // IMPORTANT! before calling encrypt() again

  data_t ciphertext_from_bob_to_alice =
    sc.encrypt(decrypted_by_bob_from_alice,
	       keypair_alice.pubkey(),
	       keypair_bob.privkey(),
	       nonce);

  // 5. alice attempts to decrypt again (also with the incremented nonce)
  data_t decrypted_by_alice_from_bob =
    sc.decrypt(ciphertext_from_bob_to_alice,
	       keypair_alice.privkey(),
	       keypair_bob.pubkey(),
	       nonce);

  // 6. if decryption (MAC or signature) fails, decrypt() would throw,
  // but we manually check anyway. We assume that bob echoed the
  // plaintext without modifying it.

  BOOST_CHECK(plainblob == decrypted_by_alice_from_bob);
  
  return plainblob == decrypted_by_alice_from_bob;
}

bool
falsify_mac(const std::string &plaintext)
{
  CryptorPK               sc            {};
  KeyPair                 keypair_alice {};
  Nonce<CryptorPK::NSZPK> nonce         {};

  data_t plainblob {plaintext.cbegin(), plaintext.cend()};

  data_t ciphertext = sc.encrypt(plainblob,
				 keypair_alice,
				 nonce);

  BOOST_CHECK(ciphertext.size() >= CryptorPK::MACSIZE);

  // falsify mac, which starts before the ciphertext proper
  ++ciphertext[0];

  try {
    data_t decrypted = sc.decrypt(ciphertext,
				  keypair_alice,
				  nonce);
  }
  catch (std::exception &e) {
    // decryption failed as expected: test passed.
    return true;
  }

  // No expection caught: decryption went ahead, eventhough we've
  // modified the mac. Test failed.

  return false;
}

bool
falsify_ciphertext(const std::string &plaintext)
{
  // before even bothering falsifying a ciphertext, check that the
  // corresponding plaintext is not emptry!
  BOOST_CHECK_MESSAGE(! plaintext.empty(),
		      "Nothing to falsify, empty plaintext");
  
  CryptorPK               sc            {};
  KeyPair                 keypair_alice {};
  Nonce<CryptorPK::NSZPK> nonce         {};

  data_t plainblob {plaintext.cbegin(), plaintext.cend()};

  // encrypt to self
  data_t ciphertext = sc.encrypt(plainblob,
				 keypair_alice,
				 nonce);

  BOOST_CHECK(ciphertext.size() > CryptorPK::MACSIZE);

  // falsify ciphertext, which starts just after MAC
  ++ciphertext[CryptorPK::MACSIZE];

  try {
    data_t decrypted = sc.decrypt(ciphertext,
				  keypair_alice,
				  nonce);
  }
  catch (std::exception &e) {
    // Exception caught as expected. Test passed.
    return true;
  }

  // Expection not caught: decryption went ahead, eventhough we've
  // modified the ciphertext. Test failed.

  return false;
}

struct SodiumFixture {
  SodiumFixture()  {
    BOOST_REQUIRE(sodium_init() != -1);
    BOOST_TEST_MESSAGE("SodiumFixture(): sodium_init() successful.");
  }
  ~SodiumFixture() {
    BOOST_TEST_MESSAGE("~SodiumFixture(): teardown -- no-op.");
  }
};

BOOST_FIXTURE_TEST_SUITE ( sodium_test_suite, SodiumFixture );

BOOST_AUTO_TEST_CASE( sodium_cryptorpk_test_full_plaintext )
{
  std::string plaintext {"the quick brown fox jumps over the lazy dog"};
  BOOST_CHECK(test_of_correctness(plaintext));
}

BOOST_AUTO_TEST_CASE( sodium_cryptorpk_test_empty_plaintext )
{
  std::string plaintext {};
  BOOST_CHECK(test_of_correctness(plaintext));
}

BOOST_AUTO_TEST_CASE( sodium_cryptopk_test_encrypt_to_self )
{
  CryptorPK               sc            {};
  KeyPair                 keypair_alice {};
  Nonce<CryptorPK::NSZPK> nonce         {};

  std::string plaintext {"the quick brown fox jumps over the lazy dog"};
  data_t plainblob {plaintext.cbegin(), plaintext.cend()};

  data_t ciphertext = sc.encrypt(plainblob,
				 keypair_alice,
				 nonce);

  BOOST_CHECK_EQUAL(ciphertext.size(), plainblob.size() + CryptorPK::MACSIZE);

  data_t decrypted = sc.decrypt(ciphertext,
				keypair_alice,
				nonce);

  // if the ciphertext (with MAC) was modified, or came from another
  // source, decryption would have thrown. But we manually check anyway.

  BOOST_CHECK(plainblob == decrypted);
}

BOOST_AUTO_TEST_CASE( sodium_cryptopk_test_falsify_ciphertext )
{
  std::string plaintext {"the quick brown fox jumps over the lazy dog"};

  BOOST_CHECK(falsify_ciphertext(plaintext));
}

BOOST_AUTO_TEST_CASE( sodium_cryptopk_test_falsify_mac_fulltext )
{
  std::string plaintext {"the quick brown fox jumps over the lazy dog"};

  BOOST_CHECK(falsify_mac(plaintext));
}

BOOST_AUTO_TEST_CASE( sodium_cryptopk_test_falsify_mac_empty )
{
  std::string plaintext {};

  BOOST_CHECK(falsify_mac(plaintext));
}

BOOST_AUTO_TEST_SUITE_END ();
