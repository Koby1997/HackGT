/*
 * AIX ssha cracker patch for JtR. Hacked together during April of 2013 by Dhiru
 * Kholia <dhiru at openwall.com> and magnum.
 *
 * Thanks to atom (of hashcat project) and philsmd for discovering and
 * publishing the details of various AIX hashing algorithms.
 *
 * This software is Copyright (c) 2013 Dhiru Kholia <dhiru at openwall.com> and
 * magnum, and it is hereby released to the general public under the following
 * terms:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted.
 */

#if FMT_EXTERNS_H
extern struct fmt_main fmt_aixssha1;
extern struct fmt_main fmt_aixssha256;
extern struct fmt_main fmt_aixssha512;
#elif FMT_REGISTERS_H
john_register_one(&fmt_aixssha1);
john_register_one(&fmt_aixssha256);
john_register_one(&fmt_aixssha512);
#else

#include <string.h>

#ifdef _OPENMP
#include <omp.h>
#ifndef OMP_SCALE
#define OMP_SCALE               8 // Tuned on i7 w/HT for SHA-256
#endif
#endif

#include "arch.h"
#include "misc.h"
#include "common.h"
#include "formats.h"
#include "params.h"
#include "options.h"
#include "pbkdf2_hmac_sha1.h"
#include "pbkdf2_hmac_sha256.h"
#include "pbkdf2_hmac_sha512.h"
#include "memdbg.h"

#define FORMAT_LABEL_SHA1       "aix-ssha1"
#define FORMAT_LABEL_SHA256     "aix-ssha256"
#define FORMAT_LABEL_SHA512     "aix-ssha512"
#define FORMAT_NAME_SHA1        "AIX LPA {ssha1}"
#define FORMAT_NAME_SHA256      "AIX LPA {ssha256}"
#define FORMAT_NAME_SHA512      "AIX LPA {ssha512}"

#define FORMAT_TAG1             "{ssha1}"
#define FORMAT_TAG256           "{ssha256}"
#define FORMAT_TAG512           "{ssha512}"
#define FORMAT_TAG1_LEN         (sizeof(FORMAT_TAG1)-1)
#define FORMAT_TAG256_LEN       (sizeof(FORMAT_TAG256)-1)
#define FORMAT_TAG512_LEN       (sizeof(FORMAT_TAG512)-1)

#ifdef SIMD_COEF_32
#define ALGORITHM_NAME_SHA1     "PBKDF2-SHA1 " SHA1_ALGORITHM_NAME
#else
#define ALGORITHM_NAME_SHA1     "PBKDF2-SHA1 32/" ARCH_BITS_STR
#endif
#ifdef SIMD_COEF_32
#define ALGORITHM_NAME_SHA256   "PBKDF2-SHA256 " SHA256_ALGORITHM_NAME
#else
#define ALGORITHM_NAME_SHA256   "PBKDF2-SHA256 32/" ARCH_BITS_STR
#endif
#ifdef SIMD_COEF_64
#define ALGORITHM_NAME_SHA512   "PBKDF2-SHA512 " SHA512_ALGORITHM_NAME
#else
#define ALGORITHM_NAME_SHA512   "PBKDF2-SHA512 32/" ARCH_BITS_STR
#endif
#define BENCHMARK_COMMENT       ""
#define BENCHMARK_LENGTH        -1
#define PLAINTEXT_LENGTH        125 /* actual max in AIX is 255 */
#define BINARY_SIZE             20
#define BINARY_ALIGN            4
#define CMP_SIZE                BINARY_SIZE - 2
#define LARGEST_BINARY_SIZE     64
#define MAX_SALT_SIZE           24
#define SALT_SIZE               sizeof(struct custom_salt)
#define SALT_ALIGN              4
#define MIN_KEYS_PER_CRYPT      1
#define MAX_KEYS_PER_CRYPT      1

static struct fmt_tests aixssha_tests1[] = {
	{"{ssha1}06$T6numGi8BRLzTYnF$AdXq1t6baevg9/cu5QBBk8Xg.se", "whatdoyouwantfornothing$$$$$$"},
	{"{ssha1}06$6cZ2YrFYwVQPAVNb$1agAljwERjlin9RxFxzKl.E0.sJ", "gentoo=>meh"},
	/* Full 125 byte PW (longest JtR will handle).  generated by pass_gen.pl */
	{"{ssha1}06$uOYCzfO5dt0EdnwG$CK81ljQknzEAcfwg97cocEwz.gv", "12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345"},
	{NULL}
};

static struct fmt_tests aixssha_tests256[] = {
	{"{ssha256}06$YPhynOx/iJQaJOeV$EXQbOSYZftEo3k01uoanAbA7jEKZRUU9LCCs/tyU.wG", "verylongbutnotverystrongpassword"},
	{"{ssha256}06$5lsi4pETf/0p/12k$xACBftDMh30RqgrM5Sppl.Txgho41u0oPoD21E1b.QT", "I<3JtR"},
	/* Full 125 byte PW (longest JtR will handle).  generated by pass_gen.pl */
	{"{ssha256}06$qcXPTOQzDAqZuiHc$pS/1HC4uI5jIERNerX8.Zd0G/gDepZuqR7S5WHEn.AW", "12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345"},
	{NULL}
};
static struct fmt_tests aixssha_tests512[] = {
	{"{ssha512}06$y2/.O4drNJd3ecgJ$DhNk3sS28lkIo7XZaXWSkFOIdP2Zsd9DIKdYDSuSU5tsnl29Q7xTc3f64eAGMpcMJCVp/SXZ4Xgx3jlHVIOr..", "solarisalwaysbusyitseems"},
	{"{ssha512}06$Dz/dDr1qa8JJm0UB$DFNu2y8US18fW37ht8WRiwhSeOqAMJTJ6mLDW03D/SeQpdI50GJMYb1fBog5/ZU3oM9qsSr9w6u22.OjjufV..", "idontbelievethatyourpasswordislongerthanthisone"},
	/* hash posted on john-users */
	{"{ssha512}06$................$0egLaF88SUk6GAFIMN/vTwa/IYB.KlubYmjiaWvmQ975vHvgC3rf0I6ZYzgyUiQftS8qs7ULLQpRLrA3LA....", "44"},
	{"{ssha512}06$aXayEJGxA02Bl4d2$TWfWx34oD.UjrS/Qtco6Ij2XPY1CPYJfdk3CcxEjnMZvQw2p5obHYH7SI2wxcJgaS9.S9Hz948R.GdGwsvR...", "test"},
	/* http://www.ibmsystemsmag.com/aix/administrator/security/password_hash/?page=2 <== partially corrupted hash? */
	{"{ssha512}06$otYx2eSXx.OkEY4F$No5ZvSfhYuB1MSkBhhcKJIjS0.q//awdkcZwF9/TXi3EnL6QeronmS0jCc3P2aEV9WLi5arzN1YjVwkx8bng..", "colorado"},
	/* Full 125 byte PW (longest JtR will handle).  generated by pass_gen.pl */
	{"{ssha512}06$w6THk2lJbkqW0rXv$yH6VWp3X9ad2l8nhYi22lrrmWskXvEU.PONbWUAZHrjhgQjdU83jtRnYmpRZIJzTVC3RFcoqpbtd63n/UdlS..", "12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345"},
	{NULL}
};

static char (*saved_key)[PLAINTEXT_LENGTH + 1];
static uint32_t (*crypt_out)[BINARY_SIZE / sizeof(uint32_t)];

static struct custom_salt {
	int iterations;
	int type;
	unsigned char salt[MAX_SALT_SIZE + 1];
} *cur_salt;

static void init(struct fmt_main *self)
{
#ifdef _OPENMP
	omp_autotune(self, OMP_SCALE);
#endif
	saved_key = mem_calloc_align(sizeof(*saved_key),
			self->params.max_keys_per_crypt, MEM_ALIGN_WORD);
	crypt_out = mem_calloc_align(sizeof(*crypt_out),
			self->params.max_keys_per_crypt, MEM_ALIGN_WORD);
}

static void done(void)
{
	MEM_FREE(crypt_out);
	MEM_FREE(saved_key);
}
static inline int valid_common(char *ciphertext, struct fmt_main *self, int b64len, char *sig, int siglen)
{
	char *p = ciphertext;
	int len;

	if (!strncmp(p, sig, siglen))
		p += siglen;
	else
		return 0;

	len = strspn(p, DIGITCHARS); /* iterations, exactly two digits */
	if (len != 2 || atoi(p) > 31)  /* actual range is 4..31 */
		return 0;
	p += 2;
	if (*p++ != '$')
		return 0;

	len = strspn(p, BASE64_CRYPT); /* salt, 8..24 base64 chars */
	if (len < 8 || len > MAX_SALT_SIZE)
		return 0;
	p += len;
	if (*p++ != '$')
		return 0;
	len = strspn(p, BASE64_CRYPT); /* hash */
	if (len != b64len)
		return 0;
	if (p[len] != 0) /* nothing more allowed */
		return 0;

	return 1;
}

static int valid_sha1(char *ciphertext, struct fmt_main *self) {
	return valid_common(ciphertext, self, 27, FORMAT_TAG1, FORMAT_TAG1_LEN);
}
static int valid_sha256(char *ciphertext, struct fmt_main *self) {
	return valid_common(ciphertext, self, 43, FORMAT_TAG256, FORMAT_TAG256_LEN);
}
static int valid_sha512(char *ciphertext, struct fmt_main *self) {
	return valid_common(ciphertext, self, 86, FORMAT_TAG512, FORMAT_TAG512_LEN);
}

static void *get_salt(char *ciphertext)
{
	char *ctcopy = strdup(ciphertext);
	char *keeptr = ctcopy;
	char *p;
	static struct custom_salt cs;
	keeptr = ctcopy;

	memset(&cs, 0, sizeof(cs));
	if ((strncmp(ciphertext, FORMAT_TAG1, FORMAT_TAG1_LEN) == 0)) {
		cs.type = 1;
		ctcopy += FORMAT_TAG1_LEN;
	} else if ((strncmp(ciphertext, FORMAT_TAG256, FORMAT_TAG256_LEN) == 0)) {
		cs.type = 256;
		ctcopy += FORMAT_TAG256_LEN;
	} else {
		cs.type = 512;
		ctcopy += FORMAT_TAG512_LEN;
	}

	p = strtokm(ctcopy, "$");
	cs.iterations = 1 << atoi(p);
	p = strtokm(NULL, "$");
	strncpy((char*)cs.salt, p, 17);

	MEM_FREE(keeptr);
	return (void *)&cs;
}

#define TO_BINARY(b1, b2, b3) {	  \
	value = (uint32_t)atoi64[ARCH_INDEX(pos[0])] | \
		((uint32_t)atoi64[ARCH_INDEX(pos[1])] << 6) | \
		((uint32_t)atoi64[ARCH_INDEX(pos[2])] << 12) | \
		((uint32_t)atoi64[ARCH_INDEX(pos[3])] << 18); \
	pos += 4; \
	out.c[b1] = value >> 16; \
	out.c[b2] = value >> 8; \
	out.c[b3] = value; }

static void *get_binary(char *ciphertext)
{
	static union {
		unsigned char c[LARGEST_BINARY_SIZE+3];
		uint64_t dummy;
	} out;
	uint32_t value;
	char *pos = strrchr(ciphertext, '$') + 1;
	int len = strlen(pos);
	int i;

	memset(&out, 0, sizeof(out));
	for (i = 0; i < len/4*3; i += 3)
		TO_BINARY(i, i + 1, i + 2);

	if (len % 3 == 1) {
		value = (uint32_t)atoi64[ARCH_INDEX(pos[0])] |
			((uint32_t)atoi64[ARCH_INDEX(pos[1])] << 6);
		out.c[i] = value;
	} else if (len % 3 == 2) { /* sha-1, sha-256 */
		value = (uint32_t)atoi64[ARCH_INDEX(pos[0])] |
			((uint32_t)atoi64[ARCH_INDEX(pos[1])] << 6) |
			((uint32_t)atoi64[ARCH_INDEX(pos[2])] << 12);
		out.c[i++] = value >> 8;
		out.c[i++] = value;
	}
	return (void *)out.c;
}

#define COMMON_GET_HASH_VAR crypt_out
#include "common-get-hash.h"

static void set_salt(void *salt)
{
	cur_salt = (struct custom_salt *)salt;
}

static int crypt_all(int *pcount, struct db_salt *salt)
{
	const int count = *pcount;
	int inc=1, index = 0;

	switch(cur_salt->type) {
	case 1:
#ifdef SSE_GROUP_SZ_SHA1
		inc = SSE_GROUP_SZ_SHA1;
#endif
		break;
	case 256:
#ifdef SSE_GROUP_SZ_SHA256
		inc = SSE_GROUP_SZ_SHA256;
#endif
		break;
	default:
#ifdef SSE_GROUP_SZ_SHA512
		inc = SSE_GROUP_SZ_SHA512;
#endif
		break;
	}

#ifdef _OPENMP
#pragma omp parallel for
#endif
	for (index = 0; index < count; index += inc)
	{
		int j = index;
		while (j < index + inc) {
			if (cur_salt->type == 1) {
#ifdef SSE_GROUP_SZ_SHA1
				int lens[SSE_GROUP_SZ_SHA1], i;
				unsigned char *pin[SSE_GROUP_SZ_SHA1];
				union {
					uint32_t *pout[SSE_GROUP_SZ_SHA1];
					unsigned char *poutc;
				} x;
				for (i = 0; i < SSE_GROUP_SZ_SHA1; ++i) {
					lens[i] = strlen(saved_key[j]);
					pin[i] = (unsigned char*)(saved_key[j]);
					x.pout[i] = crypt_out[j];
					++j;
				}
				pbkdf2_sha1_sse((const unsigned char **)pin, lens, cur_salt->salt, strlen((char*)cur_salt->salt), cur_salt->iterations, &(x.poutc), BINARY_SIZE, 0);
#else
				pbkdf2_sha1((const unsigned char*)(saved_key[j]), strlen(saved_key[j]),
					cur_salt->salt, strlen((char*)cur_salt->salt),
					cur_salt->iterations, (unsigned char*)crypt_out[j], BINARY_SIZE, 0);
				++j;
#endif
			}
			else if (cur_salt->type == 256) {
#ifdef SSE_GROUP_SZ_SHA256
				int lens[SSE_GROUP_SZ_SHA256], i;
				unsigned char *pin[SSE_GROUP_SZ_SHA256];
				union {
					uint32_t *pout[SSE_GROUP_SZ_SHA256];
					unsigned char *poutc;
				} x;
				for (i = 0; i < SSE_GROUP_SZ_SHA256; ++i) {
					lens[i] = strlen(saved_key[j]);
					pin[i] = (unsigned char*)saved_key[j];
					x.pout[i] = crypt_out[j];
					++j;
				}
				pbkdf2_sha256_sse((const unsigned char **)pin, lens, cur_salt->salt, strlen((char*)cur_salt->salt), cur_salt->iterations, &(x.poutc), BINARY_SIZE, 0);
#else
				pbkdf2_sha256((const unsigned char*)(saved_key[j]), strlen(saved_key[j]),
					cur_salt->salt, strlen((char*)cur_salt->salt),
					cur_salt->iterations, (unsigned char*)crypt_out[j], BINARY_SIZE, 0);
				++j;
#endif
			}
			else {
#ifdef SSE_GROUP_SZ_SHA512
				int lens[SSE_GROUP_SZ_SHA512], i;
				unsigned char *pin[SSE_GROUP_SZ_SHA512];
				union {
					uint32_t *pout[SSE_GROUP_SZ_SHA512];
					unsigned char *poutc;
				} x;
				for (i = 0; i < SSE_GROUP_SZ_SHA512; ++i) {
					lens[i] = strlen(saved_key[j]);
					pin[i] = (unsigned char*)saved_key[j];
					x.pout[i] = crypt_out[j];
					++j;
				}
				pbkdf2_sha512_sse((const unsigned char **)pin, lens, cur_salt->salt, strlen((char*)cur_salt->salt), cur_salt->iterations, &(x.poutc), BINARY_SIZE, 0);
#else
				pbkdf2_sha512((const unsigned char*)(saved_key[j]), strlen(saved_key[j]),
					cur_salt->salt, strlen((char*)cur_salt->salt),
					cur_salt->iterations, (unsigned char*)crypt_out[j], BINARY_SIZE, 0);
				++j;
#endif
			}
		}
	}
	return count;
}

static int cmp_all(void *binary, int count)
{
	int index;

	// dump_stuff_msg("\nbinary   ", binary, CMP_SIZE);
	for (index = 0; index < count; index++) {
		// dump_stuff_msg("crypt_out", crypt_out[index], CMP_SIZE);
		if (!memcmp(binary, crypt_out[index], CMP_SIZE-2))
			return 1;
	}
	return 0;
}

static int cmp_one(void *binary, int index)
{
	return !memcmp(binary, crypt_out[index], CMP_SIZE-2);
}

static int cmp_exact(char *source, int index)
{
	return 1;
}

static void aixssha_set_key(char *key, int index)
{
	strnzcpy(saved_key[index], key, sizeof(*saved_key));
}

static char *get_key(int index)
{
	return saved_key[index];
}

/* report iteration count as tunable cost value */
static unsigned int aixssha_iteration_count(void *salt)
{
	struct custom_salt *my_salt;

	my_salt = salt;
	return (unsigned int) my_salt->iterations;
}

struct fmt_main fmt_aixssha1 = {
	{
		FORMAT_LABEL_SHA1,
		FORMAT_NAME_SHA1,
		ALGORITHM_NAME_SHA1,
		BENCHMARK_COMMENT,
		BENCHMARK_LENGTH,
		0,
		PLAINTEXT_LENGTH,
		BINARY_SIZE,
		BINARY_ALIGN,
		SALT_SIZE,
		SALT_ALIGN,
#ifdef SIMD_COEF_32
		SSE_GROUP_SZ_SHA1,
		SSE_GROUP_SZ_SHA1,
#else
		MIN_KEYS_PER_CRYPT,
		MAX_KEYS_PER_CRYPT,
#endif
		FMT_CASE | FMT_8_BIT | FMT_OMP,
		{
			"iteration count",
		},
		{ FORMAT_TAG1 },
		aixssha_tests1
	}, {
		init,
		done,
		fmt_default_reset,
		fmt_default_prepare,
		valid_sha1,
		fmt_default_split,
		get_binary,
		get_salt,
		{
			aixssha_iteration_count,
		},
		fmt_default_source,
		{
			fmt_default_binary_hash_0,
			fmt_default_binary_hash_1,
			fmt_default_binary_hash_2,
			fmt_default_binary_hash_3,
			fmt_default_binary_hash_4,
			fmt_default_binary_hash_5,
			fmt_default_binary_hash_6
		},
		fmt_default_salt_hash,
		NULL,
		set_salt,
		aixssha_set_key,
		get_key,
		fmt_default_clear_keys,
		crypt_all,
		{
#define COMMON_GET_HASH_LINK
#include "common-get-hash.h"
		},
		cmp_all,
		cmp_one,
		cmp_exact
	}
};

struct fmt_main fmt_aixssha256 = {
	{
		FORMAT_LABEL_SHA256,
		FORMAT_NAME_SHA256,
		ALGORITHM_NAME_SHA256,
		BENCHMARK_COMMENT,
		BENCHMARK_LENGTH,
		0,
		PLAINTEXT_LENGTH,
		BINARY_SIZE,
		BINARY_ALIGN,
		SALT_SIZE,
		SALT_ALIGN,
#ifdef SIMD_COEF_32
		SSE_GROUP_SZ_SHA256,
		SSE_GROUP_SZ_SHA256,
#else
		MIN_KEYS_PER_CRYPT,
		MAX_KEYS_PER_CRYPT,
#endif
		FMT_CASE | FMT_8_BIT | FMT_OMP,
		{
			"iteration count",
		},
		{ FORMAT_TAG256 },
		aixssha_tests256
	}, {
		init,
		done,
		fmt_default_reset,
		fmt_default_prepare,
		valid_sha256,
		fmt_default_split,
		get_binary,
		get_salt,
		{
			aixssha_iteration_count,
		},
		fmt_default_source,
		{
			fmt_default_binary_hash_0,
			fmt_default_binary_hash_1,
			fmt_default_binary_hash_2,
			fmt_default_binary_hash_3,
			fmt_default_binary_hash_4,
			fmt_default_binary_hash_5,
			fmt_default_binary_hash_6
		},
		fmt_default_salt_hash,
		NULL,
		set_salt,
		aixssha_set_key,
		get_key,
		fmt_default_clear_keys,
		crypt_all,
		{
#define COMMON_GET_HASH_LINK
#include "common-get-hash.h"
		},
		cmp_all,
		cmp_one,
		cmp_exact
	}
};

struct fmt_main fmt_aixssha512 = {
	{
		FORMAT_LABEL_SHA512,
		FORMAT_NAME_SHA512,
		ALGORITHM_NAME_SHA512,
		BENCHMARK_COMMENT,
		BENCHMARK_LENGTH,
		0,
		PLAINTEXT_LENGTH,
		BINARY_SIZE,
		BINARY_ALIGN,
		SALT_SIZE,
		SALT_ALIGN,
#ifdef SIMD_COEF_64
		SSE_GROUP_SZ_SHA512,
		SSE_GROUP_SZ_SHA512,
#else
		MIN_KEYS_PER_CRYPT,
		MAX_KEYS_PER_CRYPT,
#endif
		FMT_CASE | FMT_8_BIT | FMT_OMP,
		{
			"iteration count",
		},
		{ FORMAT_TAG512 },
		aixssha_tests512
	}, {
		init,
		done,
		fmt_default_reset,
		fmt_default_prepare,
		valid_sha512,
		fmt_default_split,
		get_binary,
		get_salt,
		{
			aixssha_iteration_count,
		},
		fmt_default_source,
		{
			fmt_default_binary_hash_0,
			fmt_default_binary_hash_1,
			fmt_default_binary_hash_2,
			fmt_default_binary_hash_3,
			fmt_default_binary_hash_4,
			fmt_default_binary_hash_5,
			fmt_default_binary_hash_6
		},
		fmt_default_salt_hash,
		NULL,
		set_salt,
		aixssha_set_key,
		get_key,
		fmt_default_clear_keys,
		crypt_all,
		{
#define COMMON_GET_HASH_LINK
#include "common-get-hash.h"
		},
		cmp_all,
		cmp_one,
		cmp_exact
	}
};

#endif /* plugin stanza */
