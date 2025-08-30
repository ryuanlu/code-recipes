#include <openssl/evp.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#define AES_KEY	"0123456789abcdef0123456789abcdef"
#define AES_IV	"0123456789abcdef"

#define BUFFER_SIZE	(4096)

enum
{
	DECRYPT,
	ENCRYPT,
};

int cipher(int encrypt)
{
	const unsigned char key[] = AES_KEY;
	const unsigned char iv[] = AES_IV;
	EVP_CIPHER_CTX* context = NULL;
	unsigned char* input_buffer = NULL;
	unsigned char* output_buffer = NULL;
	int r = 0;
	int out_length = 0;

	context = EVP_CIPHER_CTX_new();

	EVP_CipherInit_ex(context, EVP_aes_256_cbc(), NULL, key, iv, encrypt);
	input_buffer = calloc(1, BUFFER_SIZE);
	output_buffer = calloc(1, BUFFER_SIZE);

	while(!feof(stdin))
	{
		r = fread(input_buffer, 1, BUFFER_SIZE, stdin);
		EVP_CipherUpdate(context, output_buffer, &out_length, input_buffer, r);
		fwrite(output_buffer, 1, out_length, stdout);
	}

	EVP_CipherFinal_ex(context, output_buffer, &out_length);
	fwrite(output_buffer, 1, out_length, stdout);

	free(output_buffer);
	free(input_buffer);
	EVP_CIPHER_CTX_free(context);

	return EXIT_SUCCESS;
}

int main(int argc, char** argv)
{
	int option = 0;

	do
	{
		option = getopt(argc, argv, "de");

		switch(option)
		{
			case 'd':
			return cipher(DECRYPT);
			case 'e':
			return cipher(ENCRYPT);
			break;
			default:
			break;
		}

	}while(option != -1);

	return EXIT_FAILURE;
}
