/*---------------------------------------------------------------------------------------------------------------------#
#- Code taken and modified from: cy33hc's Vita Launcher ---------------------------------------------------------------#
#- under GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007 -----------------------------------------------------------#
#- https://github.com/cy33hc/vita-launcher ----------------------------------------------------------------------------#
#- Credit to original author: cy33hc ----------------------------------------------------------------------------------#
#----------------------------------------------------------------------------------------------------------------------*/

#ifndef LAUNCHER_EBOOT_H
#define LAUNCHER_EBOOT_H

namespace EBOOT {
    int ExtractSfo(const char* eboot_path, const char* sfo_output_path);
    void* ExtractSfoToMemory(const char* eboot_path, uint32_t* out_size);
}

#endif