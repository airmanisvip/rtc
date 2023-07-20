/* Copyright (c) 2014, Google Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <openssl/bytestring.h>
#include <openssl/crypto.h>
#include <openssl/digest.h>
#include <openssl/err.h>
#include <openssl/pkcs8.h>
#include <openssl/rsa.h>

#include "../internal.h"
#include "../test/test_util.h"


// kExampleRSAKeyDER is an RSA private key in ASN.1, DER format. Of course, you
// should never use this key anywhere but in an example.
static const uint8_t kExampleRSAKeyDER[] = {
    0x30, 0x82, 0x02, 0x5c, 0x02, 0x01, 0x00, 0x02, 0x81, 0x81, 0x00, 0xf8,
    0xb8, 0x6c, 0x83, 0xb4, 0xbc, 0xd9, 0xa8, 0x57, 0xc0, 0xa5, 0xb4, 0x59,
    0x76, 0x8c, 0x54, 0x1d, 0x79, 0xeb, 0x22, 0x52, 0x04, 0x7e, 0xd3, 0x37,
    0xeb, 0x41, 0xfd, 0x83, 0xf9, 0xf0, 0xa6, 0x85, 0x15, 0x34, 0x75, 0x71,
    0x5a, 0x84, 0xa8, 0x3c, 0xd2, 0xef, 0x5a, 0x4e, 0xd3, 0xde, 0x97, 0x8a,
    0xdd, 0xff, 0xbb, 0xcf, 0x0a, 0xaa, 0x86, 0x92, 0xbe, 0xb8, 0x50, 0xe4,
    0xcd, 0x6f, 0x80, 0x33, 0x30, 0x76, 0x13, 0x8f, 0xca, 0x7b, 0xdc, 0xec,
    0x5a, 0xca, 0x63, 0xc7, 0x03, 0x25, 0xef, 0xa8, 0x8a, 0x83, 0x58, 0x76,
    0x20, 0xfa, 0x16, 0x77, 0xd7, 0x79, 0x92, 0x63, 0x01, 0x48, 0x1a, 0xd8,
    0x7b, 0x67, 0xf1, 0x52, 0x55, 0x49, 0x4e, 0xd6, 0x6e, 0x4a, 0x5c, 0xd7,
    0x7a, 0x37, 0x36, 0x0c, 0xde, 0xdd, 0x8f, 0x44, 0xe8, 0xc2, 0xa7, 0x2c,
    0x2b, 0xb5, 0xaf, 0x64, 0x4b, 0x61, 0x07, 0x02, 0x03, 0x01, 0x00, 0x01,
    0x02, 0x81, 0x80, 0x74, 0x88, 0x64, 0x3f, 0x69, 0x45, 0x3a, 0x6d, 0xc7,
    0x7f, 0xb9, 0xa3, 0xc0, 0x6e, 0xec, 0xdc, 0xd4, 0x5a, 0xb5, 0x32, 0x85,
    0x5f, 0x19, 0xd4, 0xf8, 0xd4, 0x3f, 0x3c, 0xfa, 0xc2, 0xf6, 0x5f, 0xee,
    0xe6, 0xba, 0x87, 0x74, 0x2e, 0xc7, 0x0c, 0xd4, 0x42, 0xb8, 0x66, 0x85,
    0x9c, 0x7b, 0x24, 0x61, 0xaa, 0x16, 0x11, 0xf6, 0xb5, 0xb6, 0xa4, 0x0a,
    0xc9, 0x55, 0x2e, 0x81, 0xa5, 0x47, 0x61, 0xcb, 0x25, 0x8f, 0xc2, 0x15,
    0x7b, 0x0e, 0x7c, 0x36, 0x9f, 0x3a, 0xda, 0x58, 0x86, 0x1c, 0x5b, 0x83,
    0x79, 0xe6, 0x2b, 0xcc, 0xe6, 0xfa, 0x2c, 0x61, 0xf2, 0x78, 0x80, 0x1b,
    0xe2, 0xf3, 0x9d, 0x39, 0x2b, 0x65, 0x57, 0x91, 0x3d, 0x71, 0x99, 0x73,
    0xa5, 0xc2, 0x79, 0x20, 0x8c, 0x07, 0x4f, 0xe5, 0xb4, 0x60, 0x1f, 0x99,
    0xa2, 0xb1, 0x4f, 0x0c, 0xef, 0xbc, 0x59, 0x53, 0x00, 0x7d, 0xb1, 0x02,
    0x41, 0x00, 0xfc, 0x7e, 0x23, 0x65, 0x70, 0xf8, 0xce, 0xd3, 0x40, 0x41,
    0x80, 0x6a, 0x1d, 0x01, 0xd6, 0x01, 0xff, 0xb6, 0x1b, 0x3d, 0x3d, 0x59,
    0x09, 0x33, 0x79, 0xc0, 0x4f, 0xde, 0x96, 0x27, 0x4b, 0x18, 0xc6, 0xd9,
    0x78, 0xf1, 0xf4, 0x35, 0x46, 0xe9, 0x7c, 0x42, 0x7a, 0x5d, 0x9f, 0xef,
    0x54, 0xb8, 0xf7, 0x9f, 0xc4, 0x33, 0x6c, 0xf3, 0x8c, 0x32, 0x46, 0x87,
    0x67, 0x30, 0x7b, 0xa7, 0xac, 0xe3, 0x02, 0x41, 0x00, 0xfc, 0x2c, 0xdf,
    0x0c, 0x0d, 0x88, 0xf5, 0xb1, 0x92, 0xa8, 0x93, 0x47, 0x63, 0x55, 0xf5,
    0xca, 0x58, 0x43, 0xba, 0x1c, 0xe5, 0x9e, 0xb6, 0x95, 0x05, 0xcd, 0xb5,
    0x82, 0xdf, 0xeb, 0x04, 0x53, 0x9d, 0xbd, 0xc2, 0x38, 0x16, 0xb3, 0x62,
    0xdd, 0xa1, 0x46, 0xdb, 0x6d, 0x97, 0x93, 0x9f, 0x8a, 0xc3, 0x9b, 0x64,
    0x7e, 0x42, 0xe3, 0x32, 0x57, 0x19, 0x1b, 0xd5, 0x6e, 0x85, 0xfa, 0xb8,
    0x8d, 0x02, 0x41, 0x00, 0xbc, 0x3d, 0xde, 0x6d, 0xd6, 0x97, 0xe8, 0xba,
    0x9e, 0x81, 0x37, 0x17, 0xe5, 0xa0, 0x64, 0xc9, 0x00, 0xb7, 0xe7, 0xfe,
    0xf4, 0x29, 0xd9, 0x2e, 0x43, 0x6b, 0x19, 0x20, 0xbd, 0x99, 0x75, 0xe7,
    0x76, 0xf8, 0xd3, 0xae, 0xaf, 0x7e, 0xb8, 0xeb, 0x81, 0xf4, 0x9d, 0xfe,
    0x07, 0x2b, 0x0b, 0x63, 0x0b, 0x5a, 0x55, 0x90, 0x71, 0x7d, 0xf1, 0xdb,
    0xd9, 0xb1, 0x41, 0x41, 0x68, 0x2f, 0x4e, 0x39, 0x02, 0x40, 0x5a, 0x34,
    0x66, 0xd8, 0xf5, 0xe2, 0x7f, 0x18, 0xb5, 0x00, 0x6e, 0x26, 0x84, 0x27,
    0x14, 0x93, 0xfb, 0xfc, 0xc6, 0x0f, 0x5e, 0x27, 0xe6, 0xe1, 0xe9, 0xc0,
    0x8a, 0xe4, 0x34, 0xda, 0xe9, 0xa2, 0x4b, 0x73, 0xbc, 0x8c, 0xb9, 0xba,
    0x13, 0x6c, 0x7a, 0x2b, 0x51, 0x84, 0xa3, 0x4a, 0xe0, 0x30, 0x10, 0x06,
    0x7e, 0xed, 0x17, 0x5a, 0x14, 0x00, 0xc9, 0xef, 0x85, 0xea, 0x52, 0x2c,
    0xbc, 0x65, 0x02, 0x40, 0x51, 0xe3, 0xf2, 0x83, 0x19, 0x9b, 0xc4, 0x1e,
    0x2f, 0x50, 0x3d, 0xdf, 0x5a, 0xa2, 0x18, 0xca, 0x5f, 0x2e, 0x49, 0xaf,
    0x6f, 0xcc, 0xfa, 0x65, 0x77, 0x94, 0xb5, 0xa1, 0x0a, 0xa9, 0xd1, 0x8a,
    0x39, 0x37, 0xf4, 0x0b, 0xa0, 0xd7, 0x82, 0x27, 0x5e, 0xae, 0x17, 0x17,
    0xa1, 0x1e, 0x54, 0x34, 0xbf, 0x6e, 0xc4, 0x8e, 0x99, 0x5d, 0x08, 0xf1,
    0x2d, 0x86, 0x9d, 0xa5, 0x20, 0x1b, 0xe5, 0xdf,
};

static const uint8_t kExampleDSAKeyDER[] = {
    0x30, 0x82, 0x03, 0x56, 0x02, 0x01, 0x00, 0x02, 0x82, 0x01, 0x01, 0x00,
    0x9e, 0x12, 0xfa, 0xb3, 0xde, 0x12, 0x21, 0x35, 0x01, 0xdd, 0x82, 0xaa,
    0x10, 0xca, 0x2d, 0x10, 0x1d, 0x2d, 0x4e, 0xbf, 0xef, 0x4d, 0x2a, 0x3f,
    0x8d, 0xaa, 0x0f, 0xe0, 0xce, 0xda, 0xd8, 0xd6, 0xaf, 0x85, 0x61, 0x6a,
    0xa2, 0xf3, 0x25, 0x2c, 0x0a, 0x2b, 0x5a, 0x6d, 0xb0, 0x9e, 0x6f, 0x14,
    0x90, 0x0e, 0x0d, 0xdb, 0x83, 0x11, 0x87, 0x6d, 0xd8, 0xf9, 0x66, 0x95,
    0x25, 0xf9, 0x9e, 0xd6, 0x59, 0x49, 0xe1, 0x84, 0xd5, 0x06, 0x47, 0x93,
    0x27, 0x11, 0x69, 0xa2, 0x28, 0x68, 0x0b, 0x95, 0xec, 0x12, 0xf5, 0x9a,
    0x8e, 0x20, 0xb2, 0x1f, 0x2b, 0x58, 0xeb, 0x2a, 0x20, 0x12, 0xd3, 0x5b,
    0xde, 0x2e, 0xe3, 0x51, 0x82, 0x2f, 0xe8, 0xf3, 0x2d, 0x0a, 0x33, 0x05,
    0x65, 0xdc, 0xce, 0x5c, 0x67, 0x2b, 0x72, 0x59, 0xc1, 0x4b, 0x24, 0x33,
    0xd0, 0xb5, 0xb2, 0xca, 0x2b, 0x2d, 0xb0, 0xab, 0x62, 0x6e, 0x8f, 0x13,
    0xf4, 0x7f, 0xe0, 0x34, 0x5d, 0x90, 0x4e, 0x72, 0x94, 0xbb, 0x03, 0x8e,
    0x9c, 0xe2, 0x1a, 0x9e, 0x58, 0x0b, 0x83, 0x35, 0x62, 0x78, 0x70, 0x6c,
    0xfe, 0x76, 0x84, 0x36, 0xc6, 0x9d, 0xe1, 0x49, 0xcc, 0xff, 0x98, 0xb4,
    0xaa, 0xb8, 0xcb, 0x4f, 0x63, 0x85, 0xc9, 0xf1, 0x02, 0xce, 0x59, 0x34,
    0x6e, 0xae, 0xef, 0x27, 0xe0, 0xad, 0x22, 0x2d, 0x53, 0xd6, 0xe8, 0x9c,
    0xc8, 0xcd, 0xe5, 0x77, 0x6d, 0xd0, 0x00, 0x57, 0xb0, 0x3f, 0x2d, 0x88,
    0xab, 0x3c, 0xed, 0xba, 0xfd, 0x7b, 0x58, 0x5f, 0x0b, 0x7f, 0x78, 0x35,
    0xe1, 0x7a, 0x37, 0x28, 0xbb, 0xf2, 0x5e, 0xa6, 0x25, 0x72, 0xf2, 0x45,
    0xdc, 0x11, 0x1f, 0x3c, 0xe3, 0x9c, 0xb6, 0xff, 0xac, 0xc3, 0x1b, 0x0a,
    0x27, 0x90, 0xe7, 0xbd, 0xe9, 0x02, 0x24, 0xea, 0x9b, 0x09, 0x31, 0x53,
    0x62, 0xaf, 0x3d, 0x2b, 0x02, 0x21, 0x00, 0xf3, 0x81, 0xdc, 0xf5, 0x3e,
    0xbf, 0x72, 0x4f, 0x8b, 0x2e, 0x5c, 0xa8, 0x2c, 0x01, 0x0f, 0xb4, 0xb5,
    0xed, 0xa9, 0x35, 0x8d, 0x0f, 0xd8, 0x8e, 0xd2, 0x78, 0x58, 0x94, 0x88,
    0xb5, 0x4f, 0xc3, 0x02, 0x82, 0x01, 0x00, 0x0c, 0x40, 0x2a, 0x72, 0x5d,
    0xcc, 0x3a, 0x62, 0xe0, 0x2b, 0xf4, 0xcf, 0x43, 0xcd, 0x17, 0xf4, 0xa4,
    0x93, 0x59, 0x12, 0x20, 0x22, 0x36, 0x69, 0xcf, 0x41, 0x93, 0xed, 0xab,
    0x42, 0x3a, 0xd0, 0x8d, 0xfb, 0x55, 0x2e, 0x30, 0x8a, 0x6a, 0x57, 0xa5,
    0xff, 0xbc, 0x7c, 0xd0, 0xfb, 0x20, 0x87, 0xf8, 0x1f, 0x8d, 0xf0, 0xcb,
    0x08, 0xab, 0x21, 0x33, 0x28, 0x7d, 0x2b, 0x69, 0x68, 0x71, 0x4a, 0x94,
    0xf6, 0x33, 0xc9, 0x40, 0x84, 0x5a, 0x48, 0xa3, 0xe1, 0x67, 0x08, 0xdd,
    0xe7, 0x61, 0xcc, 0x6a, 0x8e, 0xab, 0x2d, 0x84, 0xdb, 0x21, 0xb6, 0xea,
    0x5b, 0x07, 0x68, 0x14, 0x93, 0xcc, 0x9c, 0x31, 0xfb, 0xc3, 0x68, 0xb2,
    0x43, 0xf6, 0xdd, 0xf8, 0xc9, 0x32, 0xa8, 0xb4, 0x03, 0x8f, 0x44, 0xe7,
    0xb1, 0x5c, 0xa8, 0x76, 0x34, 0x4a, 0x14, 0x78, 0x59, 0xf2, 0xb4, 0x3b,
    0x39, 0x45, 0x86, 0x68, 0xad, 0x5e, 0x0a, 0x1a, 0x9a, 0x66, 0x95, 0x46,
    0xdd, 0x28, 0x12, 0xe3, 0xb3, 0x61, 0x7a, 0x0a, 0xef, 0x99, 0xd5, 0x8e,
    0x3b, 0xb4, 0xcc, 0x87, 0xfd, 0x94, 0x22, 0x5e, 0x01, 0xd2, 0xdc, 0xc4,
    0x69, 0xa7, 0x72, 0x68, 0x14, 0x6c, 0x51, 0x91, 0x8f, 0x18, 0xe8, 0xb4,
    0xd7, 0x0a, 0xa1, 0xf0, 0xc7, 0x62, 0x3b, 0xcc, 0x52, 0xcf, 0x37, 0x31,
    0xd3, 0x86, 0x41, 0xb2, 0xd2, 0x83, 0x0b, 0x7e, 0xec, 0xb2, 0xf0, 0x95,
    0x52, 0xff, 0x13, 0x7d, 0x04, 0x6e, 0x49, 0x4e, 0x7f, 0x33, 0xc3, 0x59,
    0x00, 0x02, 0xb1, 0x6d, 0x1b, 0x97, 0xd9, 0x36, 0xfd, 0xa2, 0x8f, 0x90,
    0xc3, 0xed, 0x3c, 0xa3, 0x53, 0x38, 0x16, 0x8a, 0xc1, 0x6f, 0x77, 0xc3,
    0xc5, 0x7a, 0xdc, 0x2e, 0x8f, 0x7c, 0x6c, 0x22, 0x56, 0xe4, 0x1a, 0x5f,
    0x65, 0x45, 0x05, 0x90, 0xdb, 0xb5, 0xbc, 0xf0, 0x6d, 0x66, 0x61, 0x02,
    0x82, 0x01, 0x00, 0x31, 0x97, 0x31, 0xa1, 0x4e, 0x38, 0x56, 0x88, 0xdb,
    0x94, 0x1d, 0xbf, 0x65, 0x5c, 0xda, 0x4b, 0xc2, 0x10, 0xde, 0x74, 0x20,
    0x03, 0xce, 0x13, 0x60, 0xf2, 0x25, 0x1d, 0x55, 0x7c, 0x5d, 0x94, 0x82,
    0x54, 0x08, 0x53, 0xdb, 0x85, 0x95, 0xbf, 0xdd, 0x5e, 0x50, 0xd5, 0x96,
    0xe0, 0x79, 0x51, 0x1b, 0xbf, 0x4d, 0x4e, 0xb9, 0x3a, 0xc5, 0xee, 0xc4,
    0x5e, 0x98, 0x75, 0x7b, 0xbe, 0xff, 0x30, 0xe6, 0xd0, 0x7b, 0xa6, 0xf1,
    0xbc, 0x29, 0xea, 0xdf, 0xec, 0xf3, 0x8b, 0xfa, 0x83, 0x11, 0x9f, 0x3f,
    0xf0, 0x5d, 0x06, 0x51, 0x32, 0xaa, 0x21, 0xfc, 0x26, 0x17, 0xe7, 0x50,
    0xc2, 0x16, 0xba, 0xfa, 0x54, 0xb7, 0x7e, 0x1d, 0x2c, 0xa6, 0xa3, 0x41,
    0x66, 0x33, 0x94, 0x83, 0xb9, 0xbf, 0xa0, 0x4f, 0xbd, 0xa6, 0xfd, 0x2c,
    0x81, 0x58, 0x35, 0x33, 0x39, 0xc0, 0x6d, 0x33, 0x40, 0x56, 0x64, 0x12,
    0x5a, 0xcd, 0x35, 0x53, 0x21, 0x78, 0x8f, 0x27, 0x24, 0x37, 0x66, 0x8a,
    0xdf, 0x5e, 0x5f, 0x63, 0xfc, 0x8b, 0x2d, 0xef, 0x57, 0xdb, 0x40, 0x25,
    0xd5, 0x17, 0x53, 0x0b, 0xe4, 0xa5, 0xae, 0x54, 0xbf, 0x46, 0x4f, 0xa6,
    0x79, 0xc3, 0x74, 0xfa, 0x1f, 0x85, 0x34, 0x64, 0x6d, 0xc5, 0x03, 0xeb,
    0x72, 0x98, 0x80, 0x7b, 0xc0, 0x8f, 0x35, 0x11, 0xa7, 0x09, 0xeb, 0x51,
    0xe0, 0xb0, 0xac, 0x92, 0x14, 0xf2, 0xad, 0x37, 0x95, 0x5a, 0xba, 0x8c,
    0xc4, 0xdb, 0xed, 0xc4, 0x4e, 0x8b, 0x8f, 0x84, 0x33, 0x64, 0xf8, 0x57,
    0x12, 0xd7, 0x08, 0x7e, 0x90, 0x66, 0xdf, 0x91, 0x50, 0x23, 0xf2, 0x73,
    0xc0, 0x6b, 0xb1, 0x15, 0xdd, 0x64, 0xd7, 0xc9, 0x75, 0x17, 0x73, 0x72,
    0xda, 0x33, 0xc4, 0x6f, 0xa5, 0x47, 0xa1, 0xcc, 0xd1, 0xc6, 0x62, 0xe5,
    0xca, 0xab, 0x5f, 0x2a, 0x8f, 0x6b, 0xcc, 0x02, 0x21, 0x00, 0xb0, 0xc7,
    0x68, 0x70, 0x27, 0x43, 0xbc, 0x51, 0x24, 0x29, 0x93, 0xa9, 0x71, 0xa5,
    0x28, 0x89, 0x79, 0x54, 0x44, 0xf7, 0xc6, 0x45, 0x22, 0x03, 0xd0, 0xce,
    0x84, 0xfe, 0x61, 0x17, 0xd4, 0x6e,
};

static const uint8_t kMsg[] = {1, 2, 3, 4};

static const uint8_t kSignature[] = {
    0xa5, 0xf0, 0x8a, 0x47, 0x5d, 0x3c, 0xb3, 0xcc, 0xa9, 0x79, 0xaf, 0x4d,
    0x8c, 0xae, 0x4c, 0x14, 0xef, 0xc2, 0x0b, 0x34, 0x36, 0xde, 0xf4, 0x3e,
    0x3d, 0xbb, 0x4a, 0x60, 0x5c, 0xc8, 0x91, 0x28, 0xda, 0xfb, 0x7e, 0x04,
    0x96, 0x7e, 0x63, 0x13, 0x90, 0xce, 0xb9, 0xb4, 0x62, 0x7a, 0xfd, 0x09,
    0x3d, 0xc7, 0x67, 0x78, 0x54, 0x04, 0xeb, 0x52, 0x62, 0x6e, 0x24, 0x67,
    0xb4, 0x40, 0xfc, 0x57, 0x62, 0xc6, 0xf1, 0x67, 0xc1, 0x97, 0x8f, 0x6a,
    0xa8, 0xae, 0x44, 0x46, 0x5e, 0xab, 0x67, 0x17, 0x53, 0x19, 0x3a, 0xda,
    0x5a, 0xc8, 0x16, 0x3e, 0x86, 0xd5, 0xc5, 0x71, 0x2f, 0xfc, 0x23, 0x48,
    0xd9, 0x0b, 0x13, 0xdd, 0x7b, 0x5a, 0x25, 0x79, 0xef, 0xa5, 0x7b, 0x04,
    0xed, 0x44, 0xf6, 0x18, 0x55, 0xe4, 0x0a, 0xe9, 0x57, 0x79, 0x5d, 0xd7,
    0x55, 0xa7, 0xab, 0x45, 0x02, 0x97, 0x60, 0x42,
};

// kExampleRSAKeyPKCS8 is kExampleRSAKeyDER encoded in a PKCS #8
// PrivateKeyInfo.
static const uint8_t kExampleRSAKeyPKCS8[] = {
    0x30, 0x82, 0x02, 0x76, 0x02, 0x01, 0x00, 0x30, 0x0d, 0x06, 0x09, 0x2a,
    0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01, 0x05, 0x00, 0x04, 0x82,
    0x02, 0x60, 0x30, 0x82, 0x02, 0x5c, 0x02, 0x01, 0x00, 0x02, 0x81, 0x81,
    0x00, 0xf8, 0xb8, 0x6c, 0x83, 0xb4, 0xbc, 0xd9, 0xa8, 0x57, 0xc0, 0xa5,
    0xb4, 0x59, 0x76, 0x8c, 0x54, 0x1d, 0x79, 0xeb, 0x22, 0x52, 0x04, 0x7e,
    0xd3, 0x37, 0xeb, 0x41, 0xfd, 0x83, 0xf9, 0xf0, 0xa6, 0x85, 0x15, 0x34,
    0x75, 0x71, 0x5a, 0x84, 0xa8, 0x3c, 0xd2, 0xef, 0x5a, 0x4e, 0xd3, 0xde,
    0x97, 0x8a, 0xdd, 0xff, 0xbb, 0xcf, 0x0a, 0xaa, 0x86, 0x92, 0xbe, 0xb8,
    0x50, 0xe4, 0xcd, 0x6f, 0x80, 0x33, 0x30, 0x76, 0x13, 0x8f, 0xca, 0x7b,
    0xdc, 0xec, 0x5a, 0xca, 0x63, 0xc7, 0x03, 0x25, 0xef, 0xa8, 0x8a, 0x83,
    0x58, 0x76, 0x20, 0xfa, 0x16, 0x77, 0xd7, 0x79, 0x92, 0x63, 0x01, 0x48,
    0x1a, 0xd8, 0x7b, 0x67, 0xf1, 0x52, 0x55, 0x49, 0x4e, 0xd6, 0x6e, 0x4a,
    0x5c, 0xd7, 0x7a, 0x37, 0x36, 0x0c, 0xde, 0xdd, 0x8f, 0x44, 0xe8, 0xc2,
    0xa7, 0x2c, 0x2b, 0xb5, 0xaf, 0x64, 0x4b, 0x61, 0x07, 0x02, 0x03, 0x01,
    0x00, 0x01, 0x02, 0x81, 0x80, 0x74, 0x88, 0x64, 0x3f, 0x69, 0x45, 0x3a,
    0x6d, 0xc7, 0x7f, 0xb9, 0xa3, 0xc0, 0x6e, 0xec, 0xdc, 0xd4, 0x5a, 0xb5,
    0x32, 0x85, 0x5f, 0x19, 0xd4, 0xf8, 0xd4, 0x3f, 0x3c, 0xfa, 0xc2, 0xf6,
    0x5f, 0xee, 0xe6, 0xba, 0x87, 0x74, 0x2e, 0xc7, 0x0c, 0xd4, 0x42, 0xb8,
    0x66, 0x85, 0x9c, 0x7b, 0x24, 0x61, 0xaa, 0x16, 0x11, 0xf6, 0xb5, 0xb6,
    0xa4, 0x0a, 0xc9, 0x55, 0x2e, 0x81, 0xa5, 0x47, 0x61, 0xcb, 0x25, 0x8f,
    0xc2, 0x15, 0x7b, 0x0e, 0x7c, 0x36, 0x9f, 0x3a, 0xda, 0x58, 0x86, 0x1c,
    0x5b, 0x83, 0x79, 0xe6, 0x2b, 0xcc, 0xe6, 0xfa, 0x2c, 0x61, 0xf2, 0x78,
    0x80, 0x1b, 0xe2, 0xf3, 0x9d, 0x39, 0x2b, 0x65, 0x57, 0x91, 0x3d, 0x71,
    0x99, 0x73, 0xa5, 0xc2, 0x79, 0x20, 0x8c, 0x07, 0x4f, 0xe5, 0xb4, 0x60,
    0x1f, 0x99, 0xa2, 0xb1, 0x4f, 0x0c, 0xef, 0xbc, 0x59, 0x53, 0x00, 0x7d,
    0xb1, 0x02, 0x41, 0x00, 0xfc, 0x7e, 0x23, 0x65, 0x70, 0xf8, 0xce, 0xd3,
    0x40, 0x41, 0x80, 0x6a, 0x1d, 0x01, 0xd6, 0x01, 0xff, 0xb6, 0x1b, 0x3d,
    0x3d, 0x59, 0x09, 0x33, 0x79, 0xc0, 0x4f, 0xde, 0x96, 0x27, 0x4b, 0x18,
    0xc6, 0xd9, 0x78, 0xf1, 0xf4, 0x35, 0x46, 0xe9, 0x7c, 0x42, 0x7a, 0x5d,
    0x9f, 0xef, 0x54, 0xb8, 0xf7, 0x9f, 0xc4, 0x33, 0x6c, 0xf3, 0x8c, 0x32,
    0x46, 0x87, 0x67, 0x30, 0x7b, 0xa7, 0xac, 0xe3, 0x02, 0x41, 0x00, 0xfc,
    0x2c, 0xdf, 0x0c, 0x0d, 0x88, 0xf5, 0xb1, 0x92, 0xa8, 0x93, 0x47, 0x63,
    0x55, 0xf5, 0xca, 0x58, 0x43, 0xba, 0x1c, 0xe5, 0x9e, 0xb6, 0x95, 0x05,
    0xcd, 0xb5, 0x82, 0xdf, 0xeb, 0x04, 0x53, 0x9d, 0xbd, 0xc2, 0x38, 0x16,
    0xb3, 0x62, 0xdd, 0xa1, 0x46, 0xdb, 0x6d, 0x97, 0x93, 0x9f, 0x8a, 0xc3,
    0x9b, 0x64, 0x7e, 0x42, 0xe3, 0x32, 0x57, 0x19, 0x1b, 0xd5, 0x6e, 0x85,
    0xfa, 0xb8, 0x8d, 0x02, 0x41, 0x00, 0xbc, 0x3d, 0xde, 0x6d, 0xd6, 0x97,
    0xe8, 0xba, 0x9e, 0x81, 0x37, 0x17, 0xe5, 0xa0, 0x64, 0xc9, 0x00, 0xb7,
    0xe7, 0xfe, 0xf4, 0x29, 0xd9, 0x2e, 0x43, 0x6b, 0x19, 0x20, 0xbd, 0x99,
    0x75, 0xe7, 0x76, 0xf8, 0xd3, 0xae, 0xaf, 0x7e, 0xb8, 0xeb, 0x81, 0xf4,
    0x9d, 0xfe, 0x07, 0x2b, 0x0b, 0x63, 0x0b, 0x5a, 0x55, 0x90, 0x71, 0x7d,
    0xf1, 0xdb, 0xd9, 0xb1, 0x41, 0x41, 0x68, 0x2f, 0x4e, 0x39, 0x02, 0x40,
    0x5a, 0x34, 0x66, 0xd8, 0xf5, 0xe2, 0x7f, 0x18, 0xb5, 0x00, 0x6e, 0x26,
    0x84, 0x27, 0x14, 0x93, 0xfb, 0xfc, 0xc6, 0x0f, 0x5e, 0x27, 0xe6, 0xe1,
    0xe9, 0xc0, 0x8a, 0xe4, 0x34, 0xda, 0xe9, 0xa2, 0x4b, 0x73, 0xbc, 0x8c,
    0xb9, 0xba, 0x13, 0x6c, 0x7a, 0x2b, 0x51, 0x84, 0xa3, 0x4a, 0xe0, 0x30,
    0x10, 0x06, 0x7e, 0xed, 0x17, 0x5a, 0x14, 0x00, 0xc9, 0xef, 0x85, 0xea,
    0x52, 0x2c, 0xbc, 0x65, 0x02, 0x40, 0x51, 0xe3, 0xf2, 0x83, 0x19, 0x9b,
    0xc4, 0x1e, 0x2f, 0x50, 0x3d, 0xdf, 0x5a, 0xa2, 0x18, 0xca, 0x5f, 0x2e,
    0x49, 0xaf, 0x6f, 0xcc, 0xfa, 0x65, 0x77, 0x94, 0xb5, 0xa1, 0x0a, 0xa9,
    0xd1, 0x8a, 0x39, 0x37, 0xf4, 0x0b, 0xa0, 0xd7, 0x82, 0x27, 0x5e, 0xae,
    0x17, 0x17, 0xa1, 0x1e, 0x54, 0x34, 0xbf, 0x6e, 0xc4, 0x8e, 0x99, 0x5d,
    0x08, 0xf1, 0x2d, 0x86, 0x9d, 0xa5, 0x20, 0x1b, 0xe5, 0xdf,
};

// kExampleECKeyDER is a sample EC private key encoded as an ECPrivateKey
// structure.
static const uint8_t kExampleECKeyDER[] = {
    0x30, 0x77, 0x02, 0x01, 0x01, 0x04, 0x20, 0x07, 0x0f, 0x08, 0x72, 0x7a,
    0xd4, 0xa0, 0x4a, 0x9c, 0xdd, 0x59, 0xc9, 0x4d, 0x89, 0x68, 0x77, 0x08,
    0xb5, 0x6f, 0xc9, 0x5d, 0x30, 0x77, 0x0e, 0xe8, 0xd1, 0xc9, 0xce, 0x0a,
    0x8b, 0xb4, 0x6a, 0xa0, 0x0a, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d,
    0x03, 0x01, 0x07, 0xa1, 0x44, 0x03, 0x42, 0x00, 0x04, 0xe6, 0x2b, 0x69,
    0xe2, 0xbf, 0x65, 0x9f, 0x97, 0xbe, 0x2f, 0x1e, 0x0d, 0x94, 0x8a, 0x4c,
    0xd5, 0x97, 0x6b, 0xb7, 0xa9, 0x1e, 0x0d, 0x46, 0xfb, 0xdd, 0xa9, 0xa9,
    0x1e, 0x9d, 0xdc, 0xba, 0x5a, 0x01, 0xe7, 0xd6, 0x97, 0xa8, 0x0a, 0x18,
    0xf9, 0xc3, 0xc4, 0xa3, 0x1e, 0x56, 0xe2, 0x7c, 0x83, 0x48, 0xdb, 0x16,
    0x1a, 0x1c, 0xf5, 0x1d, 0x7e, 0xf1, 0x94, 0x2d, 0x4b, 0xcf, 0x72, 0x22,
    0xc1,
};

// kExampleECKeyPKCS8 is a sample EC private key encoded as a PKCS#8
// PrivateKeyInfo.
static const uint8_t kExampleECKeyPKCS8[] = {
    0x30, 0x81, 0x87, 0x02, 0x01, 0x00, 0x30, 0x13, 0x06, 0x07, 0x2a, 0x86,
    0x48, 0xce, 0x3d, 0x02, 0x01, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d,
    0x03, 0x01, 0x07, 0x04, 0x6d, 0x30, 0x6b, 0x02, 0x01, 0x01, 0x04, 0x20,
    0x43, 0x09, 0xc0, 0x67, 0x75, 0x21, 0x47, 0x9d, 0xa8, 0xfa, 0x16, 0xdf,
    0x15, 0x73, 0x61, 0x34, 0x68, 0x6f, 0xe3, 0x8e, 0x47, 0x91, 0x95, 0xab,
    0x79, 0x4a, 0x72, 0x14, 0xcb, 0xe2, 0x49, 0x4f, 0xa1, 0x44, 0x03, 0x42,
    0x00, 0x04, 0xde, 0x09, 0x08, 0x07, 0x03, 0x2e, 0x8f, 0x37, 0x9a, 0xd5,
    0xad, 0xe5, 0xc6, 0x9d, 0xd4, 0x63, 0xc7, 0x4a, 0xe7, 0x20, 0xcb, 0x90,
    0xa0, 0x1f, 0x18, 0x18, 0x72, 0xb5, 0x21, 0x88, 0x38, 0xc0, 0xdb, 0xba,
    0xf6, 0x99, 0xd8, 0xa5, 0x3b, 0x83, 0xe9, 0xe3, 0xd5, 0x61, 0x99, 0x73,
    0x42, 0xc6, 0x6c, 0xe8, 0x0a, 0x95, 0x40, 0x41, 0x3b, 0x0d, 0x10, 0xa7,
    0x4a, 0x93, 0xdb, 0x5a, 0xe7, 0xec,
};

// kExampleECKeySpecifiedCurvePKCS8 is a sample EC private key encoded as a
// PKCS#8 PrivateKeyInfo with P-256's parameters spelled out rather than using
// the curve OID.
static const uint8_t kExampleECKeySpecifiedCurvePKCS8[] = {
    0x30, 0x82, 0x01, 0x79, 0x02, 0x01, 0x00, 0x30, 0x82, 0x01, 0x03, 0x06,
    0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02, 0x01, 0x30, 0x81, 0xf7, 0x02,
    0x01, 0x01, 0x30, 0x2c, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x01,
    0x01, 0x02, 0x21, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x30, 0x5b, 0x04, 0x20, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc,
    0x04, 0x20, 0x5a, 0xc6, 0x35, 0xd8, 0xaa, 0x3a, 0x93, 0xe7, 0xb3, 0xeb,
    0xbd, 0x55, 0x76, 0x98, 0x86, 0xbc, 0x65, 0x1d, 0x06, 0xb0, 0xcc, 0x53,
    0xb0, 0xf6, 0x3b, 0xce, 0x3c, 0x3e, 0x27, 0xd2, 0x60, 0x4b, 0x03, 0x15,
    0x00, 0xc4, 0x9d, 0x36, 0x08, 0x86, 0xe7, 0x04, 0x93, 0x6a, 0x66, 0x78,
    0xe1, 0x13, 0x9d, 0x26, 0xb7, 0x81, 0x9f, 0x7e, 0x90, 0x04, 0x41, 0x04,
    0x6b, 0x17, 0xd1, 0xf2, 0xe1, 0x2c, 0x42, 0x47, 0xf8, 0xbc, 0xe6, 0xe5,
    0x63, 0xa4, 0x40, 0xf2, 0x77, 0x03, 0x7d, 0x81, 0x2d, 0xeb, 0x33, 0xa0,
    0xf4, 0xa1, 0x39, 0x45, 0xd8, 0x98, 0xc2, 0x96, 0x4f, 0xe3, 0x42, 0xe2,
    0xfe, 0x1a, 0x7f, 0x9b, 0x8e, 0xe7, 0xeb, 0x4a, 0x7c, 0x0f, 0x9e, 0x16,
    0x2b, 0xce, 0x33, 0x57, 0x6b, 0x31, 0x5e, 0xce, 0xcb, 0xb6, 0x40, 0x68,
    0x37, 0xbf, 0x51, 0xf5, 0x02, 0x21, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00,
    0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xbc,
    0xe6, 0xfa, 0xad, 0xa7, 0x17, 0x9e, 0x84, 0xf3, 0xb9, 0xca, 0xc2, 0xfc,
    0x63, 0x25, 0x51, 0x02, 0x01, 0x01, 0x04, 0x6d, 0x30, 0x6b, 0x02, 0x01,
    0x01, 0x04, 0x20, 0x43, 0x09, 0xc0, 0x67, 0x75, 0x21, 0x47, 0x9d, 0xa8,
    0xfa, 0x16, 0xdf, 0x15, 0x73, 0x61, 0x34, 0x68, 0x6f, 0xe3, 0x8e, 0x47,
    0x91, 0x95, 0xab, 0x79, 0x4a, 0x72, 0x14, 0xcb, 0xe2, 0x49, 0x4f, 0xa1,
    0x44, 0x03, 0x42, 0x00, 0x04, 0xde, 0x09, 0x08, 0x07, 0x03, 0x2e, 0x8f,
    0x37, 0x9a, 0xd5, 0xad, 0xe5, 0xc6, 0x9d, 0xd4, 0x63, 0xc7, 0x4a, 0xe7,
    0x20, 0xcb, 0x90, 0xa0, 0x1f, 0x18, 0x18, 0x72, 0xb5, 0x21, 0x88, 0x38,
    0xc0, 0xdb, 0xba, 0xf6, 0x99, 0xd8, 0xa5, 0x3b, 0x83, 0xe9, 0xe3, 0xd5,
    0x61, 0x99, 0x73, 0x42, 0xc6, 0x6c, 0xe8, 0x0a, 0x95, 0x40, 0x41, 0x3b,
    0x0d, 0x10, 0xa7, 0x4a, 0x93, 0xdb, 0x5a, 0xe7, 0xec,
};

// kExampleBadECKeyDER is a sample EC private key encoded as an ECPrivateKey
// structure. The private key is equal to the order and will fail to import.
static const uint8_t kExampleBadECKeyDER[] = {
    0x30, 0x66, 0x02, 0x01, 0x00, 0x30, 0x13, 0x06, 0x07, 0x2A, 0x86, 0x48,
    0xCE, 0x3D, 0x02, 0x01, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03,
    0x01, 0x07, 0x04, 0x4C, 0x30, 0x4A, 0x02, 0x01, 0x01, 0x04, 0x20, 0xFF,
    0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xBC, 0xE6, 0xFA, 0xAD, 0xA7, 0x17, 0x9E, 0x84, 0xF3,
    0xB9, 0xCA, 0xC2, 0xFC, 0x63, 0x25, 0x51, 0xA1, 0x23, 0x03, 0x21, 0x00,
    0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xBC, 0xE6, 0xFA, 0xAD, 0xA7, 0x17, 0x9E, 0x84,
    0xF3, 0xB9, 0xCA, 0xC2, 0xFC, 0x63, 0x25, 0x51
};

// kExampleBadECKeyDER2 is a sample EC private key encoded as an ECPrivateKey
// structure, but with the curve OID swapped out for 1.1.1.1.1.1.1.1.1. It is
// then concatenated with an ECPrivateKey wrapped in a PrivateKeyInfo,
// optional public key omitted, and with the private key chopped off.
static const uint8_t kExampleBadECKeyDER2[] = {
    0x30, 0x77, 0x02, 0x01, 0x01, 0x04, 0x20, 0x07, 0x0f, 0x08, 0x72, 0x7a,
    0xd4, 0xa0, 0x4a, 0x9c, 0xdd, 0x59, 0xc9, 0x4d, 0x89, 0x68, 0x77, 0x08,
    0xb5, 0x6f, 0xc9, 0x5d, 0x30, 0x77, 0x0e, 0xe8, 0xd1, 0xc9, 0xce, 0x0a,
    0x8b, 0xb4, 0x6a, 0xa0, 0x0a, 0x06, 0x08, 0x29, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0xa1, 0x44, 0x03, 0x42, 0x00, 0x04, 0xe6, 0x2b, 0x69,
    0xe2, 0xbf, 0x65, 0x9f, 0x97, 0xbe, 0x2f, 0x1e, 0x0d, 0x94, 0x8a, 0x4c,
    0xd5, 0x97, 0x6b, 0xb7, 0xa9, 0x1e, 0x0d, 0x46, 0xfb, 0xdd, 0xa9, 0xa9,
    0x1e, 0x9d, 0xdc, 0xba, 0x5a, 0x01, 0xe7, 0xd6, 0x97, 0xa8, 0x0a, 0x18,
    0xf9, 0xc3, 0xc4, 0xa3, 0x1e, 0x56, 0xe2, 0x7c, 0x83, 0x48, 0xdb, 0x16,
    0x1a, 0x1c, 0xf5, 0x1d, 0x7e, 0xf1, 0x94, 0x2d, 0x4b, 0xcf, 0x72, 0x22,
    0xc1, 0x30, 0x41, 0x02, 0x01, 0x00, 0x30, 0x13, 0x06, 0x07, 0x2a, 0x86,
    0x48, 0xce, 0x3d, 0x02, 0x01, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce, 0x3d,
    0x03, 0x01, 0x07, 0x04, 0x27, 0x30, 0x25, 0x02, 0x01, 0x01, 0x04, 0x20,
    0x07,
};

// kInvalidPrivateKey is an invalid private key. See
// https://rt.openssl.org/Ticket/Display.html?id=4131.
static const uint8_t kInvalidPrivateKey[] = {
    0x30, 0x39, 0x02, 0x01, 0x02, 0x30, 0x09, 0x06, 0x01, 0x38, 0x08,
    0x04, 0x69, 0x30, 0x30, 0x80, 0x30, 0x19, 0x01, 0x02, 0x9f, 0xf8,
    0x8b, 0x29, 0x80, 0x30, 0xb0, 0x1b, 0x06, 0x09, 0x22, 0xbe, 0x08,
    0x04, 0xe9, 0x30, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x3a, 0x01, 0x80,
    0x09, 0x30, 0x80, 0x06, 0x01, 0x02, 0x30, 0x80, 0x30, 0x01, 0x3b,
    0x02, 0x00, 0x00, 0x04, 0x20, 0x30, 0x82, 0x04, 0xe9, 0x30, 0xc3,
    0xe8, 0x30, 0x01, 0x05, 0x30, 0x80, 0x30, 0x01, 0x3b, 0x01, 0x04,
    0x02, 0x02, 0xff, 0x00, 0x30, 0x29, 0x02, 0x11, 0x03, 0x29, 0x29,
    0x02, 0x00, 0x99, 0x30, 0x80, 0x06, 0x21, 0x02, 0x24, 0x04, 0xe8,
    0x30, 0x01, 0x01, 0x04, 0x30, 0x80, 0x1b, 0x06, 0x09, 0x2a, 0x86,
    0x48, 0x30, 0x01, 0xaa, 0x02, 0x86, 0xc0, 0x30, 0xdf, 0xe9, 0x80,
};

static bssl::UniquePtr<EVP_PKEY> LoadExampleRSAKey() {
  bssl::UniquePtr<RSA> rsa(RSA_private_key_from_bytes(kExampleRSAKeyDER,
                                           sizeof(kExampleRSAKeyDER)));
  if (!rsa) {
    return nullptr;
  }
  bssl::UniquePtr<EVP_PKEY> pkey(EVP_PKEY_new());
  if (!pkey || !EVP_PKEY_set1_RSA(pkey.get(), rsa.get())) {
    return nullptr;
  }
  return pkey;
}

TEST(EVPExtraTest, DigestSignInit) {
  bssl::UniquePtr<EVP_PKEY> pkey = LoadExampleRSAKey();
  ASSERT_TRUE(pkey);
  bssl::ScopedEVP_MD_CTX md_ctx;
  ASSERT_TRUE(
      EVP_DigestSignInit(md_ctx.get(), NULL, EVP_sha256(), NULL, pkey.get()));
  ASSERT_TRUE(EVP_DigestSignUpdate(md_ctx.get(), kMsg, sizeof(kMsg)));

  // Determine the size of the signature.
  size_t sig_len = 0;
  ASSERT_TRUE(EVP_DigestSignFinal(md_ctx.get(), NULL, &sig_len));

  // Sanity check for testing.
  EXPECT_EQ(static_cast<size_t>(EVP_PKEY_size(pkey.get())), sig_len);

  std::vector<uint8_t> sig;
  sig.resize(sig_len);
  ASSERT_TRUE(EVP_DigestSignFinal(md_ctx.get(), sig.data(), &sig_len));
  sig.resize(sig_len);

  // Ensure that the signature round-trips.
  md_ctx.Reset();
  ASSERT_TRUE(
      EVP_DigestVerifyInit(md_ctx.get(), NULL, EVP_sha256(), NULL, pkey.get()));
  ASSERT_TRUE(EVP_DigestVerifyUpdate(md_ctx.get(), kMsg, sizeof(kMsg)));
  ASSERT_TRUE(EVP_DigestVerifyFinal(md_ctx.get(), sig.data(), sig_len));
}

TEST(EVPExtraTest, DigestVerifyInit) {
  bssl::UniquePtr<EVP_PKEY> pkey = LoadExampleRSAKey();
  bssl::ScopedEVP_MD_CTX md_ctx;
  ASSERT_TRUE(pkey);
  ASSERT_TRUE(
      EVP_DigestVerifyInit(md_ctx.get(), NULL, EVP_sha256(), NULL, pkey.get()));
  ASSERT_TRUE(EVP_DigestVerifyUpdate(md_ctx.get(), kMsg, sizeof(kMsg)));
  ASSERT_TRUE(
      EVP_DigestVerifyFinal(md_ctx.get(), kSignature, sizeof(kSignature)));
}

TEST(EVPExtraTest, VerifyRecover) {
  bssl::UniquePtr<EVP_PKEY> pkey = LoadExampleRSAKey();
  ASSERT_TRUE(pkey);
  bssl::UniquePtr<RSA> rsa(EVP_PKEY_get1_RSA(pkey.get()));
  ASSERT_TRUE(rsa);

  const uint8_t kDummyHash[32] = {0};
  uint8_t sig[2048/8];
  unsigned sig_len = sizeof(sig);
  ASSERT_TRUE(RSA_sign(NID_sha256, kDummyHash, sizeof(kDummyHash), sig,
                       &sig_len, rsa.get()));

  size_t out_len;
  bssl::UniquePtr<EVP_PKEY_CTX> ctx(EVP_PKEY_CTX_new(pkey.get(), nullptr));
  ASSERT_TRUE(EVP_PKEY_verify_recover_init(ctx.get()));
  ASSERT_TRUE(EVP_PKEY_CTX_set_rsa_padding(ctx.get(), RSA_PKCS1_PADDING));
  ASSERT_TRUE(EVP_PKEY_CTX_set_signature_md(ctx.get(), EVP_sha256()));
  ASSERT_TRUE(
      EVP_PKEY_verify_recover(ctx.get(), nullptr, &out_len, sig, sig_len));

  std::vector<uint8_t> recovered;
  recovered.resize(out_len);

  ASSERT_TRUE(EVP_PKEY_verify_recover(ctx.get(), recovered.data(), &out_len,
                                      sig, sig_len));
  EXPECT_EQ(Bytes(kDummyHash), Bytes(recovered.data(), out_len));

  out_len = recovered.size();
  ASSERT_TRUE(EVP_PKEY_CTX_set_signature_md(ctx.get(), nullptr));
  ASSERT_TRUE(EVP_PKEY_verify_recover(ctx.get(), recovered.data(), &out_len,
                                      sig, sig_len));

  // The size of a SHA-256 hash plus PKCS#1 v1.5 ASN.1 stuff happens to be 51
  // bytes.
  EXPECT_EQ(51u, out_len);
}

static void TestValidPrivateKey(const uint8_t *input, size_t input_len,
                                int expected_id) {
  const uint8_t *p = input;
  bssl::UniquePtr<EVP_PKEY> pkey(d2i_AutoPrivateKey(NULL, &p, input_len));
  ASSERT_TRUE(pkey);
  EXPECT_EQ(input + input_len, p);
  EXPECT_EQ(expected_id, EVP_PKEY_id(pkey.get()));
}

TEST(EVPExtraTest, d2i_AutoPrivateKey) {
  TestValidPrivateKey(kExampleRSAKeyDER, sizeof(kExampleRSAKeyDER),
                      EVP_PKEY_RSA);
  TestValidPrivateKey(kExampleRSAKeyPKCS8, sizeof(kExampleRSAKeyPKCS8),
                      EVP_PKEY_RSA);
  TestValidPrivateKey(kExampleECKeyDER, sizeof(kExampleECKeyDER), EVP_PKEY_EC);
  TestValidPrivateKey(kExampleECKeyPKCS8, sizeof(kExampleECKeyPKCS8),
                      EVP_PKEY_EC);
  TestValidPrivateKey(kExampleECKeySpecifiedCurvePKCS8,
                      sizeof(kExampleECKeySpecifiedCurvePKCS8), EVP_PKEY_EC);
  TestValidPrivateKey(kExampleDSAKeyDER, sizeof(kExampleDSAKeyDER),
                      EVP_PKEY_DSA);

  const uint8_t *p = kInvalidPrivateKey;
  bssl::UniquePtr<EVP_PKEY> pkey(
      d2i_AutoPrivateKey(NULL, &p, sizeof(kInvalidPrivateKey)));
  EXPECT_FALSE(pkey) << "Parsed invalid private key";
  ERR_clear_error();
}

// Tests loading a bad key in PKCS8 format.
TEST(EVPExtraTest, BadECKey) {
  const uint8_t *derp = kExampleBadECKeyDER;
  bssl::UniquePtr<PKCS8_PRIV_KEY_INFO> p8inf(
      d2i_PKCS8_PRIV_KEY_INFO(NULL, &derp, sizeof(kExampleBadECKeyDER)));
  ASSERT_TRUE(p8inf);
  EXPECT_EQ(kExampleBadECKeyDER + sizeof(kExampleBadECKeyDER), derp);

  bssl::UniquePtr<EVP_PKEY> pkey(EVP_PKCS82PKEY(p8inf.get()));
  ASSERT_FALSE(pkey) << "Imported invalid EC key";
  ERR_clear_error();
}

// Tests |EVP_marshal_public_key| on an empty key.
TEST(EVPExtraTest, MarshalEmptyPublicKey) {
  bssl::UniquePtr<EVP_PKEY> empty(EVP_PKEY_new());
  ASSERT_TRUE(empty);

  bssl::ScopedCBB cbb;
  EXPECT_FALSE(EVP_marshal_public_key(cbb.get(), empty.get()))
      << "Marshalled empty public key.";
  EXPECT_EQ(EVP_R_UNSUPPORTED_ALGORITHM, ERR_GET_REASON(ERR_peek_last_error()));
}

static bssl::UniquePtr<EVP_PKEY> ParsePrivateKey(int type, const uint8_t *in,
                                                 size_t len) {
  const uint8_t *ptr = in;
  bssl::UniquePtr<EVP_PKEY> pkey(d2i_PrivateKey(type, nullptr, &ptr, len));
  if (!pkey) {
    return nullptr;
  }

  EXPECT_EQ(in + len, ptr);
  return pkey;
}

TEST(EVPExtraTest, d2i_PrivateKey) {
  EXPECT_TRUE(ParsePrivateKey(EVP_PKEY_RSA, kExampleRSAKeyDER,
                              sizeof(kExampleRSAKeyDER)));
  EXPECT_TRUE(ParsePrivateKey(EVP_PKEY_DSA, kExampleDSAKeyDER,
                              sizeof(kExampleDSAKeyDER)));
  EXPECT_TRUE(ParsePrivateKey(EVP_PKEY_RSA, kExampleRSAKeyPKCS8,
                              sizeof(kExampleRSAKeyPKCS8)));
  EXPECT_TRUE(
      ParsePrivateKey(EVP_PKEY_EC, kExampleECKeyDER, sizeof(kExampleECKeyDER)));

  EXPECT_FALSE(ParsePrivateKey(EVP_PKEY_EC, kExampleBadECKeyDER,
                               sizeof(kExampleBadECKeyDER)));
  ERR_clear_error();

  // Copy the input into a |malloc|'d vector to flag memory errors.
  std::vector<uint8_t> copy(
      kExampleBadECKeyDER2,
      kExampleBadECKeyDER2 + sizeof(kExampleBadECKeyDER2));
  EXPECT_FALSE(ParsePrivateKey(EVP_PKEY_EC, copy.data(), copy.size()));
  ERR_clear_error();

  // Test that an RSA key may not be imported as an EC key.
  EXPECT_FALSE(ParsePrivateKey(EVP_PKEY_EC, kExampleRSAKeyPKCS8,
                               sizeof(kExampleRSAKeyPKCS8)));
  ERR_clear_error();
}

TEST(EVPExtraTest, Ed25519) {
  static const uint8_t kPublicKey[32] = {
      0xd7, 0x5a, 0x98, 0x01, 0x82, 0xb1, 0x0a, 0xb7, 0xd5, 0x4b, 0xfe,
      0xd3, 0xc9, 0x64, 0x07, 0x3a, 0x0e, 0xe1, 0x72, 0xf3, 0xda, 0xa6,
      0x23, 0x25, 0xaf, 0x02, 0x1a, 0x68, 0xf7, 0x07, 0x51, 0x1a,
  };

  static const uint8_t kPublicKeySPKI[] = {
      0x30, 0x2a, 0x30, 0x05, 0x06, 0x03, 0x2b, 0x65, 0x70, 0x03, 0x21,
      0x00, 0xd7, 0x5a, 0x98, 0x01, 0x82, 0xb1, 0x0a, 0xb7, 0xd5, 0x4b,
      0xfe, 0xd3, 0xc9, 0x64, 0x07, 0x3a, 0x0e, 0xe1, 0x72, 0xf3, 0xda,
      0xa6, 0x23, 0x25, 0xaf, 0x02, 0x1a, 0x68, 0xf7, 0x07, 0x51, 0x1a,
  };

  static const uint8_t kPrivateKey[64] = {
      0x9d, 0x61, 0xb1, 0x9d, 0xef, 0xfd, 0x5a, 0x60, 0xba, 0x84, 0x4a,
      0xf4, 0x92, 0xec, 0x2c, 0xc4, 0x44, 0x49, 0xc5, 0x69, 0x7b, 0x32,
      0x69, 0x19, 0x70, 0x3b, 0xac, 0x03, 0x1c, 0xae, 0x7f, 0x60, 0xd7,
      0x5a, 0x98, 0x01, 0x82, 0xb1, 0x0a, 0xb7, 0xd5, 0x4b, 0xfe, 0xd3,
      0xc9, 0x64, 0x07, 0x3a, 0x0e, 0xe1, 0x72, 0xf3, 0xda, 0xa6, 0x23,
      0x25, 0xaf, 0x02, 0x1a, 0x68, 0xf7, 0x07, 0x51, 0x1a,
  };

  static const uint8_t kPrivateKeyPKCS8[] = {
      0x30, 0x2e, 0x02, 0x01, 0x00, 0x30, 0x05, 0x06, 0x03, 0x2b, 0x65, 0x70,
      0x04, 0x22, 0x04, 0x20, 0x9d, 0x61, 0xb1, 0x9d, 0xef, 0xfd, 0x5a, 0x60,
      0xba, 0x84, 0x4a, 0xf4, 0x92, 0xec, 0x2c, 0xc4, 0x44, 0x49, 0xc5, 0x69,
      0x7b, 0x32, 0x69, 0x19, 0x70, 0x3b, 0xac, 0x03, 0x1c, 0xae, 0x7f, 0x60,
  };

  // Create a public key.
  bssl::UniquePtr<EVP_PKEY> pubkey(EVP_PKEY_new_ed25519_public(kPublicKey));
  ASSERT_TRUE(pubkey);
  EXPECT_EQ(EVP_PKEY_ED25519, EVP_PKEY_id(pubkey.get()));

  // The public key must encode properly.
  bssl::ScopedCBB cbb;
  uint8_t *der;
  size_t der_len;
  ASSERT_TRUE(CBB_init(cbb.get(), 0));
  ASSERT_TRUE(EVP_marshal_public_key(cbb.get(), pubkey.get()));
  ASSERT_TRUE(CBB_finish(cbb.get(), &der, &der_len));
  bssl::UniquePtr<uint8_t> free_der(der);
  EXPECT_EQ(Bytes(kPublicKeySPKI), Bytes(der, der_len));

  // The public key must gracefully fail to encode as a private key.
  ASSERT_TRUE(CBB_init(cbb.get(), 0));
  EXPECT_FALSE(EVP_marshal_private_key(cbb.get(), pubkey.get()));
  uint32_t err = ERR_get_error();
  EXPECT_EQ(ERR_LIB_EVP, ERR_GET_LIB(err));
  EXPECT_EQ(EVP_R_NOT_A_PRIVATE_KEY, ERR_GET_REASON(err));
  cbb.Reset();

  // Create a private key.
  bssl::UniquePtr<EVP_PKEY> privkey(EVP_PKEY_new_ed25519_private(kPrivateKey));
  ASSERT_TRUE(privkey);
  EXPECT_EQ(EVP_PKEY_ED25519, EVP_PKEY_id(privkey.get()));

  // The public key must encode from the private key.
  ASSERT_TRUE(CBB_init(cbb.get(), 0));
  ASSERT_TRUE(EVP_marshal_public_key(cbb.get(), privkey.get()));
  ASSERT_TRUE(CBB_finish(cbb.get(), &der, &der_len));
  free_der.reset(der);
  EXPECT_EQ(Bytes(kPublicKeySPKI), Bytes(der, der_len));

  // The private key must encode properly.
  ASSERT_TRUE(CBB_init(cbb.get(), 0));
  ASSERT_TRUE(EVP_marshal_private_key(cbb.get(), privkey.get()));
  ASSERT_TRUE(CBB_finish(cbb.get(), &der, &der_len));
  free_der.reset(der);
  EXPECT_EQ(Bytes(kPrivateKeyPKCS8), Bytes(der, der_len));

  // Test EVP_PKEY_cmp.
  EXPECT_EQ(1, EVP_PKEY_cmp(pubkey.get(), privkey.get()));

  static const uint8_t kZeros[32] = {0};
  bssl::UniquePtr<EVP_PKEY> pubkey2(EVP_PKEY_new_ed25519_public(kZeros));
  ASSERT_TRUE(pubkey2);
  EXPECT_EQ(0, EVP_PKEY_cmp(pubkey.get(), pubkey2.get()));
  EXPECT_EQ(0, EVP_PKEY_cmp(privkey.get(), pubkey2.get()));

  // Ed25519 may not be used streaming.
  bssl::ScopedEVP_MD_CTX ctx;
  ASSERT_TRUE(
      EVP_DigestSignInit(ctx.get(), nullptr, nullptr, nullptr, privkey.get()));
  EXPECT_FALSE(EVP_DigestSignUpdate(ctx.get(), nullptr, 0));
  size_t len;
  EXPECT_FALSE(EVP_DigestSignFinal(ctx.get(), nullptr, &len));
  ERR_clear_error();

  ctx.Reset();
  ASSERT_TRUE(EVP_DigestVerifyInit(ctx.get(), nullptr, nullptr, nullptr,
                                   privkey.get()));
  EXPECT_FALSE(EVP_DigestVerifyUpdate(ctx.get(), nullptr, 0));
  EXPECT_FALSE(EVP_DigestVerifyFinal(ctx.get(), nullptr, 0));
  ERR_clear_error();
}
