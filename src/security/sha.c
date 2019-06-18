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

#include <security.h>
#include <string.h>

static const struct sha_algo_data_t
{
	const char *name;
	const EVP_MD *(*md)();
} sha_algo_data[] =
{
	{ SN_sha256, EVP_sha256 },
	{ SN_sha512, EVP_sha512 },
};

static const struct sha_algo_data_t *sha_algo_data_lookup(const char *name)
{
	int i;

	for (i = 0; i < sizeof(sha_algo_data) / sizeof(sha_algo_data[0]); ++i)
	{
		if (strcmp(sha_algo_data[i].name, name) == 0)
		{
			return &sha_algo_data[i];
		}
	}

	return NULL;
}

/* running hashing context (only single concurrent hash operation supported) */
static EVP_MD_CTX *running_mdctx = NULL;

nyx_error_t sha_init(const char *name)
{
	const struct sha_algo_data_t *algo = sha_algo_data_lookup(name);

	if (algo == NULL)
	{
		return NYX_ERROR_INVALID_VALUE;
	}

	if (running_mdctx != NULL)
	{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
		EVP_MD_CTX_cleanup(running_mdctx);
                EVP_MD_CTX_destroy(running_mdctx);
#else
		EVP_MD_CTX_reset(running_mdctx);
		EVP_MD_CTX_free(running_mdctx);
#endif
	}
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	running_mdctx = EVP_MD_CTX_create();
        EVP_MD_CTX_init(running_mdctx);
#else
	running_mdctx = EVP_MD_CTX_new();
        if (!running_mdctx) {
            nyx_debug("Out of memory: EVP_MD_CTX");
            return NYX_ERROR_OUT_OF_MEMORY;
        }
#endif
	if (EVP_DigestInit_ex(running_mdctx, algo->md(), NULL) != 1)
	{
		return NYX_ERROR_GENERIC;
	}

	return NYX_ERROR_NONE;
}

nyx_error_t sha_update(const char *src, int srclen)
{
	if (running_mdctx == NULL)
	{
		return NYX_ERROR_INVALID_VALUE;
	}

	if (EVP_DigestUpdate(running_mdctx, (const void *)src, (size_t)srclen) != 1)
	{
		return NYX_ERROR_GENERIC;
	}

	return NYX_ERROR_NONE;
}

nyx_error_t sha_finalize(char *dest, int *destlen)
{
	if (running_mdctx == NULL)
	{
		return NYX_ERROR_INVALID_VALUE;
	}

	if (EVP_DigestFinal_ex(running_mdctx, (unsigned char *)dest,
	                       (unsigned int *)destlen) != 1)
	{
		return NYX_ERROR_GENERIC;
	}
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	EVP_MD_CTX_cleanup(running_mdctx);
        EVP_MD_CTX_destroy(running_mdctx);
#else
        EVP_MD_CTX_reset(running_mdctx);
        EVP_MD_CTX_free(running_mdctx);
#endif
	running_mdctx = NULL;

	return NYX_ERROR_NONE;
}
