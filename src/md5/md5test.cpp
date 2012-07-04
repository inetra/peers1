#include <stdio.h>
#include <string.h>
#include "md5.h"
#include "MD5Digest.h"

int __cdecl main()
{
    static const char *const test[7] = {
	"", /*d41d8cd98f00b204e9800998ecf8427e*/
	"a", /*0cc175b9c0f1b6a831c399e269772661*/
	"abc", /*900150983cd24fb0d6963f7d28e17f72*/
	"message digest", /*f96b697d7cb7938d525a2f31aaf161d0*/
	"abcdefghijklmnopqrstuvwxyz", /*c3fcd3d76192e4007dfb496cca67e13b*/
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
				/*d174ab98d277d9f5a5611c2c9f419d9f*/
	"12345678901234567890123456789012345678901234567890123456789012345678901234567890" /*57edf4a22be3c955ac49da2e2107b67a*/
    };
    int i;

    for (i = 0; i < 7; ++i) {
	md5_state_t state;
	md5_byte_t digest[16];

	md5_init(&state);
	md5_append(&state, (const md5_byte_t *)test[i], (int) strlen(test[i]));
	md5_finish(&state, digest);
	printf("MD5 (\"%s\") = ", test[i]);
	for (int di = 0; di < 16; ++di) {
	    printf("%02x", digest[di]);
	}
	printf("\n");
	MD5Digest d;
	d.update((const md5_byte_t *)test[i], strlen(test[i]));
	
	printf("MD5*(\"%s\") = ", test[i]);
	for (int di = 0; di < 16; ++di) {
	    printf("%02x", d.digest()[di]);
	}
	printf("\n");
    }
    return 0;
}
