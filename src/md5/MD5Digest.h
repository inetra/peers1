#pragma once
#include "md5.h"
#include "../client/stdinc.h"

class MD5Digest {
private:
  bool digestReady;
  md5_state_s state;
  md5_byte_t m_digest[16];
public:
  MD5Digest():digestReady(false) {
    md5_init(&state);
  }
  void update(const md5_byte_t* data, const size_t length) {
    if (digestReady) {
      digestReady = false;
    }
    md5_append(&state, data, (int) length);
  }
  md5_byte_t* digest() {
    if (!digestReady) {
      md5_finish(&state, m_digest);
      digestReady = true;
    }
    return m_digest;
  }
  string digestAsString();
};
