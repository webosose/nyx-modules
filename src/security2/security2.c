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

/*
********************************************************************************
* @file security2.c
*
* @brief The SECURITY2 module implementation.
********************************************************************************
*/

#include "security2.h"
#include <stdlib.h>
#include <string.h>
#include <openssl/err.h>
#include <openssl/conf.h>
#include <sys/types.h>
#include <unistd.h>

/*
 * Callbacks for multithreaded OpenSSL
 */
static pthread_mutex_t *lock_crypto = NULL;
static int is_initializing = 0;
static int reference_count;

static int sync_interlocked_exchange(int *shared, int exchange)
{
	int u;

	do
	{
		u = *shared;
	}
	while (__sync_val_compare_and_swap(shared, u, exchange) != u);

	return u;
}

static unsigned long pthreads_thread_id_callback(void)
{
	return (unsigned long)pthread_self();
}

static void pthreads_crypto_locking_callback(
    int mode, int type, const char *file, int line)
{
	if (mode & CRYPTO_LOCK)
	{
		pthread_mutex_lock(&(lock_crypto[type]));
	}

	if (mode & CRYPTO_UNLOCK)
	{
		pthread_mutex_unlock(&(lock_crypto[type]));
	}
}

NYX_DECLARE_MODULE(NYX_DEVICE_SECURITY2, "Security2");

nyx_error_t nyx_module_open(nyx_instance_t i, nyx_device_t **d)
{
	nyx_error_t ret = NYX_ERROR_NONE;

	if (NULL == d)
	{
		return NYX_ERROR_INVALID_VALUE;
	}

	*d = (nyx_device_t *) calloc(sizeof(nyx_device_t), 1);

	if (NULL == *d)
	{
		return NYX_ERROR_OUT_OF_MEMORY;
	}

	/* register methods */
	struct
	{
		module_method_t method;
		const char *function;
	} methods[] =
	{
		{ NYX_SECURITY2_CREATE_AES_KEY_MODULE_METHOD, "security2_create_aes_key" },
		{ NYX_SECURITY2_CREATE_RSA_KEY_MODULE_METHOD, "security2_create_rsa_key" },
		{ NYX_SECURITY2_CRYPT_AES_MODULE_METHOD,      "security2_aes_crypt" },
		{ NYX_SECURITY2_CRYPT_AES_SIMPLE_MODULE_METHOD,      "security2_aes_crypt_simple" },
		{ NYX_SECURITY2_CRYPT_RSA_MODULE_METHOD,      "security2_rsa_crypt" },
		{ NYX_SECURITY2_CREATE_3DES_KEY_MODULE_METHOD, "security2_create_des3_key" },
		{ NYX_SECURITY2_CREATE_HMAC_KEY_MODULE_METHOD, "security2_create_hmac_key" },
		{ NYX_SECURITY2_CRYPT_3DES_MODULE_METHOD,      "security2_des3_crypt" },
		{ NYX_SECURITY2_CRYPT_3DES_SIMPLE_MODULE_METHOD,      "security2_des3_crypt_simple" },
		{ NYX_SECURITY2_HMAC_MODULE_METHOD,      "security2_hmac" },
	};

	int m;

	for (m = 0; m < sizeof(methods) / sizeof(methods[0]); ++m)
	{
		nyx_error_t result = nyx_module_register_method(i, *d, methods[m].method,
		                     methods[m].function);

		if (result != NYX_ERROR_NONE)
		{
			free(*d);
			*d = NULL;
			return result;
		}
	}

	while (sync_interlocked_exchange(&is_initializing, 1) != 0)
	{
		usleep(1);
	}

	reference_count++;

	if (reference_count > 1)
	{
		goto exit;
	}

	lock_crypto = OPENSSL_malloc(CRYPTO_num_locks() * sizeof(pthread_mutex_t));

	if (lock_crypto == NULL)
	{
		free(*d);
		*d = NULL;
		ret =  NYX_ERROR_GENERIC;
		goto exit;
	}

	for (int i = 0; i < CRYPTO_num_locks(); i++)
	{
		pthread_mutex_init(&(lock_crypto[i]), NULL);
	}

	CRYPTO_set_locking_callback(pthreads_crypto_locking_callback);

	OPENSSL_init();
	OpenSSL_add_all_algorithms();
	ERR_load_BIO_strings();
	ERR_load_crypto_strings();
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	OPENSSL_config(NULL);
#endif
	CRYPTO_set_id_callback(pthreads_thread_id_callback);


exit:
	sync_interlocked_exchange(&is_initializing, 0);
	return ret;
}

nyx_error_t nyx_module_close(nyx_device_handle_t d)
{
	nyx_error_t ret = NYX_ERROR_NONE;

	while (sync_interlocked_exchange(&is_initializing, 1) != 0)
	{
		usleep(1);
	}

	if (reference_count == 0)
	{
		goto exit;
	}

	reference_count--;

	if (reference_count > 0)
	{
		goto exit;
	}

	CRYPTO_set_locking_callback(NULL);

	for (int i = 0; i < CRYPTO_num_locks(); i++)
	{
		pthread_mutex_destroy(&(lock_crypto[i]));
	}

	OPENSSL_free(lock_crypto);
	lock_crypto = NULL;

	EVP_cleanup();
	CRYPTO_cleanup_all_ex_data();
	ERR_free_strings();

exit:
	free(d);
	sync_interlocked_exchange(&is_initializing, 0);
	return ret;
}

nyx_error_t security2_create_aes_key(nyx_device_handle_t d, int keybits,
                                     unsigned char *keydata)
{
	if (NULL == d || NULL == keydata)
	{
		return NYX_ERROR_INVALID_VALUE;
	}

	return aes_generate_key(keybits, keydata);
}

nyx_error_t security2_aes_crypt(nyx_device_handle_t d,
                                const unsigned char *keydata, int keybits,
                                nyx_security_block_mode_t mode, int encrypt, const unsigned char *src,
                                int srclen,
                                unsigned char *dest, int *destlen, const unsigned char *iv, int ivlen,
                                nyx_security_padding_t padding,
                                unsigned char *nextIv, int nextIvLen)
{
	if (NULL == d || NULL == keydata || NULL == src || NULL == dest ||
	        NULL == destlen || iv == NULL)
	{
		return NYX_ERROR_INVALID_VALUE;
	}

	return aes_crypt(keydata, keybits, encrypt, mode, src, srclen, dest, destlen,
	                 iv, ivlen, padding, nextIv, nextIvLen);
}

nyx_error_t security2_aes_crypt_simple(nyx_device_handle_t d,
                                       const unsigned char *keydata, int keybits,
                                       nyx_security_block_mode_t mode, int encrypt, const unsigned char *src,
                                       int srclen,
                                       unsigned char *dest, int *destlen)
{
	if (NULL == d || NULL == keydata || NULL == src || NULL == dest ||
	        NULL == destlen)
	{
		return NYX_ERROR_INVALID_VALUE;
	}

	return aes_crypt_simple(keydata, keybits, encrypt, mode, src, srclen, dest,
	                        destlen);
}


nyx_error_t security2_create_des3_key(nyx_device_handle_t d, int keybits,
                                      unsigned char *keydata)
{
	if (NULL == d || NULL == keydata)
	{
		return NYX_ERROR_INVALID_VALUE;
	}

	return des3_generate_key(keybits, keydata);
}

nyx_error_t security2_des3_crypt(nyx_device_handle_t d,
                                 const unsigned char *keydata,
                                 nyx_security_block_mode_t mode, int encrypt, const unsigned char *src,
                                 int srclen,
                                 unsigned char *dest, int *destlen, const unsigned char *iv, int ivlen,
                                 nyx_security_padding_t padding, unsigned char *nextIv, int nextIvLen)
{
	if (NULL == d || NULL == keydata || NULL == src || NULL == dest ||
	        NULL == destlen)
	{
		return NYX_ERROR_INVALID_VALUE;
	}

	return des3_crypt(keydata, encrypt, mode, src, srclen, dest, destlen, iv, ivlen,
	                  padding, nextIv, nextIvLen);
}

nyx_error_t security2_des3_crypt_simple(nyx_device_handle_t d,
                                        const unsigned char *keydata,
                                        nyx_security_block_mode_t mode, int encrypt, const unsigned char *src,
                                        int srclen,
                                        unsigned char *dest, int *destlen)
{
	if (NULL == d || NULL == keydata || NULL == src || NULL == dest ||
	        NULL == destlen)
	{
		return NYX_ERROR_INVALID_VALUE;
	}

	return des3_crypt_simple(keydata, encrypt, mode, src, srclen, dest, destlen);
}

nyx_error_t security2_create_hmac_key(nyx_device_handle_t d, int keybits,
                                      unsigned char *keydata)
{
	if (NULL == d || NULL == keydata)
	{
		return NYX_ERROR_INVALID_VALUE;
	}

	return hmac_generate_key(keybits, keydata);
}

nyx_error_t security2_hmac(nyx_device_handle_t d, const unsigned char *keydata,
                           int keybits,
                           const unsigned char *src, int srclen,
                           unsigned char *dest, int *destlen)
{
	if (NULL == d || NULL == keydata || NULL == src || NULL == dest ||
	        NULL == destlen)
	{
		return NYX_ERROR_INVALID_VALUE;
	}

	return hmac(keydata, keybits, src, srclen, dest, destlen);
}

nyx_error_t security2_create_rsa_key(nyx_device_handle_t d, int keybits,
                                     unsigned char *keydata, int *serializedKeyDataLen,
                                     unsigned char *publicKey, int *pubKeySize)
{
	if (NULL == d || NULL == serializedKeyDataLen || NULL == pubKeySize)
	{
		return NYX_ERROR_INVALID_VALUE;
	}

	return rsa_generate_key(keybits, keydata, serializedKeyDataLen, publicKey,
	                        pubKeySize);
}

nyx_error_t security2_rsa_crypt(nyx_device_handle_t d,
                                const unsigned char *keydata, int serializedKeyDataLen,
                                nyx_security_rsa_operation_t operation, const unsigned char *src, int srclen,
                                unsigned char *dest, int *destlen)
{
	if (NULL == d || NULL == keydata || NULL == src || NULL == dest ||
	        NULL == destlen)
	{
		return NYX_ERROR_INVALID_VALUE;
	}

	return rsa_crypt(keydata, serializedKeyDataLen, operation, src, srclen, dest,
	                 destlen);
}
