/*---------------------------------------------------------------------------------------------------------------------#
#- Compressed ISO reader for PSP metadata/artwork extraction ----------------------------------------------------------#
#- Format rules follow behavior reviewed from maxcso: https://github.com/unknownbrackets/maxcso -----------------------#
#----------------------------------------------------------------------------------------------------------------------*/

#include <vitasdk.h>
#include <cstring>
#include <cstdlib>
#include <ios>
#include "compressed_iso.h"

extern "C" {
#include "../lz4/lz4.h"
}

const uint32_t CompressedISO::DAX_FRAME_SIZE = 0x2000;
const uint32_t CompressedISO::INDEX_FLAG = 0x80000000;
static const uint32_t CACHE_GUARD_SIZE = ISO::SECTOR_SIZE;

static uint32_t read_le32(const unsigned char *p)
{
	return ((uint32_t)p[0]) | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint64_t read_le64(const unsigned char *p)
{
	return (uint64_t)read_le32(p) | ((uint64_t)read_le32(p + 4) << 32);
}

CompressedISO::CompressedISO(std::string path)
: ISO(path),
  mFormat(COMPRESSED_ISO_UNKNOWN),
	  mTotalBytes(0),
	  mBlockSize(ISO::SECTOR_SIZE),
	  mTotalBlock(0),
	  mHeaderSize(0),
	  mFileSize(0),
	  mAlign(0),
	  mReady(false),
	  mCacheAlloc(NULL),
	  mCache(NULL),
	  mCacheBlock(0),
	  mCacheValid(false)
{
	mReady = init();
}

CompressedISO::~CompressedISO()
{
	if (mCacheAlloc) {
		free(mCacheAlloc);
		mCacheAlloc = NULL;
		mCache = NULL;
	}
}

void CompressedISO::ExtractPic1(std::string pic_path)
{
	if (!mReady) {
		return;
	}
	ISO::ExtractPic1(pic_path);
}

void* CompressedISO::ExtractSfoToMemory(uint32_t* out_size)
{
	if (out_size) {
		*out_size = 0;
	}
	if (!mReady) {
		return NULL;
	}
	return ISO::ExtractSfoToMemory(out_size);
}

bool CompressedISO::isCompressedISO(std::string filePath)
{
	bool result = false;
	SceUID fd = sceIoOpen(filePath.c_str(), SCE_O_RDONLY, 0777);
	if (fd >= 0) {
		unsigned char header[4];
		if (sizeof(header) == sceIoRead(fd, header, sizeof(header))) {
			if (!memcmp(header, "CISO", 4) || !memcmp(header, "ZISO", 4) || !memcmp(header, "DAX\0", 4)) {
				result = true;
			}
		}
		sceIoClose(fd);
	}
	return result;
}

bool CompressedISO::init()
{
	if (!mFin.is_open()) {
		return false;
	}

	mFin.seekg(0, std::ios::end);
	std::streampos endPos = mFin.tellg();
	if (endPos <= 0) {
		return false;
	}
	mFileSize = (uint64_t)endPos;

	unsigned char header[32];
	memset(header, 0, sizeof(header));
	mFin.seekg(0, std::ios::beg);
	mFin.read((char*)header, sizeof(header));
	if (mFin.gcount() < 24) {
		return false;
	}

	if (!memcmp(header, "CISO", 4) || !memcmp(header, "ZISO", 4)) {
		return initCisoLike(header);
	}

	if (!memcmp(header, "DAX\0", 4)) {
		return initDax(header);
	}

	return false;
}

bool CompressedISO::initCisoLike(unsigned char *header)
{
	mFormat = !memcmp(header, "ZISO", 4) ? COMPRESSED_ISO_ZSO : (header[20] == 2 ? COMPRESSED_ISO_CSO2 : COMPRESSED_ISO_CSO1);
	mHeaderSize = read_le32(header + 4);
	if (mHeaderSize == 0) {
		mHeaderSize = 24;
	}
	mTotalBytes = read_le64(header + 8);
	mBlockSize = read_le32(header + 16);
	mAlign = header[21];

	if (mHeaderSize < 24 || mHeaderSize > mFileSize || mTotalBytes == 0 ||
	    mBlockSize < ISO::SECTOR_SIZE || mBlockSize > 2 * 1024 * 1024 ||
	    (mBlockSize % ISO::SECTOR_SIZE) != 0 || mAlign > 31) {
		return false;
	}
	if ((mTotalBytes + mBlockSize - 1) / mBlockSize > 0xFFFFFFFFULL) {
		return false;
	}

	mTotalBlock = (uint32_t)((mTotalBytes + mBlockSize - 1) / mBlockSize);
	mIndex.resize(mTotalBlock + 1);
	if ((uint64_t)mHeaderSize + ((uint64_t)mIndex.size() * sizeof(uint32_t)) > mFileSize) {
		return false;
	}

	mFin.seekg(mHeaderSize, std::ios::beg);
	mFin.read((char*)&mIndex[0], mIndex.size() * sizeof(uint32_t));
	if (mFin.gcount() != (std::streamsize)(mIndex.size() * sizeof(uint32_t))) {
		return false;
	}

	return allocCache() && validateIsoPayload();
}

bool CompressedISO::initDax(unsigned char *header)
{
	uint32_t version = read_le32(header + 8);
	uint32_t ncAreas = read_le32(header + 12);
	if (version > 1) {
		return false;
	}

	mFormat = COMPRESSED_ISO_DAX;
	mTotalBytes = read_le32(header + 4);
	mBlockSize = DAX_FRAME_SIZE;
	mTotalBlock = (uint32_t)((mTotalBytes + DAX_FRAME_SIZE - 1) / DAX_FRAME_SIZE);

	if (mTotalBytes == 0 || mTotalBlock == 0) {
		return false;
	}

	mDaxIndex.resize(mTotalBlock);
	mDaxSize.resize(mTotalBlock);
	mDaxPlain.assign(mTotalBlock, 0);
	uint64_t daxTableEnd = 32 + ((uint64_t)mTotalBlock * sizeof(uint32_t)) + ((uint64_t)mTotalBlock * sizeof(uint16_t)) + ((uint64_t)ncAreas * 8);
	if (daxTableEnd > mFileSize) {
		return false;
	}

	mFin.seekg(32, std::ios::beg);
	mFin.read((char*)&mDaxIndex[0], mTotalBlock * sizeof(uint32_t));
	if (mFin.gcount() != (std::streamsize)(mTotalBlock * sizeof(uint32_t))) {
		return false;
	}
	mFin.read((char*)&mDaxSize[0], mTotalBlock * sizeof(uint16_t));
	if (mFin.gcount() != (std::streamsize)(mTotalBlock * sizeof(uint16_t))) {
		return false;
	}

	for (uint32_t i = 0; i < ncAreas; ++i) {
		unsigned char area[8];
		mFin.read((char*)area, sizeof(area));
		if (mFin.gcount() != (std::streamsize)sizeof(area)) {
			return false;
		}
		uint32_t start = read_le32(area);
		uint32_t count = read_le32(area + 4);
		for (uint32_t frame = 0; frame < count && start + frame < mDaxPlain.size(); ++frame) {
			mDaxPlain[start + frame] = 1;
		}
	}

	return allocCache() && validateIsoPayload();
}

bool CompressedISO::allocCache()
{
	uint64_t allocSize = (uint64_t)mBlockSize + (CACHE_GUARD_SIZE * 2);
	if (allocSize > 2 * 1024 * 1024 + (CACHE_GUARD_SIZE * 2)) {
		return false;
	}

	mCacheAlloc = (char*)malloc((size_t)allocSize);
	if (!mCacheAlloc) {
		return false;
	}

	memset(mCacheAlloc, 0, (size_t)allocSize);
	mCache = mCacheAlloc + CACHE_GUARD_SIZE;
	return true;
}

bool CompressedISO::validateIsoPayload()
{
	uint64_t logicalPos = (uint64_t)16 * ISO::SECTOR_SIZE;
	if (logicalPos + ISO::SECTOR_SIZE > mTotalBytes) {
		return false;
	}

	uint32_t block = (uint32_t)(logicalPos / mBlockSize);
	uint32_t offset = (uint32_t)(logicalPos - ((uint64_t)block * mBlockSize));
	if (offset + ISO::SECTOR_SIZE > mBlockSize) {
		return false;
	}
	if (!readBlock(block)) {
		return false;
	}

	char *sector = mCache + offset;
	return sector[0] == 0x01 &&
	       memcmp(sector + 1, "CD001", 5) == 0 &&
	       sector[6] == 0x01;
}

bool CompressedISO::readBlock(uint32_t block)
{
	if (mCacheValid && mCacheBlock == block) {
		return true;
	}

	if (!mCache || block >= mTotalBlock) {
		return false;
	}

	memset(mCache, 0, mBlockSize);
	mCacheBlock = block;
	mCacheValid = false;

	if (mFormat == COMPRESSED_ISO_DAX) {
		return readDaxBlock(block);
	}

	return readCisoLikeBlock(block);
}

bool CompressedISO::readCisoLikeBlock(uint32_t block)
{
	if (block + 1 >= mIndex.size()) {
		return false;
	}

	uint32_t entry = mIndex[block];
	uint32_t next = mIndex[block + 1];
	uint64_t pos = (uint64_t)(entry & ~INDEX_FLAG) << mAlign;
	uint64_t nextPos = (uint64_t)(next & ~INDEX_FLAG) << mAlign;
	if (nextPos < pos || pos > mFileSize || nextPos > mFileSize) {
		return false;
	}

	uint32_t storedSize = (uint32_t)(nextPos - pos);
	bool plain = false;
	bool lz4 = false;
	bool deflate = false;

	if (mFormat == COMPRESSED_ISO_CSO1) {
		plain = (entry & INDEX_FLAG) != 0;
		deflate = !plain;
	} else if (mFormat == COMPRESSED_ISO_ZSO) {
		plain = (entry & INDEX_FLAG) != 0;
		lz4 = !plain;
	} else if (mFormat == COMPRESSED_ISO_CSO2) {
		plain = storedSize >= mBlockSize;
		if (!plain) {
			lz4 = (entry & INDEX_FLAG) != 0;
			deflate = !lz4;
		}
	} else {
		return false;
	}

	uint32_t readable = mBlockSize;
	if ((uint64_t)mCacheBlock * mBlockSize + readable > mTotalBytes) {
		readable = (uint32_t)(mTotalBytes - ((uint64_t)mCacheBlock * mBlockSize));
	}

	if (plain) {
		if (pos + readable > mFileSize) {
			return false;
		}
		mFin.seekg((std::streamoff)pos, std::ios::beg);
		mFin.read(mCache, readable);
		mCacheValid = mFin.gcount() == (std::streamsize)readable;
		return mCacheValid;
	}

	void *compressed = malloc(storedSize);
	if (!compressed) {
		return false;
	}
	mFin.seekg((std::streamoff)pos, std::ios::beg);
	mFin.read((char*)compressed, storedSize);
	if (mFin.gcount() != (std::streamsize)storedSize) {
		free(compressed);
		return false;
	}

	if (deflate) {
		mCacheValid = inflateBlock(compressed, storedSize, mCache, mBlockSize, -15);
	} else if (lz4) {
		int decoded = LZ4_decompress_safe_partial((const char*)compressed, mCache, (int)storedSize, (int)mBlockSize, (int)mBlockSize);
		mCacheValid = decoded > 0;
	}

	free(compressed);
	return mCacheValid;
}

bool CompressedISO::readDaxBlock(uint32_t block)
{
	if (block >= mDaxIndex.size()) {
		return false;
	}

	uint32_t storedSize = mDaxSize[block];
	uint32_t readSize = mDaxPlain[block] ? DAX_FRAME_SIZE : storedSize;
	if ((uint64_t)mDaxIndex[block] + readSize > mFileSize) {
		return false;
	}

	void *data = malloc(readSize);
	if (!data) {
		return false;
	}

	mFin.seekg((std::streamoff)mDaxIndex[block], std::ios::beg);
	mFin.read((char*)data, readSize);
	if (mFin.gcount() != (std::streamsize)readSize) {
		free(data);
		return false;
	}

	if (mDaxPlain[block]) {
		memcpy(mCache, data, readSize > mBlockSize ? mBlockSize : readSize);
		mCacheValid = true;
	} else {
		mCacheValid = inflateBlock(data, storedSize, mCache, DAX_FRAME_SIZE, 15);
	}

	free(data);
	return mCacheValid;
}

bool CompressedISO::inflateBlock(const void *src, uint32_t srcSize, void *dst, uint32_t dstSize, int windowBits)
{
	z_stream stream;
	memset(&stream, 0, sizeof(stream));
	stream.next_in = (Bytef*)src;
	stream.avail_in = (uInt)srcSize;
	stream.next_out = (Bytef*)dst;
	stream.avail_out = (uInt)dstSize;

	int err = inflateInit2(&stream, windowBits);
	if (err != Z_OK) {
		return false;
	}

	err = inflate(&stream, Z_FINISH);
	inflateEnd(&stream);

	return err == Z_STREAM_END && stream.total_out > 0 && stream.total_out <= dstSize;
}

int CompressedISO::readSector(char *destBuf, unsigned sector)
{
	if (!mReady) {
		return -1;
	}

	uint64_t logicalPos = (uint64_t)sector * ISO::SECTOR_SIZE;
	if (logicalPos >= mTotalBytes) {
		return -2;
	}

	uint32_t block = (uint32_t)(logicalPos / mBlockSize);
	uint32_t offset = (uint32_t)(logicalPos - ((uint64_t)block * mBlockSize));
	if (offset + ISO::SECTOR_SIZE > mBlockSize) {
		return -4;
	}
	if (!readBlock(block)) {
		return -3;
	}

	uint32_t readable = ISO::SECTOR_SIZE;
	if (logicalPos + readable > mTotalBytes) {
		readable = (uint32_t)(mTotalBytes - logicalPos);
	}
	memcpy(destBuf, mCache + offset, readable);
	if (readable < ISO::SECTOR_SIZE) {
		memset(destBuf + readable, 0, ISO::SECTOR_SIZE - readable);
	}

	return ISO::SECTOR_SIZE;
}
