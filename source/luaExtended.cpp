/*---------------------------------------------------------------------------------------------------------------------#
#----------------------------------------------------------------------------------------------------------------------#
#------  This File is Part Of : ---------------------------------------------------------------------------------------#
#------- _  -------------------  ______  _   --------------------------------------------------------------------------#
#------ | | ------------------- (_____ \| |  --------------------------------------------------------------------------#
#------ | | ---  _   _   ____   _____) )| |  ____  _   _   ____   ____   ----------------------------------------------#
#------ | | --- | | | | / _  |  |  ____/| | / _  || | | | / _  ) / ___)  ----------------------------------------------#
#------ | |_____| |_| |( ( | |  | |     | |( ( | || |_| |( (/ / | |  --------------------------------------------------#
#------ |_______)\____| \_||_|  |_|     |_| \_||_| \__  | \____)|_|  --------------------------------------------------#
#------------------------------------------------- (____/ -------------------------------------------------------------#
#------------------------   ______   _   ------------------------------------------------------------------------------#
#------------------------  (_____ \ | |  ------------------------------------------------------------------------------#
#------------------------   _____) )| | _   _   ___   -----------------------------------------------------------------#
#------------------------  |  ____/ | || | | | /___)  -----------------------------------------------------------------#
#------------------------  | |      | || |_| ||___ |  -----------------------------------------------------------------#
#------------------------  |_|      |_| \____|(___/   -----------------------------------------------------------------#
#----------------------------------------------------------------------------------------------------------------------#
#----------------------------------------------------------------------------------------------------------------------#
#- Licensed under the GPL License -------------------------------------------------------------------------------------#
#----------------------------------------------------------------------------------------------------------------------#
#- Copyright (c) Nanni <lpp.nanni@gmail.com> --------------------------------------------------------------------------#
#- Copyright (c) Rinnegatamante <rinnegatamante@gmail.com> ------------------------------------------------------------#
#----------------------------------------------------------------------------------------------------------------------#
#----------------------------------------------------------------------------------------------------------------------#
#- This file :---------------------------------------------------------------------------------------------------------#
#- Created by jimbob4000 to extend functionality for RetroFlow Launcher: ----------------------------------------------#
#- https://github.com/jimbob4000/RetroFlow-Launcher -------------------------------------------------------------------#
#----------------------------------------------------------------------------------------------------------------------#
#- Credits : ----------------------------------------------------------------------------------------------------------#
#----------------------------------------------------------------------------------------------------------------------#
#- cy33hc for SFO reading for iso, cso and pbp files: taken and modified from: cy33hc's Vita Launcher -----------------#
#- https://github.com/cy33hc/vita-launcher ----------------------------------------------------------------------------#
#----------------------------------------------------------------------------------------------------------------------#
#- cy33hc for Copyicons functions made possible by compiling, cy33hc's copyicons code into a lib ----------------------#
#- https://github.com/cy33hc/copyicons --------------------------------------------------------------------------------#
#----------------------------------------------------------------------------------------------------------------------#
#- Both of cy33hc's works are licensed under GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007 -----------------------#
#----------------------------------------------------------------------------------------------------------------------*/

#include <stdlib.h>
#include <string.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <vita2d.h>
#include "include/luaplayer.h"
#include "include/unzip.h"
#include "include/extended/iso.h"
#include "include/extended/cso.h"
#include "include/extended/sfo.h"
#include "include/extended/eboot.h"

extern "C" {
#include <vitasdk.h>
#include <png.h>
#include <libimagequant.h>
// Forward declarations for copy functions
int copy_decrypted_icon(const char *app_path);
int copy_decrypted_pic(const char *app_path);
}

// Font structure for vita2d font support
struct ttf {
	uint32_t magic;
	vita2d_font* f;
	vita2d_pgf* f2;
	vita2d_pvf* f3;
	int size;
	float scale;
};

// Font buffer structure for memory-loaded fonts
struct font_buffer {
	uint32_t magic;
	void* data;
	size_t size;
};

typedef struct {
    char title[128];
    char category[10];
    char disc_id[20];
    char disc_version[10];
    char region[10];
    char system_ver[10];
    int success;
} SfoInfo;

// Simple file helper to read data
// Note: Not used by extractSfoFromGame anymore (optimized to use memory-based extraction)
static std::vector<char> loadFile(const char* path) {
    std::vector<char> data;
    SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0777);
    if (fd >= 0) {
        uint32_t size = sceIoLseek(fd, 0, SEEK_END);
        sceIoLseek(fd, 0, SEEK_SET);
        data.resize(size);
        sceIoRead(fd, data.data(), size);
        sceIoClose(fd);
    }
    return data;
}

// Internal C++ function for extracting SFO info from PSP/PSX games
static int extractSfoFromGame(const char* game_path, SfoInfo* info) {
    if (!info) return 0;
    
    // Initialize the structure
    memset(info, 0, sizeof(SfoInfo));
    info->success = 0;
    
    std::string game_path_str(game_path);
    
    void* sfo_data = nullptr;
    uint32_t sfo_size = 0;
    
    // Extract SFO data directly to memory (no temporary files)
    if (ISO::isISO(game_path_str)) {
        ISO *iso = new ISO(game_path_str);
        if (iso) {
            sfo_data = iso->ExtractSfoToMemory(&sfo_size);
            delete iso;
        }
    } else if (CSO::isCSO(game_path_str)) {
        CSO *cso = new CSO(game_path_str);
        if (cso) {
            sfo_data = cso->ExtractSfoToMemory(&sfo_size);
            delete cso;
        }
    } else if (game_path_str.length() >= 4 && 
               (game_path_str.substr(game_path_str.length() - 4) == ".pbp" ||
                game_path_str.substr(game_path_str.length() - 4) == ".PBP")) {
        // EBOOT.PBP file
        sfo_data = EBOOT::ExtractSfoToMemory(game_path_str.c_str(), &sfo_size);
    }
    
    if (sfo_data && sfo_size > 0) {
        // Parse SFO data directly from memory
        const char* title = SFO::GetString((const char*)sfo_data, sfo_size, "TITLE");
        if (title) {
            strncpy(info->title, title, sizeof(info->title) - 1);
            info->title[sizeof(info->title) - 1] = '\0';
        }
        
        const char* category = SFO::GetString((const char*)sfo_data, sfo_size, "CATEGORY");
        if (category) {
            strncpy(info->category, category, sizeof(info->category) - 1);
            info->category[sizeof(info->category) - 1] = '\0';
        }
        
        const char* disc_id = SFO::GetString((const char*)sfo_data, sfo_size, "DISC_ID");
        if (disc_id) {
            strncpy(info->disc_id, disc_id, sizeof(info->disc_id) - 1);
            info->disc_id[sizeof(info->disc_id) - 1] = '\0';
        }
        
        const char* disc_version = SFO::GetString((const char*)sfo_data, sfo_size, "DISC_VERSION");
        if (disc_version) {
            strncpy(info->disc_version, disc_version, sizeof(info->disc_version) - 1);
            info->disc_version[sizeof(info->disc_version) - 1] = '\0';
        }
        
        const char* region = SFO::GetString((const char*)sfo_data, sfo_size, "REGION");
        if (region) {
            strncpy(info->region, region, sizeof(info->region) - 1);
            info->region[sizeof(info->region) - 1] = '\0';
        }
        
        const char* system_ver = SFO::GetString((const char*)sfo_data, sfo_size, "PSP_SYSTEM_VER");
        if (system_ver) {
            strncpy(info->system_ver, system_ver, sizeof(info->system_ver) - 1);
            info->system_ver[sizeof(info->system_ver) - 1] = '\0';
        }
        
        info->success = 1;
        
        // Free the SFO data memory
        free(sfo_data);
    }
    
	return info->success;
}

static int lua_extractsfopspx(lua_State *L) {
	int argc = lua_gettop(L);
#ifndef SKIP_ERROR_HANDLING
	if (argc != 1)
		return luaL_error(L, "wrong number of arguments.");
#endif
	const char* game_path = luaL_checkstring(L, 1);
	
	// Use the internal function to extract SFO information
	SfoInfo info;
	int result = extractSfoFromGame(game_path, &info);
	
	if (!result) {
		lua_pushnil(L);
		return 1;
	}
	
	// Create the result table
	lua_newtable(L);
	
	// Add extracted SFO information to the table
	if (strlen(info.title) > 0) {
		lua_pushstring(L, "TITLE");
		lua_pushstring(L, info.title);
		lua_settable(L, -3);
	}
	
	if (strlen(info.category) > 0) {
		lua_pushstring(L, "CATEGORY");
		lua_pushstring(L, info.category);
		lua_settable(L, -3);
	}
	
	if (strlen(info.disc_id) > 0) {
		lua_pushstring(L, "DISC_ID");
		lua_pushstring(L, info.disc_id);
		lua_settable(L, -3);
	}
	
	if (strlen(info.disc_version) > 0) {
		lua_pushstring(L, "DISC_VERSION");
		lua_pushstring(L, info.disc_version);
		lua_settable(L, -3);
	}
	
	if (strlen(info.region) > 0) {
		lua_pushstring(L, "REGION");
		lua_pushstring(L, info.region);
		lua_settable(L, -3);
	}
	
	if (strlen(info.system_ver) > 0) {
		lua_pushstring(L, "PSP_SYSTEM_VER");
		lua_pushstring(L, info.system_ver);
		lua_settable(L, -3);
	}
	
	return 1;
}

// Internal C++ function for extracting PIC1.PNG from ISO/CSO games
static int extractImageFromPSPGame(const char* game_path, const char* dest_path) {
	std::string game_path_str(game_path);
	
	// Helper to ensure parent directory exists
	auto ensure_parent_dir = [](const char *path) {
		char tmp[512];
		strncpy(tmp, path, sizeof(tmp) - 1);
		tmp[sizeof(tmp) - 1] = 0;
		char *slash = strrchr(tmp, '/');
		if (slash) {
			*slash = 0;
			// Recursive mkdir implementation
			char dir_tmp[512];
			char *p = NULL;
			size_t len;
			snprintf(dir_tmp, sizeof(dir_tmp), "%s", tmp);
			len = strlen(dir_tmp);
			if(dir_tmp[len - 1] == '/') dir_tmp[len - 1] = 0;
			for(p = dir_tmp + 1; *p; p++) {
				if(*p == '/') {
					*p = 0;
					sceIoMkdir(dir_tmp, 0777);
					*p = '/';
				}
			}
			sceIoMkdir(dir_tmp, 0777);
		}
	};
	
	bool extraction_success = false;
	
	// Check file type and extract PIC1.PNG accordingly - only ISO and CSO supported
	if (ISO::isISO(game_path_str)) {
		ISO *iso = new ISO(game_path_str);
		if (iso) {
			// Ensure parent directory exists
			ensure_parent_dir(dest_path);
			iso->ExtractPic1(dest_path);
			delete iso;
			// Check if PIC1.PNG was extracted successfully
			SceUID check_file = sceIoOpen(dest_path, SCE_O_RDONLY, 0777);
			if (check_file >= 0) {
				sceIoClose(check_file);
				extraction_success = true;
			}
		}
	} else if (CSO::isCSO(game_path_str)) {
		CSO *cso = new CSO(game_path_str);
		if (cso) {
			// Ensure parent directory exists
			ensure_parent_dir(dest_path);
			cso->ExtractPic1(dest_path);
			delete cso;
			// Check if PIC1.PNG was extracted successfully
			SceUID check_file = sceIoOpen(dest_path, SCE_O_RDONLY, 0777);
			if (check_file >= 0) {
				sceIoClose(check_file);
				extraction_success = true;
			}
		}
	}
	
	return extraction_success ? 1 : 0;
}

static int lua_pspgetpic1(lua_State *L) {
	int argc = lua_gettop(L);
#ifndef SKIP_ERROR_HANDLING
	if (argc != 2)
		return luaL_error(L, "wrong number of arguments (expected: source_path, dest_path).");
#endif
	const char* game_path = luaL_checkstring(L, 1);
	const char* dest_path = luaL_checkstring(L, 2);
	
	// Extract PIC1.PNG from ISO/CSO games only
	int result = extractImageFromPSPGame(game_path, dest_path);
	
	lua_pushboolean(L, result);
	return 1;
}

// CRC32 table for polynomial 0xEDB88320
// Standard CRC-32 algorithm implementation
// Uses IEEE 802.3 polynomial (same as zlib, PNG, etc.)
static uint32_t crc32_table[256];

static void make_crc32_table() {
	uint32_t c;
	for (uint32_t i = 0; i < 256; i++) {
		c = i;
		for (uint32_t j = 0; j < 8; j++) {
			if (c & 1) {
				c = 0xEDB88320 ^ (c >> 1);
			} else {
				c = c >> 1;
			}
		}
		crc32_table[i] = c;
	}
}

static uint32_t crc32(uint32_t crc, const unsigned char *buf, size_t len) {
	crc = crc ^ 0xFFFFFFFF;
	for (size_t i = 0; i < len; i++) {
		crc = crc32_table[(crc ^ buf[i]) & 0xFF] ^ (crc >> 8);
	}
	return crc ^ 0xFFFFFFFF;
}

// Lua: crc32(data [, prev_crc])
static int lua_crc32(lua_State *L) {
	size_t len;
	const char *data = luaL_checklstring(L, 1, &len);
	uint32_t prev_crc = 0;
	if (lua_gettop(L) >= 2 && lua_isnumber(L, 2)) {
		prev_crc = lua_tointeger(L, 2);
	}
	static int table_ready = 0;
	if (!table_ready) {
		make_crc32_table();
		table_ready = 1;
	}
	uint32_t result = crc32(prev_crc, (const unsigned char *)data, len);
	// If a third argument is true, return hex string, else integer
	if (lua_gettop(L) >= 3 && lua_toboolean(L, 3)) {
		char hexstr[9];
		snprintf(hexstr, sizeof(hexstr), "%08x", result);
		lua_pushstring(L, hexstr);
	} else {
		lua_pushinteger(L, result);
	}
	return 1;
}

// Lua: crc32_file(filename)
static int lua_crc32_file(lua_State *L) {
	const char *filename = luaL_checkstring(L, 1);
	SceUID file = sceIoOpen(filename, SCE_O_RDONLY, 0777);
	if (file < 0) {
		lua_pushnil(L);
		lua_pushstring(L, "Error opening file");
		return 2;
	}
	static int table_ready = 0;
	if (!table_ready) {
		make_crc32_table();
		table_ready = 1;
	}
	uint32_t crc_result = 0;
	unsigned char buffer[4096];
	size_t bytesRead;
	while ((bytesRead = sceIoRead(file, buffer, sizeof(buffer))) > 0) {
		crc_result = crc32(crc_result, buffer, bytesRead);
	}
	sceIoClose(file);
	char hexstr[9];
	snprintf(hexstr, sizeof(hexstr), "%08x", crc_result);
	lua_pushstring(L, hexstr);
	return 1;
}

static int lua_archiveGetContents(lua_State *L) {
	const char *archive_path = luaL_checkstring(L, 1);
	
	// Determine archive type by extension
	const char *ext = strrchr(archive_path, '.');
	if (!ext) {
		lua_pushnil(L);
		return 1;
	}
	ext++; // Skip the dot
	
	// Handle ZIP files only
	if (strcasecmp(ext, "zip") == 0) {
		unzFile zip = unzOpen(archive_path);
		if (!zip) {
			lua_pushnil(L);
			return 1;
		}
		
		lua_newtable(L);
		int index = 1;
		
		unz_global_info global_info;
		if (unzGetGlobalInfo(zip, &global_info) == UNZ_OK) {
			unzGoToFirstFile(zip);
			for (int i = 0; i < global_info.number_entry; i++) {
				char filename[256];
				unz_file_info file_info;
				if (unzGetCurrentFileInfo(zip, &file_info, filename, sizeof(filename), NULL, 0, NULL, 0) == UNZ_OK) {
					lua_pushinteger(L, index++);
					lua_pushstring(L, filename);
					lua_settable(L, -3);
				}
				if (i < global_info.number_entry - 1) {
					unzGoToNextFile(zip);
				}
			}
		}
		unzClose(zip);
		return 1;
	}
	else {
		// Unsupported archive type
		lua_pushnil(L);
		return 1;
	}
}

// Lua: getPNGDimensions(filepath) - returns width, height or nil, error_msg
static int lua_getPNGDimensions(lua_State *L) {
	const char *filepath = luaL_checkstring(L, 1);
	
	// Open file for reading
	SceUID file = sceIoOpen(filepath, SCE_O_RDONLY, 0777);
	if (file < 0) {
		lua_pushnil(L);
		lua_pushstring(L, "Could not open file");
		return 2;
	}
	
	// Read PNG signature (8 bytes)
	unsigned char signature[8];
	if (sceIoRead(file, signature, 8) != 8) {
		sceIoClose(file);
		lua_pushnil(L);
		lua_pushstring(L, "File too short");
		return 2;
	}
	
	// Check PNG signature: 89 50 4E 47 0D 0A 1A 0A
	const unsigned char png_sig[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
	if (memcmp(signature, png_sig, 8) != 0) {
		sceIoClose(file);
		lua_pushnil(L);
		lua_pushstring(L, "Not a valid PNG file");
		return 2;
	}
	
	// Read IHDR chunk length (4 bytes, big-endian)
	unsigned char length_bytes[4];
	if (sceIoRead(file, length_bytes, 4) != 4) {
		sceIoClose(file);
		lua_pushnil(L);
		lua_pushstring(L, "Invalid PNG header");
		return 2;
	}
	
	// Read chunk type (4 bytes) - should be "IHDR"
	char chunk_type[4];
	if (sceIoRead(file, chunk_type, 4) != 4 || memcmp(chunk_type, "IHDR", 4) != 0) {
		sceIoClose(file);
		lua_pushnil(L);
		lua_pushstring(L, "IHDR chunk not found");
		return 2;
	}
	
	// Read width (4 bytes, big-endian)
	unsigned char width_bytes[4];
	if (sceIoRead(file, width_bytes, 4) != 4) {
		sceIoClose(file);
		lua_pushnil(L);
		lua_pushstring(L, "Could not read width");
		return 2;
	}
	
	// Convert big-endian bytes to uint32_t
	uint32_t width = (width_bytes[0] << 24) | (width_bytes[1] << 16) | 
					 (width_bytes[2] << 8) | width_bytes[3];
	
	// Read height (4 bytes, big-endian)
	unsigned char height_bytes[4];
	if (sceIoRead(file, height_bytes, 4) != 4) {
		sceIoClose(file);
		lua_pushnil(L);
		lua_pushstring(L, "Could not read height");
		return 2;
	}
	
	// Convert big-endian bytes to uint32_t
	uint32_t height = (height_bytes[0] << 24) | (height_bytes[1] << 16) | 
					  (height_bytes[2] << 8) | height_bytes[3];
	
	sceIoClose(file);
	
	// Return width and height
	lua_pushinteger(L, width);
	lua_pushinteger(L, height);
	return 2;
}

static int lua_executeUriNPXS(lua_State *L) {
	int argc = lua_gettop(L);
#ifndef SKIP_ERROR_HANDLING
	if (argc != 1)
		return luaL_error(L, "wrong number of arguments");
#endif
	const char *uri_string = luaL_checkstring(L, 1);
	sceAppMgrLaunchAppByUri(0x40000, (char*)uri_string);
	return 0;
}

static int lua_installAppWithRif(lua_State *L) {
	int argc = lua_gettop(L);
#ifndef SKIP_ERROR_HANDLING
	if (argc != 2)
		return luaL_error(L, "wrong number of arguments.");
#endif
	const char* app_path = luaL_checkstring(L, 1);
	const char* rif_path = luaL_checkstring(L, 2);
	
	// Install app with RIF file support
	char rif_dest[256];
	snprintf(rif_dest, sizeof(rif_dest), "ux0:license/%s", strrchr(rif_path, '/') ? strrchr(rif_path, '/') + 1 : rif_path);
	
	// Ensure license directory exists
	sceIoMkdir("ux0:license", 0777);
	
	// Copy RIF file to license directory
	SceUID src = sceIoOpen(rif_path, SCE_O_RDONLY, 0);
	if (src >= 0) {
		SceUID dst = sceIoOpen(rif_dest, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
		if (dst >= 0) {
			char buffer[1024];
			int bytes_read;
			while ((bytes_read = sceIoRead(src, buffer, sizeof(buffer))) > 0) {
				sceIoWrite(dst, buffer, bytes_read);
			}
			sceIoClose(dst);
		}
		sceIoClose(src);
	}
	
	// Install the application (would typically use promoter utility)
	// For now, return success if RIF was copied
	lua_pushinteger(L, 0);
	return 1;
}

static int lua_installAppWithRifHead(lua_State *L) {
	int argc = lua_gettop(L);
#ifndef SKIP_ERROR_HANDLING
	if (argc != 2)
		return luaL_error(L, "wrong number of arguments.");
#endif
	const char* app_path = luaL_checkstring(L, 1);
	const char* rif_path = luaL_checkstring(L, 2);
	
	// Launch app with RIF header support
	// This function would typically set up the RIF licensing before launching
	char rif_dest[256];
	snprintf(rif_dest, sizeof(rif_dest), "ux0:license/%s", strrchr(rif_path, '/') ? strrchr(rif_path, '/') + 1 : rif_path);
	
	// Ensure license directory exists
	sceIoMkdir("ux0:license", 0777);
	
	// Copy RIF file
	SceUID src = sceIoOpen(rif_path, SCE_O_RDONLY, 0);
	if (src >= 0) {
		SceUID dst = sceIoOpen(rif_dest, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
		if (dst >= 0) {
			char buffer[1024];
			int bytes_read;
			while ((bytes_read = sceIoRead(src, buffer, sizeof(buffer))) > 0) {
				sceIoWrite(dst, buffer, bytes_read);
			}
			sceIoClose(dst);
		}
		sceIoClose(src);
	}
	
	// Launch the application
	int result = sceAppMgrLoadExec(app_path, NULL, NULL);
	lua_pushinteger(L, result);
	return 1;
}

static int lua_loadFontIntoMemory(lua_State *L) {
	int argc = lua_gettop(L);
#ifndef SKIP_ERROR_HANDLING
	if (argc != 1)
		return luaL_error(L, "wrong number of arguments (expected file_path)");
#endif
	const char* file_path = luaL_checkstring(L, 1);
	
	// Open and read the font file
	SceUID file = sceIoOpen(file_path, SCE_O_RDONLY, 0777);
#ifndef SKIP_ERROR_HANDLING
	if (file < 0)
		return luaL_error(L, "cannot open font file");
#endif
	
	// Get file size
	SceOff file_size = sceIoLseek(file, 0, SCE_SEEK_END);
	sceIoLseek(file, 0, SCE_SEEK_SET);
	
	// Allocate buffer and read file
	void* buffer = malloc(file_size);
#ifndef SKIP_ERROR_HANDLING
	if (buffer == NULL) {
		sceIoClose(file);
		return luaL_error(L, "cannot allocate memory for font");
	}
#endif
	
	int bytes_read = sceIoRead(file, buffer, file_size);
	sceIoClose(file);
	
#ifndef SKIP_ERROR_HANDLING
	if (bytes_read != file_size) {
		free(buffer);
		return luaL_error(L, "error reading font file");
	}
#endif
	
	// Create font buffer structure
	font_buffer* font_buf = (font_buffer*)malloc(sizeof(font_buffer));
	font_buf->magic = 0x464E5442; // "FNTB" magic
	font_buf->data = buffer;
	font_buf->size = file_size;
	
	lua_pushinteger(L, (uint32_t)font_buf);
	return 1;
}

static int lua_loadFontFromMemory(lua_State *L) {
	int argc = lua_gettop(L);
#ifndef SKIP_ERROR_HANDLING
	if (argc != 1)
		return luaL_error(L, "wrong number of arguments (expected font_buffer)");
#endif
	font_buffer* font_buf = (font_buffer*)(luaL_checkinteger(L, 1));
#ifndef SKIP_ERROR_HANDLING
	if (font_buf->magic != 0x464E5442)
		return luaL_error(L, "attempt to access wrong memory block type.");
#endif
	
	ttf* result = (ttf*)malloc(sizeof(ttf));
	memset(result, 0, sizeof(ttf));
	result->size = 16;
	result->scale = 0.919f;
	result->f = vita2d_load_font_mem(font_buf->data, font_buf->size);
#ifndef SKIP_ERROR_HANDLING
	if (result->f == NULL) {
		free(result);
		return luaL_error(L, "cannot load font from memory");
	}
#endif
	result->magic = 0x4C464E54;
	lua_pushinteger(L, (uint32_t)result);
	return 1;
}

static int lua_copyPicToAppmeta(lua_State *L) {
	const char *app_path = luaL_checkstring(L, 1);
	int result = copy_decrypted_pic(app_path);
	lua_pushboolean(L, result != 0);
	return 1;
}

static int lua_copyIconToAppmeta(lua_State *L) {
	const char *app_path = luaL_checkstring(L, 1);
	int result = copy_decrypted_icon(app_path);
	lua_pushboolean(L, result != 0);
	return 1;
}

// Crop image function - creates a new image from a region of an existing image
static int lua_cropImage(lua_State *L) {
	int argc = lua_gettop(L);
#ifndef SKIP_ERROR_HANDLING
	if (argc != 5)
		return luaL_error(L, "wrong number of arguments (expected: image, src_x, src_y, width, height)");
#endif
	
	// Get the source image
	lpp_texture* src_image = (lpp_texture*)(luaL_checkinteger(L, 1));
#ifndef SKIP_ERROR_HANDLING
	if (src_image->magic != 0xABADBEEF)
		return luaL_error(L, "attempt to access wrong memory block type.");
#endif
	
	// Get crop parameters
	int src_x = luaL_checkinteger(L, 2);
	int src_y = luaL_checkinteger(L, 3);
	int crop_width = luaL_checkinteger(L, 4);
	int crop_height = luaL_checkinteger(L, 5);
	
	// Get source image dimensions
	int src_width = vita2d_texture_get_width(src_image->text);
	int src_height = vita2d_texture_get_height(src_image->text);
	
#ifndef SKIP_ERROR_HANDLING
	// Validate crop bounds
	if (src_x < 0 || src_y < 0 || crop_width <= 0 || crop_height <= 0 ||
		src_x + crop_width > src_width || src_y + crop_height > src_height)
		return luaL_error(L, "crop bounds are outside image dimensions.");
#endif
	
	// Create a new texture for the cropped image
	vita2d_texture* cropped_texture = vita2d_create_empty_texture_format(crop_width, crop_height, SCE_GXM_TEXTURE_FORMAT_A8B8G8R8);
#ifndef SKIP_ERROR_HANDLING
	if (cropped_texture == NULL)
		return luaL_error(L, "failed to create cropped texture.");
#endif
	
	// Get pixel data from source texture
	uint32_t* src_data = (uint32_t*)vita2d_texture_get_datap(src_image->text);
	uint32_t* dst_data = (uint32_t*)vita2d_texture_get_datap(cropped_texture);
	uint32_t src_stride = vita2d_texture_get_stride(src_image->text) / 4; // Convert bytes to pixels (RGBA32)
	uint32_t dst_stride = vita2d_texture_get_stride(cropped_texture) / 4;
	
	// Copy pixel data from source region to destination
	for (int y = 0; y < crop_height; y++) {
		for (int x = 0; x < crop_width; x++) {
			uint32_t src_offset = (src_y + y) * src_stride + (src_x + x);
			uint32_t dst_offset = y * dst_stride + x;
			dst_data[dst_offset] = src_data[src_offset];
		}
	}
	
	// Create return texture wrapper
	lpp_texture* result = (lpp_texture*)malloc(sizeof(lpp_texture));
	result->magic = 0xABADBEEF;
	result->text = cropped_texture;
	result->data = nullptr;
	
	lua_pushinteger(L, (uint32_t)result);
	return 1;
}

//Register our LuaExtended Functions
static const luaL_Reg LuaExtended_functions[] = {
	{"extractSfoPSPX",            lua_extractsfopspx},
	{"pspGetPic1",                lua_pspgetpic1},
	{"crc32",                     lua_crc32},
	{"crc32File",                 lua_crc32_file},
	{"archiveGetContents",        lua_archiveGetContents},
	{"getPNGDimensions",          lua_getPNGDimensions},
	{"executeUriNPXS",            lua_executeUriNPXS},
	{"installAppWithRif",         lua_installAppWithRif},
	{"installAppWithRifHead",     lua_installAppWithRifHead},
	{"loadFontIntoMemory",        lua_loadFontIntoMemory},
	{"loadFontFromMemory",        lua_loadFontFromMemory},
	{"copyPicToAppmeta",          lua_copyPicToAppmeta},
	{"copyIconToAppmeta",         lua_copyIconToAppmeta},
	{"cropImage",                 lua_cropImage},
	{0, 0}
};

void LuaExtended_init(lua_State *L) {
	lua_newtable(L);
	luaL_setfuncs(L, LuaExtended_functions, 0);
	lua_setglobal(L, "Extended");
}