#ifndef _RSI_H
#define _RSI_H
#include <stdint.h>

#define RSI_SUCCESS		0
#define RSI_ERROR_INPUT		1	
#define RSI_ERROR_STATE		2	
#define RSI_INCOMPLETE		3

#define RSI_ATTESTATION_TOKEN_CONTINUE	0xC4000195 
#define RSI_ATTESTATION_TOKEN_INIT	0xC4000194 
#define RSI_HOST_CALL			0xC4000199 
#define RSI_IPA_STATE_GET		0xC4000198 
#define RSI_IPA_STATE_SET		0xC4000197 
#define RSI_MEASUREMENT_EXTEND		0xC4000193 
#define RSI_MEASUREMENT_READ		0xC4000192 
#define RSI_REALM_CONFIG		0xC4000196 
#define RSI_VERSION			0xC4000190 

uint64_t get_rsi_version();
int rsi_get_ipa_state(uint64_t ipa);
uint64_t rsi_realm_config(uint64_t addr_frame);
int rsi_set_ipa_state(uint64_t base, uint64_t size, uint64_t ripas);
void rsi_host_call();


#endif
