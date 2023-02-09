#ifndef __INTEL_IMPL_H__
#define __INTEL_IMPL_H__

// add linkerage
#ifdef __cplusplus
extern "C" {
#endif

void AES_128_Key_Expansion (uint8_t *userkey, uint8_t *key);
void AES_ECB_encrypt(uint8_t *in, uint8_t *out, uint8_t *key);
void c_intel_benchmark       (uint8_t* keys, uint8_t*data, uint8_t *dataOut);
void c_intel_interleaved_4x  (uint8_t* keys, uint8_t*data, uint8_t *dataOut);



#ifdef __cplusplus
}
#endif

#endif /*__INTEL_IMPL_H__*/