#include "./crypto.h"

crypto::crypto() {
	memset(ciphertext, 0x00, BUFF_SIZE);
  const unsigned char ctxt[] = {0x0b, 0x15, 0x95, 0x9f, 0x61, 0x4f, 0xe5, 0x29, 0x8d, 0xda, 0xcf,
																0xeb, 0x69, 0xdb, 0x55, 0x0a, 0x00, 0x58, 0x58, 0xfa, 0x7c, 0x6f,
																0xd0, 0x52, 0x52, 0x53, 0x73, 0x61, 0x1b, 0x08, 0x2e, 0x03, 0xa2,
																0xd1, 0x16, 0x0c, 0x98, 0x4b, 0xf5, 0xc7, 0x43, 0x6c, 0xca, 0xd5}; 

	set_ciphertext((unsigned char *)ctxt, 44);
	set_our_key((unsigned char *)"0123456789012345",16);
	iv = (unsigned char *)"0123456789012345";
}

crypto::~crypto(){}

void crypto::clear_hash(){
	int i;

	for(i=0; i < HASH_SIZE; i++) {
		hash[i] = '\0';
	}
}

void crypto::clear_ciphertext(){
	int i;

	for(i=0; i < BUFF_SIZE; i++) {
		ciphertext[i] = '\0';
	}
	ctxt_len = 0;		// Clear the ciphertext length variable
}

void crypto::clear_decryptedtext(){
	int i;

	for(i=0; i < BUFF_SIZE; i++) {
		decryptedtext[i] = '\0';
	}
}

void crypto::clear_plaintext(){
	int i;

	for(i=0; i < BUFF_SIZE; i++) {
		plaintext[i] = '\0';
	}
}

void crypto::set_ciphertext(unsigned char *c, int clen){
	memset(ciphertext, 0x00, BUFF_SIZE);
	memcpy((char *)ciphertext, (char *)c, clen);
	ctxt_len = clen;
//	ptxt_len = 16*(ptxt_len/16)+16; //cast to 16-byte blocks
}
void crypto::set_plaintext(unsigned char *p){
	memset(plaintext, 0x00, BUFF_SIZE);
	strncpy((char *)plaintext, (char *)p, strlen((char *)p));
	ptxt_len = strlen((const char *)plaintext);
//	ptxt_len = 16*(ptxt_len/16)+16; //cast to 16-byte blocks
}
void crypto::set_our_key(unsigned char *k, int size){
	if(size%8!=0 || size>OUR_KEY_SIZE){
		printf("WEEWOOWEEWOO THAT KEY SEEMS SKETCHY!\n");
	}
	memset(key, 0x00, OUR_KEY_SIZE);
	strncpy((char*) key, (char *)k, size);
}
/***************CIPHERS*****************/
/*
Object type example:
Crypto crypto;
crypto.set_plaintext((unsigned char *)"hello");
crypto.set_key((unsigned char *)"very_secure");
crypto.set_iv((unsigned char *)"nonce");
crypto.triple_des_encrypt();
printf(crypto.get_ciphertext());
crypto.triple_des_decrypt();
printf(crypto.get_plaintext());
*/
/*
crypto::aes_ctr_encrypt()
REQUIRED MEMBER VARIABLES:
unsigned char* plaintext;
unsigned char* key;
unsigned char* iv;
*/
void crypto::aes_ctr_256_encrypt() {
	int length = 0;
	EVP_CIPHER_CTX *ctx;

	clear_ciphertext();

	ctx = EVP_CIPHER_CTX_new();

	EVP_EncryptInit_ex(ctx, EVP_aes_256_ctr(), NULL, key, iv);
	EVP_EncryptUpdate(ctx, ciphertext, &length, plaintext, ptxt_len + 1);
	ctxt_len = length;
	EVP_EncryptFinal_ex(ctx, ciphertext + length, &length);
	ctxt_len += length;
	EVP_CIPHER_CTX_free(ctx);


	printf("AES CTR 256 Initial Plaintext:\n");
	printf("%s\n\n", plaintext);
	printf("AES CTR 256 Ciphertext (hex representation):\n");
	print_output(ciphertext, ctxt_len);
}
/*
crypto::aes_ctr_decrypt()
REQUIRED MEMBER VARIABLES:
unsigned char* key;
unsigned char* iv;
unsigned char ciphertext[BUFF_SIZE];
*/
void crypto::aes_ctr_256_decrypt() {
	int length;
	int plaintext_len = 0;
	EVP_CIPHER_CTX *ctx;

	clear_decryptedtext();

	ctx = EVP_CIPHER_CTX_new();

	EVP_DecryptInit_ex(ctx, EVP_aes_256_ctr(), NULL, key, iv);
	EVP_DecryptUpdate(ctx, decryptedtext, &length, ciphertext, ctxt_len);
	plaintext_len += length;
	EVP_DecryptFinal_ex(ctx, decryptedtext + length, &length);
	plaintext_len += length;
	EVP_CIPHER_CTX_free(ctx);

	printf("AES CTR 256 Decrypted text:\n");
	decryptedtext[plaintext_len] = '\0';
	printf("%s\n\n", decryptedtext);
}

void crypto::aes_ctr_128_encrypt() {
	int length = 0;
	EVP_CIPHER_CTX *ctx;

	clear_ciphertext();

	ctx = EVP_CIPHER_CTX_new();

	EVP_EncryptInit_ex(ctx, EVP_aes_128_ctr(), NULL, key, iv);
	EVP_EncryptUpdate(ctx, ciphertext, &length, plaintext, ptxt_len + 1);
	ctxt_len = length;
	EVP_EncryptFinal_ex(ctx, ciphertext + length, &length);
	ctxt_len += length;
	EVP_CIPHER_CTX_free(ctx);

	printf("AES CTR 128 Initial Plaintext:\n");
	printf("%s\n\n", plaintext);
	printf("AES CTR 128 Ciphertext (hex representation):\n");
	print_output(ciphertext, ctxt_len);
}


void crypto::aes_ctr_128_decrypt() {
	int length;
	int plaintext_len = 0;
	EVP_CIPHER_CTX *ctx;

	clear_decryptedtext();

	ctx = EVP_CIPHER_CTX_new();

	EVP_DecryptInit_ex(ctx, EVP_aes_128_ctr(), NULL, key, iv);
	EVP_DecryptUpdate(ctx, decryptedtext, &length, ciphertext, ctxt_len);
	plaintext_len += length;
	EVP_DecryptFinal_ex(ctx, decryptedtext + length, &length);
	plaintext_len += length;
	EVP_CIPHER_CTX_free(ctx);

//	printf("AES CTR Decrypted text:\n");
// decryptedtext[plaintext_len] = '\0';
// printf("%s\n\n", decryptedtext);
}


/***************HASHES******************/
void crypto::sha_1(unsigned char* input){
	SHA1(input, strlen((char *)input), hash);
	printf("SHA1: \n");
	print_output(hash, HASH_SIZE);
}

void crypto::sha_2(unsigned char* input){
	int i;
	SHA256(input, strlen((char *)input), hash);
	printf("SHA2: \n");
	print_output(hash, HASH_SIZE);
}

void crypto::hmac_sha2(unsigned char* input){
	unsigned int length = 0;
	digest = HMAC(EVP_sha256(), key, OUR_KEY_SIZE, input, strlen((char *)input), NULL, &length);
	printf("HMAC_SHA2: \n");
	print_output(digest, (int)length);
}

void crypto::hmac_blake2s(unsigned char* input){
	unsigned int length = 0;
	digest = HMAC(EVP_blake2s256(), key, OUR_KEY_SIZE, input, strlen((char *)input), NULL, &length);
	printf("HMAC_BLAKE2s256: \n");
	print_output(digest, (int)length);
}

void crypto::print_output(unsigned char* output, int len) {
	for(int i = 0; i < len; i++) {
		printf("%02x", output[i]);
	}
	printf("\n\n");
	printf("CTXT len: %d\n", ctxt_len);
}

/*****************GETTERS****************/

unsigned char* crypto::get_plaintext(){
	return plaintext;
}

unsigned char* crypto::get_ciphertext(){
	return ciphertext;
}

unsigned char* crypto::get_decryptedtext(){
	return decryptedtext;
}

unsigned char* crypto::get_hash(){
	return hash;
}

unsigned char* crypto::get_digest() {
	return digest;
}
