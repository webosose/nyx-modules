// Copyright (c) 2016-2018 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include <nyx/nyx_client.h>
#include <assert.h>
#include <stdio.h>
#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/obj_mac.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#define AES_BLOCK_SIZE 16
#define DES3_BLOCK_SIZE 8

struct Fixture
{
	nyx_device_handle_t device;
};

static void test_crypt_aes(int simplicity, int keybits,
                           const unsigned char *keydata, nyx_security_block_mode_t block_mode,
                           struct Fixture *f);
static void test_crypt_3des(int simplicity, const unsigned char *keydata,
                            nyx_security_block_mode_t block_mode, struct Fixture *f);
static void test_crypt_hmac(const unsigned char *keydata, int keybits,
                            struct Fixture *f);
static void test_crypt_rsa(int keybits, int serializedLen,
                           const unsigned char *keydata, struct Fixture *f, const unsigned char *public,
                           int pubLen);

static void fixture_setup(struct Fixture *f, gconstpointer userdata)
{
	g_assert(NYX_ERROR_NONE == nyx_init());

	f->device = NULL;
	g_assert_cmpint(NYX_ERROR_NONE, ==, nyx_device_open(NYX_DEVICE_SECURITY2,
	                "Main", &f->device));
	g_assert(f->device != NULL);
}

static void fixture_teardown(struct Fixture *f, gconstpointer userdata)
{
	g_assert_cmpint(NYX_ERROR_NONE, ==, nyx_device_close(f->device));
	g_assert_cmpint(NYX_ERROR_NONE, ==, nyx_deinit());
}

void createAndCryptAes(int keylen, nyx_security_block_mode_t mode,
                       struct Fixture *f)
{
	printf("\naes start\n");
	unsigned char data[keylen / 8];
	g_assert_cmpint(NYX_ERROR_NONE, ==, nyx_security2_create_aes_key(f->device,
	                keylen, data));
	printf("\naes len = %d\n", keylen);
	printf("aes mode = %d\n", mode);
	test_crypt_aes(0, keylen, data, mode, f);
	test_crypt_aes(1, keylen, data, mode, f);
	test_crypt_aes(2, keylen, data, mode, f);
}

static void test_create_aes_key(struct Fixture *f, gconstpointer userdata)
{
	createAndCryptAes(128, NYX_SECURITY_MODE_CBC, f);
	createAndCryptAes(128, NYX_SECURITY_MODE_ECB, f);
	createAndCryptAes(128, NYX_SECURITY_MODE_CFB, f);
	createAndCryptAes(256, NYX_SECURITY_MODE_CBC, f);
	createAndCryptAes(256, NYX_SECURITY_MODE_ECB, f);
	createAndCryptAes(256, NYX_SECURITY_MODE_CFB, f);
}

static void create3des(struct Fixture *f, nyx_security_block_mode_t block_mode)
{
	unsigned char data[192 / 8];
	g_assert_cmpint(NYX_ERROR_NONE, ==, nyx_security2_create_des3_key(f->device,
	                192, data));

	test_crypt_3des(0, (const unsigned char *) data, block_mode, f);
	test_crypt_3des(1, (const unsigned char *) data, block_mode, f);
	test_crypt_3des(2, (const unsigned char *) data, block_mode, f);
}

static void test_create_3des_key(struct Fixture *f, gconstpointer userdata)
{
	create3des(f, NYX_SECURITY_MODE_CBC);
	create3des(f, NYX_SECURITY_MODE_ECB);
	create3des(f, NYX_SECURITY_MODE_CFB);
}

static void test_create_hmac_key(struct Fixture *f, gconstpointer userdata)
{
	int keylen = 256;
	unsigned char data[keylen / 8];
	g_assert_cmpint(NYX_ERROR_NONE, ==, nyx_security2_create_hmac_key(f->device,
	                keylen, data));
	test_crypt_hmac((const unsigned char *) data, keylen, f);
}

static void test_create_rsa_key(struct Fixture *f, gconstpointer userdata)
{
	g_assert(userdata != NULL); /* key length as userdata pointer */
	const int keylen = atoi((const char *)userdata);
	printf("\nEstimating public and private keys sizes\n");
	int serializedPrivLen = 0;
	int publicKeySize = 0;

	g_assert_cmpint(NYX_ERROR_NONE, ==, nyx_security2_create_rsa_key(f->device,
	                keylen, NULL, &serializedPrivLen, NULL, &publicKeySize));
	printf("\nprivlen = %d\n", serializedPrivLen);
	printf("\npublen = %d\n", publicKeySize);

	g_assert_cmpint(serializedPrivLen, >, 0);
	g_assert_cmpint(publicKeySize, >, 0);

	unsigned char data[serializedPrivLen];
	unsigned char public[publicKeySize];
	g_assert_cmpint(NYX_ERROR_NONE, ==, nyx_security2_create_rsa_key(f->device,
	                keylen, data, &serializedPrivLen, public, &publicKeySize));


	g_assert_cmpint(NYX_ERROR_NONE, ==, nyx_security2_create_rsa_key(f->device,
	                keylen, data, &serializedPrivLen, public, &publicKeySize));
	printf("\nrsa key priv len = %d\n", serializedPrivLen);
	{
		int i;

		for (i = 0; i < serializedPrivLen; i++)
		{
			printf("%02X", data[i]);
		}

		printf("\n");
	}

	printf("\nrsa key pub len = %d\n", publicKeySize);
	{
		int i;

		for (i = 0; i < publicKeySize; i++)
		{
			printf("%02X", public[i]);
		}

		printf("\n");
	}

	printf("crypt rsa start\n");
	printf("encrypt pub, decrypt private\n");
	test_crypt_rsa(keylen, serializedPrivLen, (const unsigned char *) data, f,
	               public, publicKeySize);
}

static void test_crypt_aes(int simplicity, int keybits,
                           const unsigned char *keydata, nyx_security_block_mode_t block_mode,
                           struct Fixture *f)
{
	printf("\naes key len = %d\n", keybits);
	{
		int i;

		for (i = 0; i < keybits / 8; i++)
		{
			printf("%02X", keydata[i]);
		}
	}
	printf("\n");
	unsigned char iv[AES_BLOCK_SIZE];
	unsigned char ivnext[AES_BLOCK_SIZE];
	char testEmptyStr[AES_BLOCK_SIZE];
	g_assert_cmpint(0, !=, RAND_bytes(iv, AES_BLOCK_SIZE));
	const char *src = "1234567890123456";
	printf("\nsrc = %s\n", src);
	unsigned char enc[sizeof(src) + EVP_MAX_BLOCK_LENGTH + EVP_MAX_IV_LENGTH];
	unsigned char dec[sizeof(src) + EVP_MAX_BLOCK_LENGTH + EVP_MAX_IV_LENGTH];
	int enclen = -1;
	int declen = -1;

	memset(enc, 0, sizeof(src) + EVP_MAX_BLOCK_LENGTH + EVP_MAX_IV_LENGTH);
	memset(enc, 0, sizeof(src) + EVP_MAX_BLOCK_LENGTH + EVP_MAX_IV_LENGTH);

	printf("simplicity = %d\n", simplicity);

	switch (simplicity)
	{
		case 0:
		default:
			g_assert_cmpint(NYX_ERROR_NONE, ==, nyx_security2_crypt_aes_simple(f->device,
			                keydata, keybits,
			                block_mode, 1, (const unsigned char *) src, 16, enc, &enclen));
			break;

		case 1:
			g_assert_cmpint(NYX_ERROR_NONE, ==, nyx_security2_crypt_aes(f->device, keydata,
			                keybits,
			                block_mode, 1, (const unsigned char *) src, 16, enc, &enclen, iv,
			                AES_BLOCK_SIZE, NYX_SECURITY_PADDING_PKCS5, NULL, 0));
			break;

		case 2:
			memset(ivnext, 0, AES_BLOCK_SIZE);
			memset(testEmptyStr, 0, AES_BLOCK_SIZE);
			g_assert_cmpint(NYX_ERROR_NONE, ==, nyx_security2_crypt_aes(f->device, keydata,
			                keybits,
			                block_mode, 1, (const unsigned char *) src, 16, enc, &enclen, iv,
			                AES_BLOCK_SIZE, NYX_SECURITY_PADDING_NONE, ivnext, AES_BLOCK_SIZE));

			if (block_mode != NYX_SECURITY_MODE_ECB)
			{
				const char *ivstr = (const char *) ivnext;
				printf("\nnext iv = %d\n", enclen);
				{
					int i;

					for (i = 0; i < AES_BLOCK_SIZE; i++)
					{
						printf("%02X", ivnext[i]);
					}
				}
				printf("\n");
				g_assert_cmpstr(ivstr, !=, testEmptyStr);
			}

			break;
	}

	g_assert_cmpint(enclen, >, 0);
	printf("encoded: %s\n", enc);

	printf("\nencoded len = %d\n", enclen);
	{
		int i;

		for (i = 0; i < enclen; i++)
		{
			printf("%02X", enc[i]);
		}
	}

	switch (simplicity)
	{
		case 0:
		default:
			g_assert_cmpint(NYX_ERROR_NONE, ==, nyx_security2_crypt_aes_simple(f->device,
			                keydata, keybits,
			                block_mode, 0, enc, enclen, dec, &declen));
			break;

		case 1:
			g_assert_cmpint(NYX_ERROR_NONE, ==, nyx_security2_crypt_aes(f->device, keydata,
			                keybits,
			                block_mode, 0, enc, enclen, dec, &declen, iv, AES_BLOCK_SIZE,
			                NYX_SECURITY_PADDING_PKCS5, NULL, 0));
			break;

		case 2:
			memset(ivnext, 0, AES_BLOCK_SIZE);
			g_assert_cmpint(NYX_ERROR_NONE, ==, nyx_security2_crypt_aes(f->device, keydata,
			                keybits,
			                block_mode, 0, enc, enclen, dec, &declen, iv, AES_BLOCK_SIZE,
			                NYX_SECURITY_PADDING_NONE, ivnext, AES_BLOCK_SIZE));
			break;
	}

	g_assert_cmpint(declen, >, 0);
	memset(dec + declen, 0, sizeof(src) + EVP_MAX_BLOCK_LENGTH + EVP_MAX_IV_LENGTH -
	       declen);

	printf("\n dencoded len = %d\n", declen);
	{
		int i;

		for (i = 0; i < declen; i++)
		{
			printf("%c", dec[i]);
		}
	}
	printf("\n");

	char cmpDec[declen + 1];
	memset(cmpDec, 0, declen + 1);
	memcpy(cmpDec, dec, declen);

	printf("\n cmpDec len = %d\n", declen + 1);
	{
		int i;

		for (i = 0; i < declen + 1; i++)
		{
			printf("%c", (char) cmpDec[i]);
		}

		printf("\n");
	}

	g_assert_cmpstr(src, ==, cmpDec);
}

static void test_crypt_3des(int simplicity, const unsigned char *keydata,
                            nyx_security_block_mode_t block_mode, struct Fixture *f)
{
	const char *src = "12345678";
	printf("src = %s\n", src);

	printf("\n 3des key len = %d\n", 192 / 8);
	{
		int i;

		for (i = 0; i < 192 / 8; i++)
		{
			printf("%02X", keydata[i]);
		}
	}
	printf("\n");

	unsigned char iv[DES3_BLOCK_SIZE];
	g_assert_cmpint(0, !=, RAND_bytes(iv, DES3_BLOCK_SIZE));
	unsigned char ivnext[DES3_BLOCK_SIZE];
	char testEmptyStr[DES3_BLOCK_SIZE];

	unsigned char enc[sizeof(src) + EVP_MAX_BLOCK_LENGTH + EVP_MAX_IV_LENGTH];
	unsigned char dec[sizeof(src) + EVP_MAX_BLOCK_LENGTH + EVP_MAX_IV_LENGTH];
	int enclen = -1;
	int declen = -1;

	switch (simplicity)
	{
		case 0:
		default:
			g_assert_cmpint(NYX_ERROR_NONE, ==, nyx_security2_crypt_des3_simple(f->device,
			                keydata,
			                block_mode, 1, (const unsigned char *) src, 8, enc, &enclen));
			break;

		case 1:
			g_assert_cmpint(NYX_ERROR_NONE, ==, nyx_security2_crypt_des3(f->device, keydata,
			                block_mode, 1, (const unsigned char *) src, 8, enc, &enclen, iv,
			                DES3_BLOCK_SIZE, NYX_SECURITY_PADDING_PKCS5, NULL, 0));
			break;

		case 2:
			memset(ivnext, 0, DES3_BLOCK_SIZE);
			memset(testEmptyStr, 0, DES3_BLOCK_SIZE);
			g_assert_cmpint(NYX_ERROR_NONE, ==, nyx_security2_crypt_des3(f->device, keydata,
			                block_mode, 1, (const unsigned char *) src, 8, enc, &enclen, iv,
			                DES3_BLOCK_SIZE, NYX_SECURITY_PADDING_NONE, ivnext, DES3_BLOCK_SIZE));

			if (block_mode != NYX_SECURITY_MODE_ECB)
			{
				const char *ivstr = (const char *) ivnext;
				printf("\nnext iv = %d\n", enclen);
				{
					int i;

					for (i = 0; i < DES3_BLOCK_SIZE; i++)
					{
						printf("%02X", ivnext[i]);
					}
				}
				printf("\n");
				g_assert_cmpstr(ivstr, !=, testEmptyStr);
			}

			break;
	}

	g_assert_cmpint(enclen, >, 0);

	printf("\n 3des enc len = %d\n", enclen);
	{
		int i;

		for (i = 0; i < enclen; i++)
		{
			printf("%02X", enc[i]);
		}
	}
	printf("\n");

	switch (simplicity)
	{
		case 0:
		default:
			g_assert_cmpint(NYX_ERROR_NONE, ==, nyx_security2_crypt_des3_simple(f->device,
			                keydata,
			                block_mode, 0, enc, enclen, dec, &declen));
			break;

		case 1:
			g_assert_cmpint(NYX_ERROR_NONE, ==, nyx_security2_crypt_des3(f->device, keydata,
			                block_mode, 0, enc, enclen, dec, &declen, iv, DES3_BLOCK_SIZE,
			                NYX_SECURITY_PADDING_PKCS5, NULL, 0));
			break;

		case 2:
			memset(ivnext, 0, DES3_BLOCK_SIZE);
			g_assert_cmpint(NYX_ERROR_NONE, ==, nyx_security2_crypt_des3(f->device, keydata,
			                block_mode, 0, enc, enclen, dec, &declen, iv, DES3_BLOCK_SIZE,
			                NYX_SECURITY_PADDING_NONE, ivnext, DES3_BLOCK_SIZE));
			break;
	}

	g_assert_cmpint(declen, >, 0);

	printf("\n 3des dec len = %d\n", declen);
	{
		int i;

		for (i = 0; i < declen; i++)
		{
			printf("%c", dec[i]);
		}

		printf("\n");
	}

	char cmpDec[declen + 1];
	memset(cmpDec, 0, declen + 1);
	memcpy(cmpDec, dec, declen);

	printf("\n cmpDec len = %d\n", declen + 1);
	{
		int i;

		for (i = 0; i < declen + 1; i++)
		{
			printf("%c", cmpDec[i]);
		}

		printf("\n");
	}

	g_assert_cmpstr(src, ==, cmpDec);
}

static void test_crypt_hmac(const unsigned char *keydata, int keybits,
                            struct Fixture *f)
{
	const unsigned char *src = (const unsigned char *) "foobar";
	printf("src = %s\n", src);

	printf("\n hmac key len = %d\n", keybits / 8);
	{
		int i;

		for (i = 0; i < keybits / 8; i++)
		{
			printf("%02X", keydata[i]);
		}
	}

	unsigned char enc[sizeof(src) + EVP_MAX_BLOCK_LENGTH + EVP_MAX_IV_LENGTH];
	int enclen = -1;

	g_assert_cmpint(NYX_ERROR_NONE, ==, nyx_security2_hmac(f->device, keydata,
	                keybits,
	                src, strlen((const char *) src) + 1, enc, &enclen));

	printf("\n hmac enc len = %d\n", enclen);
	{
		int i;

		for (i = 0; i < 192 / 8; i++)
		{
			printf("%02X", enc[i]);
		}
	}

	printf("encoded: %s\n", enc);
	g_assert_cmpint(enclen, >, 0);
}

static void test_crypt_rsa(int keylen, int serializedLen,
                           const unsigned char *keydata, struct Fixture *f,
                           const unsigned char *public, int pubLen)
{

	printf("\n***** CRYPT RSA *****\n rsa key priv, len = %d\n", serializedLen);
	{
		int i;

		for (i = 0; i < serializedLen; i++)
		{
			printf("%02X", keydata[i]);
		}

		printf("\n");
	}

	printf("\nrsa key pub, len = %d\n", pubLen);
	{
		int i;

		for (i = 0; i < pubLen; i++)
		{
			printf("%02X", public[i]);
		}

		printf("\n");
	}

	g_assert_cmpint(keylen % 8, ==, 0);

	const char *src = "foobar";
	unsigned char enc[keylen / 8];
	unsigned char dec[keylen / 8];
	int enclen = -1;
	int declen = -1;

	printf("\n rsa key len = %d\n", serializedLen);
	{
		int i;

		for (i = 0; i <  serializedLen; i++)
		{
			printf("%02X", keydata[i]);
		}

		printf("\n");
	}

	g_assert_cmpint(NYX_ERROR_NONE, ==, nyx_security2_crypt_rsa(f->device, public,
	                pubLen, NYX_SECURITY_RSA_ENCRYPT,
	                (const unsigned char *) src, strlen(src) + 1, enc, &enclen));


	g_assert_cmpint(enclen, >, 0);


	printf("rsa enc len = %d\n", enclen);
	{
		int i;

		for (i = 0; i < 192 / 8; i++)
		{
			printf("%02X", enc[i]);
		}

		printf("\n");
	}

	g_assert_cmpint(NYX_ERROR_NONE, ==, nyx_security2_crypt_rsa(f->device, keydata,
	                serializedLen, NYX_SECURITY_RSA_DECRYPT,
	                enc, enclen, dec, &declen));

	g_assert_cmpint(declen, >, 0);
	printf("decoded = %s\n", dec);
	g_assert_cmpstr(src, ==, (char *) dec);

	//sign verify

	printf("sign\n");
	memset(enc, 0, keylen / 8);
	memset(dec, 0, keylen / 8);
	g_assert_cmpint(NYX_ERROR_NONE, ==, nyx_security2_crypt_rsa(f->device,
	                (const unsigned char *)keydata, serializedLen, NYX_SECURITY_RSA_SIGN,
	                (const unsigned char *)src, strlen(src) + 1, (unsigned char *) enc, &enclen));
	g_assert_cmpint(enclen, >, 0);
	printf("verify\n");
	g_assert_cmpint(NYX_ERROR_NONE, ==, nyx_security2_crypt_rsa(f->device,
	                (const unsigned char *)keydata, serializedLen, NYX_SECURITY_RSA_VERIFY,
	                (const unsigned char *)src, strlen(src) + 1, (unsigned char *) enc, &enclen));

	g_assert_cmpint(NYX_ERROR_NONE, !=, nyx_security2_crypt_rsa(f->device,
	                (const unsigned char *)keydata, serializedLen, NYX_SECURITY_RSA_VERIFY,
	                (const unsigned char *)src, strlen(src) + 1, (unsigned char *) dec, &declen));
}


#define TEST_ADD(path, func, data) \
    g_test_add(path, struct Fixture, data, fixture_setup, func, fixture_teardown)

int main(int argc, char *argv[])
{

	g_test_init(&argc, &argv, NULL);

	TEST_ADD("/nyx/security2/crypt_3des_cbc_192", test_create_3des_key,
	         "");

	TEST_ADD("/nyx/security2/create_crypt_key_aes_cbc", test_create_aes_key,
	         "");

	TEST_ADD("/nyx/security2/cerate_crypt_hmac_256", test_create_hmac_key,
	         "");

	TEST_ADD("/nyx/security2/create_key_rsa1024", test_create_rsa_key,
	         "1024");

	TEST_ADD("/nyx/security2/create_key_rsa2048", test_create_rsa_key,
	         "2048");

	TEST_ADD("/nyx/security2/create_key_rsa4096", test_create_rsa_key,
	         "4096");

	return g_test_run();
}
