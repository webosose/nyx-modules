// Copyright (c) 2016-2019 LG Electronics, Inc.
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

#include "security2.h"
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/obj_mac.h>
#include <string.h>

typedef struct aes_algo_data_t
{
	const char *name;
	int keylen;
	nyx_security_block_mode_t mode;
	const EVP_CIPHER *(*cipher_fun)(void);
} aes_algo_data_s;

static const aes_algo_data_s aes_algo_data[] =
{
	{ SN_aes_128_cbc,    128, NYX_SECURITY_MODE_CBC, EVP_aes_128_cbc },
	{ SN_aes_256_cbc,    256, NYX_SECURITY_MODE_CBC, EVP_aes_256_cbc },
	{ SN_aes_192_cbc,    192, NYX_SECURITY_MODE_CBC, EVP_aes_192_cbc },
	{ SN_aes_128_ecb,    128, NYX_SECURITY_MODE_ECB, EVP_aes_128_ecb },
	{ SN_aes_256_ecb,    256, NYX_SECURITY_MODE_ECB, EVP_aes_128_ecb },
	{ SN_aes_192_ecb,    192, NYX_SECURITY_MODE_ECB, EVP_aes_128_ecb },
	{ SN_aes_128_cfb128, 128, NYX_SECURITY_MODE_CFB, EVP_aes_128_cfb },
	{ SN_aes_256_cfb128, 256, NYX_SECURITY_MODE_CFB, EVP_aes_128_cfb },
	{ SN_aes_192_cfb128, 192, NYX_SECURITY_MODE_CFB, EVP_aes_128_cfb }
};

static int aes_supported_keylength(int keylen)
{
	for (size_t i = 0; i < sizeof(aes_algo_data) / sizeof(aes_algo_data[0]); ++i)
	{
		if (aes_algo_data[i].keylen == keylen)
		{
			return 1;
		}
	}

	return 0;
}

static const struct aes_algo_data_t *aes_algo_data_lookup(int keylen,
        nyx_security_block_mode_t mode)
{
	for (size_t i = 0; i < sizeof(aes_algo_data) / sizeof(aes_algo_data[0]); ++i)
	{
		if (aes_algo_data[i].keylen == keylen && aes_algo_data[i].mode == mode)
		{
			return &aes_algo_data[i];
		}
	}

	return NULL;
}

nyx_error_t aes_generate_key(int keybits, unsigned char *keydata)
{

	if (!aes_supported_keylength(keybits))
	{
		return NYX_ERROR_INVALID_VALUE;
	}

	if (!RAND_bytes(keydata, keybits / 8))
	{
		g_free(keydata);
		return NYX_ERROR_GENERIC;
	}

	return NYX_ERROR_NONE;
}

nyx_error_t aes_crypt(const unsigned char *keydata, int keybits, int encrypt,
                      nyx_security_block_mode_t mode, const unsigned char *src, int srclen,
                      unsigned char *dest,
                      int *destlen, const unsigned char *iv, int ivlen,
                      nyx_security_padding_t padding,
                      unsigned char *nextIv, int nextIvLen)
{
	if (AES_BLOCK_SIZE != ivlen)
	{
		return NYX_ERROR_INVALID_VALUE;
	}

	if (padding == NYX_SECURITY_PADDING_NONE)
	{
		if (AES_BLOCK_SIZE != nextIvLen)
		{
			return NYX_ERROR_INVALID_VALUE;
		}
	}
	else if (padding != NYX_SECURITY_PADDING_PKCS5)
	{
		return NYX_ERROR_INVALID_VALUE;
	}

	nyx_error_t result = NYX_ERROR_NONE;
	const struct aes_algo_data_t *algo = aes_algo_data_lookup(keybits, mode);

	if (algo == NULL)
	{
		return NYX_ERROR_INVALID_VALUE;
	}
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	EVP_CIPHER_CTX ctx;
        EVP_CIPHER_CTX_init(&ctx);
        EVP_CipherInit_ex(&ctx, algo->cipher_fun(), NULL, NULL, NULL, encrypt);
        EVP_CIPHER_CTX_set_key_length(&ctx, keybits);
        EVP_CipherInit_ex(&ctx, NULL, NULL, keydata, iv, encrypt);
#else
	EVP_CIPHER_CTX *ctx;
        ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            nyx_debug("Out of memory: EVP_CIPHER_CTX");
            return NYX_ERROR_OUT_OF_MEMORY;
        }
	EVP_CIPHER_CTX_init(ctx);
	EVP_CipherInit_ex(ctx, algo->cipher_fun(), NULL, NULL, NULL, encrypt);
	EVP_CIPHER_CTX_set_key_length(ctx, keybits);
	EVP_CipherInit_ex(ctx, NULL, NULL, keydata, iv, encrypt);
#endif
	if (mode != NYX_SECURITY_MODE_CFB)
	{
		int pad = padding == NYX_SECURITY_PADDING_PKCS5 ? 1 : 0;
#if OPENSSL_VERSION_NUMBER < 0x10100000L
		if (!EVP_CIPHER_CTX_set_padding(&ctx, pad))
#else
		if (!EVP_CIPHER_CTX_set_padding(ctx, pad))
#endif
		{
			result = NYX_ERROR_GENERIC;
			goto out;
		}
	}
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	if (!EVP_CipherUpdate(&ctx, dest, destlen,src, srclen))
#else
	if (!EVP_CipherUpdate(ctx, dest, destlen,src, srclen))
#endif
	{
		nyx_debug("EVP_CipherUpdate failed");
		ERR_print_errors_fp(stderr);
		result = NYX_ERROR_GENERIC;
		goto out;
	}

	if (nextIvLen == AES_BLOCK_SIZE && padding == NYX_SECURITY_PADDING_NONE &&
	        mode != NYX_SECURITY_MODE_ECB)
	{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
		memcpy(nextIv, &ctx.iv, AES_BLOCK_SIZE);
#else
		EVP_CipherInit_ex(ctx, NULL, NULL, keydata, nextIv, encrypt);
#endif
	}

	int tmplen;
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	if (!EVP_CipherFinal_ex(&ctx, dest + *destlen, &tmplen))
#else
	if (!EVP_CipherFinal_ex(ctx, dest + *destlen, &tmplen))
#endif
	{
		nyx_debug("EVP_CipherFinal_ex failed");
		ERR_print_errors_fp(stderr);
		result = NYX_ERROR_GENERIC;
		goto out;
	}

	*destlen += tmplen;

out:
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	EVP_CIPHER_CTX_cleanup(&ctx);
#else
	EVP_CIPHER_CTX_reset(ctx);
        EVP_CIPHER_CTX_free(ctx);
#endif
	return result;
}

nyx_error_t aes_crypt_simple(const unsigned char *keydata, int keybits,
                             int encrypt,
                             nyx_security_block_mode_t mode, const unsigned char *src, int srclen,
                             unsigned char *dest,
                             int *destlen)
{
	unsigned char iv[AES_BLOCK_SIZE];

	if (encrypt)
	{
		/* generate IV */
		if (!RAND_bytes(iv, AES_BLOCK_SIZE))
		{
			return NYX_ERROR_GENERIC;
		}

		/* skip IV */
		memcpy(dest, iv, AES_BLOCK_SIZE);
		dest += AES_BLOCK_SIZE;
	}
	else
	{
		/* skip IV */
		memcpy(iv, src, AES_BLOCK_SIZE);
		src += AES_BLOCK_SIZE;
		srclen -= AES_BLOCK_SIZE;
	}

	nyx_error_t result = aes_crypt(keydata, keybits, encrypt, mode, src, srclen,
	                               dest,
	                               destlen, iv, AES_BLOCK_SIZE, NYX_SECURITY_PADDING_PKCS5, NULL, 0);

	if (NYX_ERROR_NONE != result)
	{
		return NYX_ERROR_GENERIC;
	}

	if (encrypt)
	{
		*destlen += AES_BLOCK_SIZE;
	}

	return NYX_ERROR_NONE;
}
