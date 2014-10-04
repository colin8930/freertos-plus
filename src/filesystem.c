#include "osdebug.h"
#include "filesystem.h"
#include "fio.h"
#include "clib.h"
#include <stdint.h>
#include <string.h>
#include <hash-djb2.h>

#define MAX_FS 16

struct fs_t {
    uint32_t hash;
    fs_open_t cb;
    fs_ls_t lscb;			
    void * opaque;
};

static struct fs_t fss[MAX_FS];

__attribute__((constructor)) void fs_init() {
    memset(fss, 0, sizeof(fss));
}



int register_fs(const char * mountpoint, fs_open_t callback, fs_ls_t ls_callback, void * opaque) {
    int i;
    DBGOUT("register_fs(\"%s\", %p, %p)\r\n", mountpoint, callback, opaque);
    
    for (i = 0; i < MAX_FS; i++) {
        if (!fss[i].cb) {
            fss[i].hash = hash_djb2((const uint8_t *) mountpoint, -1);
            fss[i].lscb=ls_callback;							
            fss[i].cb = callback;
            fss[i].opaque = opaque;
            return 0;
        }
    }
    
    return -1;
}

int fs_open(const char * path, int flags, int mode) {
    const char * slash;
    uint32_t hash;
    int i;
//    DBGOUT("fs_open(\"%s\", %i, %i)\r\n", path, flags, mode);
    
    while (path[0] == '/')
        path++;
    
    slash = strchr(path, '/');
    
    if (!slash)
			
        return -2;

    hash = hash_djb2((const uint8_t *) path, slash - path);
    path = slash + 1;
			
    for (i = 0; i < MAX_FS; i++) {				
        if (fss[i].hash == hash){
								
            return fss[i].cb(fss[i].opaque, path, flags, mode);
        }
    }
   
    return -2;
}

int fs_ls(char* ls_list[]) {
                              
            return fss[0].lscb(fss[0].opaque, ls_list);

}