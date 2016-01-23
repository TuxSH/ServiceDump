#include <stdio.h>
#include <string.h>
#include <3ds.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct {
	u64 titleid;
	const char* name;
	int hasSafe;
	int hasN3DS;
	int hasN3DSSafe;
} TitleInfo;

#define NB_TITLES (sizeof(titles)/sizeof(TitleInfo))

/*
 def buildlst(L, st):
	for i in range(len(L)):
		print('{{ {0}LL, "{1}", 1, 0, 1 }},'.format(hex(st+0x100*i), L[i]))
*/

TitleInfo titles[] = {
/*{ 0x0004013000001002LL, "sm", 1, 0, 0 },
{ 0x0004013000001102LL, "fs", 1, 0, 0 },
{ 0x0004013000001202LL, "pm", 1, 0, 0 },	
{ 0x0004013000001302LL, "loader", 1, 0 },
{ 0x0004013000001402LL, "pxi", 1, 0, 0 },*/

{ 0x4013000001502LL, "am", 1, 0, 1 },
{ 0x4013000001602LL, "camera", 0, 1, 0},
{ 0x4013000001702LL, "cfg", 1, 0, 1 },
{ 0x4013000001802LL, "codec", 1, 0, 1 },

{ 0x4013000001a02LL, "dsp", 1, 0, 1 },
{ 0x4013000001b02LL, "gpio", 1, 0, 1 },
{ 0x4013000001c02LL, "gsp", 1, 1, 1 },
{ 0x4013000001d02LL, "hid", 1, 0, 1 },
{ 0x4013000001e02LL, "i2c", 1, 1, 1 },
{ 0x4013000001f02LL, "mcu", 1, 1, 1 },
{ 0x4013000002002LL, "mic", 0, 0, 0 },
{ 0x4013000002102LL, "pdn", 1, 0, 1 },
{ 0x4013000002202LL, "ptm", 1, 1, 1 },
{ 0x4013000002302LL, "spi", 1, 1, 1 },
{ 0x4013000002402LL, "ac", 1, 0, 1 },

{ 0x4013000002602LL, "cecd", 0, 0, 0 },
{ 0x4013000002702LL, "csnd", 1, 0, 1 },
{ 0x4013000002802LL, "dlp", 0, 0, 0 },
{ 0x4013000002902LL, "http", 1, 0, 1 },
{ 0x4013000002a02LL, "mp", 1, 0, 0 },
{ 0x4013000002b02LL, "ndm", 0, 0, 0 },
{ 0x4013000002c02LL, "nim", 1, 0, 1 },
{ 0x4013000002d02LL, "nwm", 1, 0, 1 },
{ 0x4013000002e02LL, "sockets", 1, 0, 1 },
{ 0x4013000002f02LL, "ssl", 1, 0, 1 },
//{ 0x4013000003002LL, "Process9", 0, 0, 0 },
{ 0x4013000003102LL, "ps", 1, 0, 1 },
{ 0x4013000003202LL, "friends", 1, 0, 1 },
{ 0x4013000003302LL, "ir", 1, 0, 1 },
{ 0x4013000003402LL, "BOSS", 0, 0, 0 },
{ 0x4013000003502LL, "news", 0, 0, 0 },

{ 0x4013000003702LL, "ro", 0, 0, 0 },
{ 0x4013000003802LL, "act", 0, 0, 0 },

{ 0x4013000004002LL, "nfc", 0, 1, 0 },

{ 0x4013000008002LL, "ns", 1, 0, 1 },

};

TitleInfo N3DSUniqueTitles[] = {
{ 0x4013000004102LL, "mvd", 0, 1, 0 },
{ 0x4013000004202LL, "qtm" , 0, 1, 0 },
};

void __appInit() {
	// Initialize services
	srvInit();
	aptInit();
	hidInit();

	fsInit();
	sdmcInit();
}

void __appExit() {
	// Exit services
	sdmcExit();
	fsExit();

	hidExit();
	aptExit();
	srvExit();
}


// decompression code stolen from ctrtool

u32 getle32(const u8* p)
{
	return (p[0]<<0) | (p[1]<<8) | (p[2]<<16) | (p[3]<<24);
}

u32 lzss_get_decompressed_size(u8* compressed, u32 compressedsize)
{
	u8* footer = compressed + compressedsize - 8;

	u32 originalbottom = getle32(footer+4);

	return originalbottom + compressedsize;
}

int lzss_decompress(u8* compressed, u32 compressedsize, u8* decompressed, u32 decompressedsize)
{
	u8* footer = compressed + compressedsize - 8;
	u32 buffertopandbottom = getle32(footer+0);
	//u32 originalbottom = getle32(footer+4);
	u32 i, j;
	u32 out = decompressedsize;
	u32 index = compressedsize - ((buffertopandbottom>>24)&0xFF);
	u32 segmentoffset;
	u32 segmentsize;
	u8 control;
	u32 stopindex = compressedsize - (buffertopandbottom&0xFFFFFF);

	memset(decompressed, 0, decompressedsize);
	memcpy(decompressed, compressed, compressedsize);

	
	while(index > stopindex)
	{
		control = compressed[--index];
		

		for(i=0; i<8; i++)
		{
			if (index <= stopindex)
				break;

			if (index <= 0)
				break;

			if (out <= 0)
				break;

			if (control & 0x80)
			{
				if (index < 2)
				{
					// fprintf(stderr, "Error, compression out of bounds\n");
					goto clean;
				}

				index -= 2;

				segmentoffset = compressed[index] | (compressed[index+1]<<8);
				segmentsize = ((segmentoffset >> 12)&15)+3;
				segmentoffset &= 0x0FFF;
				segmentoffset += 2;

				
				if (out < segmentsize)
				{
					// fprintf(stderr, "Error, compression out of bounds\n");
					goto clean;
				}

				for(j=0; j<segmentsize; j++)
				{
					u8 data;
					
					if (out+segmentoffset >= decompressedsize)
					{
						// fprintf(stderr, "Error, compression out of bounds\n");
						goto clean;
					}

					data  = decompressed[out+segmentoffset];
					decompressed[--out] = data;
				}
			}
			else
			{
				if (out < 1)
				{
					// fprintf(stderr, "Error, compression out of bounds\n");
					goto clean;
				}
				decompressed[--out] = compressed[--index];
			}

			control <<= 1;
		}
	}

	return 0;
	
	clean:
	return -1;
}


Result openCode(Handle* out, u64 tid, u8 mediatype)
{
	if(!out)return -1;

	u32 archivePath[] = {tid & 0xFFFFFFFF, (tid >> 32) & 0xFFFFFFFF, mediatype, 0x00000000};
	static const u32 filePath[] = {0x00000000, 0x00000000, 0x00000002, 0x646F632E, 0x00000065};

	return FSUSER_OpenFileDirectly(out, (FS_Archive){0x2345678a, (FS_Path){PATH_BINARY, 0x10, (u8*)archivePath}}, (FS_Path){PATH_BINARY, 0x14, (u8*)filePath}, FS_OPEN_READ, 0);
}

// stolen and adapted from hans
Result dumpCode(u64 tid , const char* path)
{
	Result ret;
	Handle fileHandle;

	ret = openCode(&fileHandle, tid, 0);

	char name[50];
	sprintf(name, "%s.bin", path);

	u8* fileBuffer = NULL;
	u64 fileSize = 0;

	{
		u32 bytesRead;

		ret = FSFILE_GetSize(fileHandle, &fileSize);
		if(ret)return ret;

		fileBuffer = malloc(fileSize);
		if(ret)return ret;

		ret = FSFILE_Read(fileHandle, &bytesRead, 0x0, fileBuffer, fileSize);
		if(ret)return ret;

		ret = FSFILE_Close(fileHandle);
		if(ret)return ret;

	}

	u32 decompressedSize = lzss_get_decompressed_size(fileBuffer, fileSize);
	u8* decompressedBuffer = malloc(decompressedSize);
	if(!decompressedBuffer)return -1;

	lzss_decompress(fileBuffer, fileSize, decompressedBuffer, decompressedSize);
	free(fileBuffer);

	FILE* f = fopen(name, "wb+");
	if(!f) { free(decompressedBuffer); return -2; }
	fwrite(decompressedBuffer, 1, decompressedSize, f);
	fclose(f);

	free(decompressedBuffer);
	return 0;
}

int main(int argc, char** argv)
{
	int i = 0;
	u8 isN3DS = 0;
	
	// Initialize services
	gfxInitDefault();

	// Init console for text output
	consoleInit(GFX_TOP, NULL);
	
	APT_CheckNew3DS(&isN3DS);
	
	mkdir("native", 777);
	mkdir("safe", 777);
	if(isN3DS) {
		mkdir("n3ds_native", 777);
		mkdir("n3ds_safe", 777);
	}

	printf("Dumping native titles\n");
	chdir("native");
	for(i = 0; i < NB_TITLES; ++i){
		TitleInfo tl = titles[i];
		if(dumpCode(tl.titleid, tl.name)) printf("Cannot dump native %s\n", tl.name);
		else printf("%s ", tl.name);
	}

	printf("\n\nDumping safe titles\n");
	chdir("../safe");
	for(i = 0; i < NB_TITLES; ++i){
		TitleInfo tl = titles[i];
		if(!tl.hasSafe) continue;
		if(dumpCode(tl.titleid|1, tl.name)) printf("Cannot dump safe %s\n", tl.name);
		else printf("%s ", tl.name);

	}


	if(isN3DS) {
		printf("\n\nDumping N3DS native titles\n");
		chdir("../n3ds_native");
		for(i = 0; i < NB_TITLES; ++i){
			TitleInfo tl = titles[i];
			if(!tl.hasN3DS) continue;
			if(dumpCode(tl.titleid|0x20000000, tl.name)) printf("Cannot dump N3DS native %s\n", tl.name);
			else printf("%s ", tl.name);
		}
		for(i = 0; i < 2; ++i){
			TitleInfo tl = N3DSUniqueTitles[i];
			if(dumpCode(tl.titleid|0x20000000, tl.name)) printf("Cannot dump N3DS native %s\n", tl.name);
			else printf("%s ", tl.name);
		}

		printf("\n\nDumping N3DS safe titles\n");
		chdir("../n3ds_safe");
		for(i = 0; i < NB_TITLES; ++i){
			TitleInfo tl = titles[i];
			if(!tl.hasN3DSSafe) continue;
			if(dumpCode(tl.titleid|0x20000001, tl.name)) printf("Cannot dump N3DS safe %s\n", tl.name);
			else printf("%s ", tl.name);
		}
	}



	printf("\n\nDone.\n");
	// Main loop
	while (aptMainLoop())
	{
		hidScanInput();

		u32 kDown = hidKeysDown();
		if (kDown & KEY_START)
			break; // break in order to return to hbmenu

		// Flush and swap framebuffers
		gfxFlushBuffers();
		gfxSwapBuffers();
		gspWaitForVBlank();
	}

	// Exit services
	gfxExit();
	return 0;
}
