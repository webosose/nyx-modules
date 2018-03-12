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

#include "security2.h"
#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <string.h>
#include <stdio.h>

static int estimatePrivateKeyLen(int keybits)
{
	int keybytes = keybits / 8;

	int result = sizeof(int) +        //struct pointer
	             3 +                  //version
	             keybytes + 32 +      //modulus
	             keybytes + 6 +       //private exponent
	             (keybytes / 2) + 4 + //prime1
	             (keybytes / 2) + 4 + //prime2
	             (keybytes / 2) + 4 + //exponent1
	             (keybytes / 2) + 4 + //exponent2
	             (keybytes / 2) + 4 + //coefficient
	             64;                  //optional data (other prime infos)
	return result;
}

nyx_error_t rsa_generate_key(int keybits, unsigned char *private, int *privLen,
                             unsigned char *publicKey, int *pubKeySize)
{
	switch (keybits)
	{
		case 1024:
		case 2048:
		case 4096:
			break;

		default:
			return NYX_ERROR_INVALID_VALUE;
	}

	if (private == NULL || publicKey == NULL)
	{
		*privLen = estimatePrivateKeyLen(keybits);
		*pubKeySize = keybits / 8 + 32;
		return NYX_ERROR_NONE;
	}

	RSA *rsa = RSA_new();

	if (!rsa)
	{
		nyx_debug("RSA_new failed");
		ERR_print_errors_fp(stderr);
		return NYX_ERROR_GENERIC;
	}

	BIGNUM *bn = BN_new();

	if (!bn)
	{
		nyx_debug("BN_new failed");
		ERR_print_errors_fp(stderr);
		RSA_free(rsa);
		return NYX_ERROR_GENERIC;
	}

	BN_set_word(bn, RSA_F4);

	if (RSA_generate_key_ex(rsa, keybits, bn, NULL) == -1)
	{
		nyx_debug("RSA_generate_key_ex failed");
		ERR_print_errors_fp(stderr);
		RSA_free(rsa);
		BN_free(bn);
		return NYX_ERROR_GENERIC;
	}

	BN_free(bn);

	//private
	BIO *privKeyBio = NULL;

	if ((privKeyBio = BIO_new(BIO_s_mem())) == NULL)
	{
		nyx_debug("BIO_new failed");
		ERR_print_errors_fp(stderr);
		RSA_free(rsa);
		return NYX_ERROR_GENERIC;
	}

	if (!i2d_RSAPrivateKey_bio(privKeyBio, rsa))
	{
		nyx_debug("i2d_RSAPrivateKey_bio failed");
		ERR_print_errors_fp(stderr);
		RSA_free(rsa);
		BIO_free_all(privKeyBio);
		return NYX_ERROR_GENERIC;
	}

	unsigned char *privKeyBuf = NULL;
	int privKeyLen = BIO_get_mem_data(privKeyBio, &privKeyBuf);

	if (privKeyBuf == NULL || privKeyLen <= 0)
	{
		nyx_debug("BIO_get_mem_data failed");
		ERR_print_errors_fp(stderr);
		BIO_free_all(privKeyBio);
		RSA_free(rsa);
		return NYX_ERROR_GENERIC;
	}

	//public
	BIO *pubKeyBio = NULL;

	if ((pubKeyBio = BIO_new(BIO_s_mem())) == NULL)
	{
		nyx_debug("BIO_new failed");
		ERR_print_errors_fp(stderr);
		RSA_free(rsa);
		BIO_free_all(privKeyBio);
		return NYX_ERROR_GENERIC;
	}

	if (!i2d_RSAPublicKey_bio(pubKeyBio, rsa))
	{
		nyx_debug("i2d_RSAPublicKey_bio failed");
		ERR_print_errors_fp(stderr);
		RSA_free(rsa);
		BIO_free_all(pubKeyBio);
		BIO_free_all(privKeyBio);
		return NYX_ERROR_GENERIC;
	}

	unsigned char *pubKeyBuf = NULL;
	int pubSize = BIO_get_mem_data(pubKeyBio, &pubKeyBuf);

	if (pubKeyBuf == NULL || pubSize <= 0)
	{
		nyx_debug("BIO_get_mem_data failed");
		ERR_print_errors_fp(stderr);
		BIO_free_all(pubKeyBio);
		BIO_free_all(privKeyBio);
		RSA_free(rsa);
		return NYX_ERROR_GENERIC;
	}

	*privLen = privKeyLen;
	*pubKeySize = pubSize;
	memcpy(private, privKeyBuf, *privLen);
	memcpy(publicKey, pubKeyBuf, *pubKeySize);

	BIO_free_all(pubKeyBio);
	BIO_free_all(privKeyBio);
	RSA_free(rsa);

	return NYX_ERROR_NONE;
}

nyx_error_t rsa_crypt(const unsigned char *keydata, int serializedKeyDataLen,
                      nyx_security_rsa_operation_t operation, const unsigned char *src, int srclen,
                      unsigned char *dest, int *destlen)
{
	switch (operation)
	{
		case NYX_SECURITY_RSA_DECRYPT:
		case NYX_SECURITY_RSA_ENCRYPT:
		{
			nyx_error_t result = NYX_ERROR_NONE;
			RSA *rsa = NULL;
			int isPublic = 0;
			d2i_RSAPrivateKey(&rsa, &keydata,
			                  serializedKeyDataLen);

			if (!rsa)
			{
				nyx_debug("private RSA d2i failed\n");
				d2i_RSAPublicKey(&rsa, &keydata,
				                 serializedKeyDataLen);
				isPublic = 1;

				if (!rsa)
				{
					printf("public RSA d2i failed\n");
					return NYX_ERROR_GENERIC;
				}

				nyx_debug("public RSA d2i succeeded\n");
			}

			if (operation == NYX_SECURITY_RSA_ENCRYPT)
			{
				// RSA_PKCS1_OAEP_PADDING limit
				if (srclen >= (RSA_size(rsa) - 41))
				{
					nyx_debug("src buffer too big");
					RSA_free(rsa);
					return NYX_ERROR_INVALID_VALUE;
				}
			}

			*destlen = RSA_size(rsa);

			int (*crypt_fun)(int, const unsigned char *, unsigned char *, RSA *, int);

			if (operation == NYX_SECURITY_RSA_ENCRYPT)
			{
				if (isPublic)
				{
					crypt_fun = RSA_public_encrypt;
				}
				else
				{
					crypt_fun = RSA_private_encrypt;
				}
			}
			else
			{
				if (isPublic)
				{
					crypt_fun = RSA_public_decrypt;
				}
				else
				{
					crypt_fun = RSA_private_decrypt;
				}
			}

			*destlen = crypt_fun(srclen, (unsigned char *)src, dest,
			                     rsa, RSA_PKCS1_OAEP_PADDING);

			if (*destlen == -1)
			{
				nyx_debug("RSA crypt failed");
				ERR_print_errors_fp(stderr);
				result = NYX_ERROR_GENERIC;
			}

			RSA_free(rsa);
			return result;
		} //end case DECRYPT,ENCRYPT

		case NYX_SECURITY_RSA_SIGN:
		case NYX_SECURITY_RSA_VERIFY:
		{
			RSA *rsa = NULL;
			SHA256_CTX sha_ctx;
			unsigned char md[SHA256_DIGEST_LENGTH];
			nyx_error_t result = NYX_ERROR_NONE;
			d2i_RSAPrivateKey(&rsa, &keydata, serializedKeyDataLen);

			if (!rsa)
			{
				d2i_RSAPublicKey(&rsa, &keydata, serializedKeyDataLen);

				if (!rsa)
				{
					return NYX_ERROR_GENERIC;
				}
			}

			if (!SHA256_Init(&sha_ctx) ||
			        !SHA256_Update(&sha_ctx, src, srclen) ||
			        !SHA256_Final(md, &sha_ctx))
			{
				RSA_free(rsa);
				return NYX_ERROR_GENERIC;
			}

			if (operation == NYX_SECURITY_RSA_SIGN)
			{
				if (RSA_sign(
				            NID_sha256,
				            md,
				            SHA256_DIGEST_LENGTH,
				            dest, (unsigned int *) destlen, rsa) != 1)
				{
					result = NYX_ERROR_GENERIC;
				}
			}
			else if (operation == NYX_SECURITY_RSA_VERIFY)
			{
				if (RSA_verify(
				            NID_sha256,
				            md, SHA256_DIGEST_LENGTH,
				            dest, *destlen, rsa) != 1)
				{
					result = NYX_ERROR_GENERIC;
				}
			}

			RSA_free(rsa);
			return result;

		} //end case SIGN, VERIFY

		default:
			nyx_debug("incorrect function mode");
			g_assert(false);
	} //end switch

	return NYX_ERROR_INVALID_VALUE;
}
