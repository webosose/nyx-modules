// Copyright (c) 2013-2019 LG Electronics, Inc.
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
#include <openssl/des.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <string.h>
#define DES3_BLOCK_SIZE 8

nyx_error_t des3_generate_key(int keybits, unsigned char *keydata)
{
	if (keybits != 192)
	{
		return NYX_ERROR_INVALID_VALUE;
	}

	for (int i = 0; i < 3; i++)
	{
		if (!DES_random_key((DES_cblock *)(keydata + 8 * i)))
		{
			return NYX_ERROR_GENERIC;
		}
	}

	return NYX_ERROR_NONE;
}

nyx_error_t des3_crypt(const unsigned char *keydata, int encrypt,
                       nyx_security_block_mode_t mode, const unsigned char *src, int srclen,
                       unsigned char *dest,
                       int *destlen, const unsigned char *iv, int ivlen,
                       nyx_security_padding_t padding,
                       unsigned char *nextIv, int nextIvLen)
{
	if (DES3_BLOCK_SIZE != ivlen)
	{
		return NYX_ERROR_INVALID_VALUE;
	}

	if (padding == NYX_SECURITY_PADDING_NONE)
	{
		if (DES3_BLOCK_SIZE != nextIvLen)
		{
			return NYX_ERROR_INVALID_VALUE;
		}

		if (!RAND_bytes(nextIv, DES3_BLOCK_SIZE))
		{
			return NYX_ERROR_GENERIC;
		}

	}
	else if (padding != NYX_SECURITY_PADDING_PKCS5)
	{
		return NYX_ERROR_INVALID_VALUE;
	}
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	EVP_CIPHER_CTX ctx;
#else
	EVP_CIPHER_CTX *ctx;
        ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            nyx_debug("Out of memory: EVP_CIPHER_CTX");
            return NYX_ERROR_OUT_OF_MEMORY;
        }
#endif
	const EVP_CIPHER *cipher;

	nyx_error_t result = NYX_ERROR_NONE;

	switch (mode)
	{
		case NYX_SECURITY_MODE_ECB:
			cipher = EVP_des_ede3_ecb();
			break;

		case NYX_SECURITY_MODE_CBC:
			cipher = EVP_des_ede3_cbc();
			break;

		case NYX_SECURITY_MODE_CFB:
			cipher = EVP_des_ede3_cfb();
			break;

		default:
			nyx_debug("%s: invalid DES mode", __FUNCTION__);
			return NYX_ERROR_INVALID_VALUE;
	}

	int updateoutlen, finaloutlen;
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	EVP_CIPHER_CTX_init(&ctx);
#else
	EVP_CIPHER_CTX_init(ctx);
#endif
	updateoutlen = *destlen;

	if (encrypt)
	{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
		EVP_EncryptInit(&ctx, cipher, keydata, iv);

                if (!EVP_EncryptUpdate(&ctx, dest,
                                       &updateoutlen, (unsigned char *)src,
                                       srclen))
#else
		EVP_EncryptInit(ctx, cipher, keydata, iv);

		if (!EVP_EncryptUpdate(ctx, dest,
		                       &updateoutlen, (unsigned char *)src,
		                       srclen))
#endif
		{
			nyx_debug("EVP_CipherUpdate failed");
			ERR_print_errors_fp(stderr);
			result = NYX_ERROR_GENERIC;
			goto out;
		}

		finaloutlen = *destlen - updateoutlen;
#if OPENSSL_VERSION_NUMBER < 0x10100000L
		if (!EVP_EncryptFinal(&ctx, dest + updateoutlen, &finaloutlen))
#else
		if (!EVP_EncryptFinal(ctx, dest + updateoutlen, &finaloutlen))
#endif
		{
			nyx_debug("EVP_CipherFinal failed");
			ERR_print_errors_fp(stderr);
			result = NYX_ERROR_GENERIC;
			goto out;
		}
	}
	else
	{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
		EVP_DecryptInit(&ctx, cipher, keydata, iv);

                if (!EVP_DecryptUpdate(&ctx, dest,
                                       &updateoutlen, (unsigned char *)src,
                                       srclen))
#else
		EVP_DecryptInit(ctx, cipher, keydata, iv);

		if (!EVP_DecryptUpdate(ctx, dest,
		                       &updateoutlen, (unsigned char *)src,
		                       srclen))
#endif
		{
			nyx_debug("EVP_CipherUpdate failed");
			ERR_print_errors_fp(stderr);
			result = NYX_ERROR_GENERIC;
			goto out;
		}

		finaloutlen = *destlen - updateoutlen;

		if (nextIvLen == DES3_BLOCK_SIZE && padding == NYX_SECURITY_PADDING_NONE &&
		        mode != NYX_SECURITY_MODE_ECB)
		{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
			memcpy(nextIv, &ctx.iv, DES3_BLOCK_SIZE);
#else
			EVP_DecryptInit_ex(ctx, cipher, NULL, keydata, nextIv);
#endif
                }
#if OPENSSL_VERSION_NUMBER < 0x10100000L
                if (!EVP_DecryptFinal(&ctx, dest + updateoutlen,
                                      (int *) &finaloutlen))
#else
		if (!EVP_DecryptFinal(ctx, dest + updateoutlen,
                                      (int *) &finaloutlen))
#endif
		{
			nyx_debug("EVP_CipherFinal failed");
			ERR_print_errors_fp(stderr);
			result = NYX_ERROR_GENERIC;
			goto out;
		}
	}

	*destlen = updateoutlen + finaloutlen;
out:
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	EVP_CIPHER_CTX_cleanup(&ctx);
#else
	EVP_CIPHER_CTX_reset(ctx);
        EVP_CIPHER_CTX_free(ctx);
#endif
	return result;
}

nyx_error_t des3_crypt_simple(const unsigned char *keydata, int encrypt,
                              nyx_security_block_mode_t mode, const unsigned char *src, int srclen,
                              unsigned char *dest,
                              int *destlen)
{
	unsigned char iv[DES3_BLOCK_SIZE];

	if (encrypt)
	{
		/* generate IV */
		if (!RAND_bytes(iv, DES3_BLOCK_SIZE))
		{
			return NYX_ERROR_GENERIC;
		}

		/* skip IV */
		memcpy(dest, iv, DES3_BLOCK_SIZE);
		dest += DES3_BLOCK_SIZE;
	}
	else
	{
		/* skip IV */
		memcpy(iv, src, DES3_BLOCK_SIZE);
		src += DES3_BLOCK_SIZE;
		srclen -= DES3_BLOCK_SIZE;
	}

	nyx_error_t result = des3_crypt(keydata, encrypt, mode, src, srclen, dest,
	                                destlen, iv, DES3_BLOCK_SIZE, NYX_SECURITY_PADDING_PKCS5, NULL, 0);

	if (NYX_ERROR_NONE != result)
	{
		return result;
	}

	if (encrypt)
	{
		*destlen += DES3_BLOCK_SIZE;
	}

	return NYX_ERROR_NONE;
}
