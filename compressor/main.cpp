#include <lz4.h>
#include <lz4hc.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <new>

typedef unsigned long long uint64;

static uint64 magic = 0x6d4936386c756d45ULL;

void decompress(const char* in_file, const char* outfile)
{
	FILE* filein = fopen(in_file, "rb");
	FILE* fileout = fopen(outfile, "wb");
	
	fseek(filein, 0L, SEEK_END);
	uint64 size = ftell(filein);
	fseek(filein, 0L, SEEK_SET);

	uint64 in_magic;
	fread(&in_magic, sizeof(uint64), 1, filein);
	
	assert(in_magic == magic);
	
	uint64 block_size = 1024 * 1024;
	uint64 bound = LZ4_compressBound(block_size);
	char* data = new char[bound];
	
	uint64 decompressed_size = 0;
	fread(&decompressed_size, sizeof(uint64), 1, filein);
	
	char* data_dst = new char[decompressed_size];
	//printf("Decompressed size: %08x\n", decompressed_size);
	
	uint64 p = 0;
	for (p = 0; p != decompressed_size;)
	{
		uint64 this_block_size = 0;
		uint64 compressed_size = 0;
		fread(&this_block_size, sizeof(uint64), 1, filein);
		fread(&compressed_size, sizeof(uint64), 1, filein);
		fread(data, compressed_size, 1, filein);
		
		for (unsigned int i = 0; i < (compressed_size + sizeof(uint64) - 1) / sizeof(uint64); ++i)
			reinterpret_cast<uint64*>(data)[i] ^= 0xA3F2A5C6B8B48544;
		
		uint64 compressed = LZ4_decompress_fast(data, data_dst + p, this_block_size);
		assert(compressed == compressed_size);
		//printf("Writing block at: %08x, of size %08x \n", p, this_block_size);
		p += this_block_size;
	}
	fwrite(data_dst, decompressed_size, 1, fileout);
	uint64 size_read = ftell(filein);
	assert(size == size_read);
	fclose(filein);
	fclose(fileout);
}

void compress(const char* in_file, const char* outfile)
{
	FILE* file = fopen(in_file, "rb");
	FILE* fileout = fopen(outfile, "wb");
	fseek(file, 0L, SEEK_END);
	uint64 size = ftell(file);
	fseek(file, 0L, SEEK_SET);
		
	fwrite(&magic, sizeof(uint64), 1, fileout);
	fwrite(&size, sizeof(uint64), 1, fileout);
		
	uint64 block_size = 1024 * 1024;
	char* data = new char[block_size];
	
	uint64 bound = LZ4_compressBound(block_size);
	char* data_dst = new char[bound];
	
	uint64 p = 0;
	for (p = 0; p + block_size < size; p += block_size)
	{
		fread(data, block_size, 1, file);
		//printf("Reading block at: %08x, of size %08x \n", p, block_size);
		uint64 compressed_size = LZ4_compress_HC(data, data_dst, block_size, bound, LZ4HC_CLEVEL_MAX);
		//printf("Compressed_size %08x \n", compressed_size);
		fwrite(&block_size, sizeof(uint64), 1, fileout);
		fwrite(&compressed_size, sizeof(uint64), 1, fileout);
		
		for (unsigned int i = 0; i < (compressed_size + sizeof(uint64) - 1) / sizeof(uint64); ++i)
			reinterpret_cast<uint64*>(data_dst)[i] ^= 0xA3F2A5C6B8B48544;
		
		fwrite(data_dst, compressed_size, 1, fileout);
	}
	
	if (p != size)
	{
		uint64 this_block_size = size - p;
		fread(data, this_block_size, 1, file);
		//printf("Reading block at: %08x, of size %08x\n", p, this_block_size);
		uint64 compressed_size = LZ4_compress_HC(data, data_dst, this_block_size, bound, LZ4HC_CLEVEL_MAX);
		//printf("Compressed_size %08x \n", compressed_size);
		fwrite(&this_block_size, sizeof(uint64), 1, fileout);
		fwrite(&compressed_size, sizeof(uint64), 1, fileout);
		
		for (unsigned int i = 0; i < (compressed_size + sizeof(uint64) - 1) / sizeof(uint64); ++i)
			reinterpret_cast<uint64*>(data_dst)[i] ^= 0xA3F2A5C6B8B48544;
		
		fwrite(data_dst, compressed_size, 1, fileout);
	}
	
	fclose(file);
	fclose(fileout);
}

int main(int argc, char **argv)
{
	if (argc > 3)
	{
		if (strcmp(argv[1], "-c") == 0)
		{
			compress(argv[2], argv[3]);
			return 0;
		}
		else if(strcmp(argv[1], "-d") == 0)
		{
			decompress(argv[2], argv[3]);
			return 0;
		}
	}
	printf("Error\n");
	return 1;
}
