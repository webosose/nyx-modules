// Copyright (c) 2013-2018 LG Electronics, Inc.
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
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>

nyx_error_t hmac_generate_key(int keybits, unsigned char *keydata)
{
	if ((keybits / 8) > EVP_MAX_KEY_LENGTH)
	{
		nyx_debug("%s: HMAC max key size exceeded", __FUNCTION__);
		return NYX_ERROR_INVALID_VALUE;
	}

	if (!RAND_bytes(keydata, keybits / 8))
	{
		g_free(keydata);
		return NYX_ERROR_GENERIC;
	}

	return NYX_ERROR_NONE;
}

nyx_error_t hmac(const unsigned char *keydata, int keybits,
                 const unsigned char *src,
                 int srclen, unsigned char *dest,
                 int *destlen)
{
	HMAC_CTX hmacctx;
	nyx_error_t result = NYX_ERROR_NONE;

	const EVP_MD *type = EVP_sha1();
	HMAC_CTX_init(&hmacctx);

	if (!HMAC_Init_ex(&hmacctx, (const void *)keydata,
	                  keybits / 8, type, NULL))
	{
		nyx_debug("HMAC_Init failed");
		result = NYX_ERROR_GENERIC;
		goto out;
	}

	if (!HMAC_Update(&hmacctx, src, srclen))
	{
		nyx_debug("HMAC_Update failed");
		result = NYX_ERROR_GENERIC;
		goto out;
	}

	if (!HMAC_Final(&hmacctx, dest,
	                (unsigned int *)destlen))
	{
		nyx_debug("HMAC_Final failed");
		result = NYX_ERROR_GENERIC;
		goto out;
	}

out:
	HMAC_CTX_cleanup(&hmacctx);
	return result;
}