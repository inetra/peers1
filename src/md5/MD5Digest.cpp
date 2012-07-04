#include "MD5Digest.h"

string MD5Digest::digestAsString() {
	BYTE *d = digest();
	char buff[33];
	for (int i = 0; i < 16; ++i) {
		sprintf(buff + i * 2, "%02x", d[i]);
	}
	return buff;
}
