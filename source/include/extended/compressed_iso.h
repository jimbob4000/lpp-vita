/*---------------------------------------------------------------------------------------------------------------------#
#- Compressed ISO reader for PSP metadata/artwork extraction ----------------------------------------------------------#
#- Format rules follow behavior reviewed from maxcso: https://github.com/unknownbrackets/maxcso -----------------------#
#----------------------------------------------------------------------------------------------------------------------*/

#ifndef _COMPRESSED_ISO_H_
#define _COMPRESSED_ISO_H_

#include "iso.h"
#include <zlib.h>

typedef enum CompressedISOFormat_ {
	COMPRESSED_ISO_UNKNOWN = 0,
	COMPRESSED_ISO_CSO1,
	COMPRESSED_ISO_CSO2,
	COMPRESSED_ISO_ZSO,
	COMPRESSED_ISO_DAX
} CompressedISOFormat;

class CompressedISO : public ISO
{
private:
	static const uint32_t DAX_FRAME_SIZE;
	static const uint32_t INDEX_FLAG;

	CompressedISOFormat mFormat;
	uint64_t mTotalBytes;
		uint32_t mBlockSize;
		uint32_t mTotalBlock;
		uint32_t mHeaderSize;
		uint64_t mFileSize;
		uint8_t mAlign;
		bool mReady;

	std::vector<uint32_t> mIndex;
	std::vector<uint32_t> mDaxIndex;
	std::vector<uint16_t> mDaxSize;
	std::vector<uint8_t> mDaxPlain;

		char *mCacheAlloc;
		char *mCache;
		uint32_t mCacheBlock;
		bool mCacheValid;

		bool init();
		bool initCisoLike(unsigned char *header);
		bool initDax(unsigned char *header);
		bool allocCache();
		bool validateIsoPayload();
		bool readBlock(uint32_t block);
	bool readCisoLikeBlock(uint32_t block);
	bool readDaxBlock(uint32_t block);
	bool inflateBlock(const void *src, uint32_t srcSize, void *dst, uint32_t dstSize, int windowBits);

	virtual int readSector(char *destBuf, unsigned sector);

public:
	CompressedISO(std::string path);
	virtual ~CompressedISO();

	void ExtractPic1(std::string pic_path);
	void* ExtractSfoToMemory(uint32_t* out_size);

	static bool isCompressedISO(std::string filePath);
};

#endif
