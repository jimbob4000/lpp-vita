/*----------------------------------------------------------------------------------------------------------------------#
#- Code taken and modified from: cy33hc's Vita Launcher ---------------------------------------------------------------#
#- under GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007 -----------------------------------------------------------#
#- https://github.com/cy33hc/vita-launcher ----------------------------------------------------------------------------#
#- Credit to original author: cy33hc ----------------------------------------------------------------------------------#
#----------------------------------------------------------------------------------------------------------------------*/

#include <vitasdk.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include "eboot.h"

typedef struct {
    char signature[4];
    int  version;
    int  offset[8];
} HEADER;

namespace EBOOT {
    
    int ExtractSfo(const char* eboot_path, const char* sfo_output_path) {
        SceUID infile;
        SceUID outfile;
        HEADER header;
        
        infile = sceIoOpen(eboot_path, SCE_O_RDONLY, 0777);
        if (infile < 0) return -1;
        
        sceIoRead(infile, &header, sizeof(HEADER));
        
        if (header.signature[0] != 0x00 ||
            header.signature[1] != 0x50 ||
            header.signature[2] != 0x42 ||
            header.signature[3] != 0x50 ||
            header.offset[0] != 0x28)
        {
            // Invalid eboot
            sceIoClose(infile);
            return -1;
        }
        
        // Get the size of param.sfo (first file)
        int sfo_size = header.offset[1] - header.offset[0];
        sceIoLseek(infile, header.offset[0], SCE_SEEK_SET);
        
        outfile = sceIoOpen(sfo_output_path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
        if (outfile < 0) {
            sceIoClose(infile);
            return -1;
        }
        
        // Copy SFO data in chunks
        char buffer[1024];
        int remaining = sfo_size;
        while (remaining > 0) {
            int read_size = (remaining > 1024) ? 1024 : remaining;
            int bytes_read = sceIoRead(infile, buffer, read_size);
            if (bytes_read <= 0) break;
            
            sceIoWrite(outfile, buffer, bytes_read);
            remaining -= bytes_read;
        }
        
        sceIoClose(outfile);
        sceIoClose(infile);
        
        return 0;
    }
    
    void* ExtractSfoToMemory(const char* eboot_path, uint32_t* out_size) {
        if (out_size) *out_size = 0;
        
        SceUID infile;
        HEADER header;
        
        infile = sceIoOpen(eboot_path, SCE_O_RDONLY, 0777);
        if (infile < 0) return nullptr;
        
        sceIoRead(infile, &header, sizeof(HEADER));
        
        if (header.signature[0] != 0x00 ||
            header.signature[1] != 0x50 ||
            header.signature[2] != 0x42 ||
            header.signature[3] != 0x50 ||
            header.offset[0] != 0x28)
        {
            // Invalid eboot
            sceIoClose(infile);
            return nullptr;
        }
        
        // Get the size of param.sfo (first file)
        int sfo_size = header.offset[1] - header.offset[0];
        sceIoLseek(infile, header.offset[0], SCE_SEEK_SET);
        
        // Allocate memory for SFO data
        void* sfo_data = malloc(sfo_size);
        if (!sfo_data) {
            sceIoClose(infile);
            return nullptr;
        }
        
        // Read SFO data directly into memory
        int bytes_read = sceIoRead(infile, sfo_data, sfo_size);
        sceIoClose(infile);
        
        if (bytes_read != sfo_size) {
            free(sfo_data);
            return nullptr;
        }
        
        if (out_size) *out_size = sfo_size;
        return sfo_data;
    }
}
