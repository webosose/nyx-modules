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
#ifndef _SECURITY2_H_
#define _SECURITY2_H_

#include <nyx/nyx_module.h>
#include <glib.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <nyx/nyx_client.h>

nyx_error_t aes_generate_key(int keybits, unsigned char *keydata);
nyx_error_t aes_crypt(const unsigned char *keydata, int keybits, int encrypt,
                      nyx_security_block_mode_t mode, const unsigned char *src, int srclen,
                      unsigned char *dest,
                      int *destlen, const unsigned char *iv, int ivlen,
                      nyx_security_padding_t padding,
                      unsigned char *nextIv, int nextIvLen);

nyx_error_t aes_crypt_simple(const unsigned char *keydata, int keybits,
                             int encrypt,
                             nyx_security_block_mode_t mode, const unsigned char *src, int srclen,
                             unsigned char *dest, int *destlen);


nyx_error_t  des3_generate_key(int keybits, unsigned char *keydata);
nyx_error_t des3_crypt(const unsigned char *keydata, int encrypt,
                       nyx_security_block_mode_t mode, const unsigned char *src, int srclen,
                       unsigned char *dest,
                       int *destlen, const unsigned char *iv, int ivlen,
                       nyx_security_padding_t padding,
                       unsigned char *nextIv, int nextIvLen);

nyx_error_t des3_crypt_simple(const unsigned char *keydata, int encrypt,
                              nyx_security_block_mode_t mode, const unsigned char *src, int srclen,
                              unsigned char *dest, int *destlen);

nyx_error_t hmac_generate_key(int keybits, unsigned char *keydata);
nyx_error_t hmac(const unsigned char *keydata, int keybits,
                 const unsigned char *src,
                 int srclen, unsigned char *dest,
                 int *destlen);

nyx_error_t rsa_generate_key(int keybits, unsigned char *keydata,
                             int *serializedKeyDataLen,
                             unsigned char *publicKey, int *pubKeySize);
nyx_error_t rsa_crypt(const unsigned char *keydata, int serializedKeyDataLen,
                      nyx_security_rsa_operation_t operation, const unsigned char *src, int srclen,
                      unsigned char *dest, int *destlen);

#endif
