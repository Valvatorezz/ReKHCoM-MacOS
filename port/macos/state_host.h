/* Experimental arm64/same-core state format. No native pointers are written.
 * Included after the bridge declarations and the generated core instance. */
#ifdef MACOS_SAVE_STATES
#include <zlib.h>
#include <fcntl.h>
#include "state_build.h"

enum { STATE_KEEP_MAX = 8, STATE_TEXTURE_LIMIT = 128 * 1024 * 1024 };
typedef struct { uint32_t offset, size; } StateKeep;
typedef struct {
    char magic[8], build[65], rom[41];
    uint32_t version, memorySize, textureCount, keepCount;
    int32_t logSlot, audioRate;
    uint32_t audioOpen, blend;
    uint64_t packedSize, rawSize;
    unsigned char digest[CC_SHA256_DIGEST_LENGTH];
    unsigned char dataRoot[CC_SHA256_DIGEST_LENGTH];
    StateKeep keep[STATE_KEEP_MAX];
    unsigned char metadataDigest[CC_SHA256_DIGEST_LENGTH];
} StateHeader;
typedef struct { uint32_t id, width, height, smooth; } StateTexture;
typedef struct {
    StateHeader header;
    unsigned char* bytes;
    SDL_Texture* textures[4096];
    unsigned char* pixels[4096];
    int widths[4096], heights[4096], smooth[4096];
} StateImage;

static StateKeep sStateKeep[STATE_KEEP_MAX];
static unsigned sStateKeepCount;
static int sStateOp;
static char sStatePath[4096];
static U32 sStateInitialStack;
static wasmTableEntry* sStateInitialTable;
static U32 sStateTableSize;
static StateImage* sStatePending;

static wasmMemory* StateMemory(void) { return m4_coreExport6_memory(&sCore); }
static void StateFree(StateImage* image) {
    if (!image) return;
    for (int i=0;i<4096;i++) {
        if(image->textures[i]) SDL_DestroyTexture(image->textures[i]);
        free(image->pixels[i]);
    }
    free(image->bytes); free(image);
}
static void StateInit(void) {
    sStateInitialStack=sCore.g0;
    free(sStateInitialTable);
    sStateTableSize=sCore.t0.size;
    sStateInitialTable=malloc((size_t)sStateTableSize*sizeof(*sStateInitialTable));
    if(!sStateInitialTable) Fatal("Cannot allocate save-state table baseline");
    memcpy(sStateInitialTable,sCore.t0.data,(size_t)sStateTableSize*sizeof(*sStateInitialTable));
    sStateKeepCount=0; sStateOp=0;
}
static int StateLogSlot(void) {
    int slot=-1;
    for(int i=0;i<64;i++) if(sFiles[i]) {
        const char* name=strrchr(sFileNames[i],'/');
        if(strcmp(name?name+1:sFileNames[i],"kh_com.log") || slot>=0) return -2;
        slot=i;
    }
    for(int i=0;i<16;i++) if(sDirs[i]) return -2;
    return slot;
}
static int StateReady(void) {
    return sStateKeepCount && sStateKeepCount<=STATE_KEEP_MAX && StateLogSlot()!=-2 &&
        sCore.t0.size==sStateTableSize &&
        !memcmp(sCore.t0.data,sStateInitialTable,(size_t)sStateTableSize*sizeof(*sStateInitialTable));
}
static void H_state_keep(const void* data,uint32_t size) {
    wasmMemory* memory=StateMemory();
    uint64_t offset=(const unsigned char*)data-memory->data;
    if(sStateKeepCount>=STATE_KEEP_MAX || offset>memory->size || size>memory->size-offset)
        Fatal("Invalid volatile save-state region");
    sStateKeep[sStateKeepCount++]=(StateKeep){(uint32_t)offset,size};
}
static int H_state_request(int op,const char* path) {
    if((op!=1 && op!=2) || !path || !*path || strlen(path)>=sizeof(sStatePath))return 0;
    SDL_strlcpy(sStatePath,path,sizeof(sStatePath));sStateOp=op;return 1;
}
static int StateSave(const char* path) {
    Uint64 started=SDL_GetTicksNS();
    wasmMemory* memory=StateMemory();
    StateHeader header={0};
    uint64_t size=memory->size, textureBytes=0;
    for(int i=0;i<4096;i++) if(sTex[i]) {
        if(!sTexPixels[i])return 0;
        textureBytes+=(uint64_t)sTexW[i]*sTexH[i]*4;
        size+=sizeof(StateTexture)+(uint64_t)sTexW[i]*sTexH[i]*4;
        header.textureCount++;
    }
    if(textureBytes>STATE_TEXTURE_LIMIT)return 0;
    unsigned char* raw=malloc((size_t)size);
    if(!raw)return 0;
    memcpy(raw,memory->data,(size_t)memory->size);
    for(unsigned i=0;i<sStateKeepCount;i++)memset(raw+sStateKeep[i].offset,0,sStateKeep[i].size);
    unsigned char* at=raw+memory->size;
    for(int i=0;i<4096;i++)if(sTex[i]) {
        StateTexture item={(uint32_t)i,(uint32_t)sTexW[i],(uint32_t)sTexH[i],(uint32_t)sTexSmooth[i]};
        size_t n=(size_t)item.width*item.height*4;
        memcpy(at,&item,sizeof(item));at+=sizeof(item);
        memcpy(at,sTexPixels[i],n);at+=n;
    }
    memcpy(header.magic,"MACSTATE",8);header.version=1;
    SDL_strlcpy(header.build,MACOS_STATE_BUILD_ID,sizeof(header.build));
    SDL_strlcpy(header.rom,MACOS_ROM_SHA1,sizeof(header.rom));
    header.memorySize=(uint32_t)memory->size;header.rawSize=size;
    header.keepCount=sStateKeepCount;memcpy(header.keep,sStateKeep,sStateKeepCount*sizeof(*sStateKeep));
    header.logSlot=StateLogSlot();header.audioRate=sGameRate;header.audioOpen=sAudio!=NULL;header.blend=sBlend;
    CC_SHA256(raw,(CC_LONG)size,header.digest);
    CC_SHA256(sData,(CC_LONG)strlen(sData),header.dataRoot);
    uLongf packedSize=compressBound((uLong)size);
    unsigned char* packed=malloc(packedSize);
    int ok=packed && compress2(packed,&packedSize,raw,(uLong)size,Z_BEST_SPEED)==Z_OK;
    free(raw);
    if(!ok){free(packed);return 0;}
    header.packedSize=packedSize;
    CC_SHA256(&header,(CC_LONG)offsetof(StateHeader,metadataDigest),header.metadataDigest);
    char temporary[4096];
    if(SDL_snprintf(temporary,sizeof(temporary),"%s.tmp.XXXXXX",path)>=(int)sizeof(temporary)){free(packed);return 0;}
    int fd=mkstemp(temporary);
    FILE* file=fd>=0?fdopen(fd,"wb"):NULL;
    if(!file){if(fd>=0){close(fd);unlink(temporary);}free(packed);return 0;}
    ok=fwrite(&header,1,sizeof(header),file)==sizeof(header) && fwrite(packed,1,packedSize,file)==packedSize;
    free(packed);
    if(fflush(file) || fsync(fd))ok=0;
    if(fclose(file))ok=0;
    if(ok && rename(temporary,path)==0) {
        Log("state: wrote %llu bytes in %.0f ms",(unsigned long long)(sizeof(header)+packedSize),(SDL_GetTicksNS()-started)/1000000.0);
        return 1;
    }
    unlink(temporary);return 0;
}
static StateImage* StateRead(const char* path) {
    FILE* file=fopen(path,"rb");
    if(!file)return NULL;
    StateImage* image=calloc(1,sizeof(*image));
    if(!image){fclose(file);return NULL;}
    StateHeader* h=&image->header;
    struct stat st;
    unsigned char rootDigest[CC_SHA256_DIGEST_LENGTH];
    CC_SHA256(sData,(CC_LONG)strlen(sData),rootDigest);
    int ok=!fstat(fileno(file),&st) && fread(h,1,sizeof(*h),file)==sizeof(*h);
    unsigned char metadataDigest[CC_SHA256_DIGEST_LENGTH];
    CC_SHA256(h,(CC_LONG)offsetof(StateHeader,metadataDigest),metadataDigest);
    uint64_t maxSize=StateMemory()->size+STATE_TEXTURE_LIMIT+4096*sizeof(StateTexture);
    ok=ok && !memcmp(h->metadataDigest,metadataDigest,sizeof(metadataDigest)) && !memcmp(h->magic,"MACSTATE",8) && h->version==1 &&
        !memcmp(h->build,MACOS_STATE_BUILD_ID,65) && !memcmp(h->rom,MACOS_ROM_SHA1,41) &&
        !memcmp(h->dataRoot,rootDigest,sizeof(rootDigest)) &&
        h->memorySize==StateMemory()->size && h->rawSize>=h->memorySize && h->rawSize<=maxSize &&
        h->packedSize<=compressBound((uLong)maxSize) && h->packedSize>0 &&
        (uint64_t)st.st_size==sizeof(*h)+h->packedSize && h->textureCount<=4096 &&
        h->keepCount==sStateKeepCount && !memcmp(h->keep,sStateKeep,sStateKeepCount*sizeof(*sStateKeep)) &&
        h->logSlot>=-1 && h->logSlot<64 && (h->logSlot<0)==(StateLogSlot()<0) &&
        h->audioRate==sGameRate && h->audioOpen==(uint32_t)(sAudio!=NULL) &&
        (h->blend==SDL_BLENDMODE_NONE || h->blend==SDL_BLENDMODE_BLEND || h->blend==SDL_BLENDMODE_ADD);
    unsigned char* packed=ok?malloc((size_t)h->packedSize):NULL;
    image->bytes=ok?malloc((size_t)h->rawSize):NULL;
    ok=ok && packed && image->bytes && fread(packed,1,(size_t)h->packedSize,file)==h->packedSize;
    fclose(file);
    uLongf length=(uLongf)h->rawSize;
    if(ok)ok=uncompress(image->bytes,&length,packed,(uLong)h->packedSize)==Z_OK && length==h->rawSize;
    free(packed);
    unsigned char digest[CC_SHA256_DIGEST_LENGTH];
    if(ok){CC_SHA256(image->bytes,(CC_LONG)h->rawSize,digest);ok=!memcmp(digest,h->digest,sizeof(digest));}
    uint64_t at=h->memorySize, total=0;
    for(uint32_t n=0;ok && n<h->textureCount;n++) {
        StateTexture item;
        if(h->rawSize-at<sizeof(item)){ok=0;break;}
        memcpy(&item,image->bytes+at,sizeof(item));at+=sizeof(item);
        uint64_t bytes=(uint64_t)item.width*item.height*4;
        if(item.id>=4096 || !item.width || !item.height || item.width>4096 || item.height>4096 ||
           item.smooth>1 || image->textures[item.id] || bytes>h->rawSize-at || (total+=bytes)>STATE_TEXTURE_LIMIT){ok=0;break;}
        int id=(int)item.id;
        image->pixels[id]=malloc((size_t)bytes);
        image->textures[id]=SDL_CreateTexture(sRenderer,SDL_PIXELFORMAT_ABGR8888,SDL_TEXTUREACCESS_STATIC,item.width,item.height);
        if(!image->pixels[id] || !image->textures[id]){ok=0;break;}
        memcpy(image->pixels[id],image->bytes+at,(size_t)bytes);at+=bytes;
        if(!SDL_UpdateTexture(image->textures[id],NULL,image->pixels[id],item.width*4)){ok=0;break;}
        image->widths[id]=item.width;image->heights[id]=item.height;image->smooth[id]=item.smooth;
    }
    if(!ok || at!=h->rawSize){StateFree(image);return NULL;}
    return image;
}
static int H_state_checkpoint(void) {
    int op=sStateOp;sStateOp=0;
    int result=-1;
    if(!op){sStateKeepCount=0;return 0;}
    if(!StateReady())Log("state: unsafe resources; request rejected");
    else if(op==1) {
        int ok=StateSave(sStatePath);
        result=ok?1:-1;
        Log("state: save %s -> %s (host frame %u)",sStatePath,ok?"ok":"failed",sFrames);
        H_notify(ok?"State saved (experimental)":"State save failed");
    } else {
        sStatePending=StateRead(sStatePath);
        if(sStatePending){Log("state: load validated %s (host frame %u)",sStatePath,sFrames);longjmp(sExit,2);}
        Log("state: rejected missing, incompatible or damaged file: %s",sStatePath);
        H_notify("State load rejected: missing, incompatible or damaged file");
    }
    sStateKeepCount=0;
    return result;
}
static void StateApply(void) {
    StateImage* image=sStatePending;sStatePending=NULL;
    wasmMemory* memory=StateMemory();
    /* Preserve live SRAM and its dirty-check/bank bookkeeping. Neither the
     * state image nor applying it can roll these back or write a .sav file. */
    for(unsigned n=0;n<sStateKeepCount;n++)memcpy(image->bytes+sStateKeep[n].offset,memory->data+sStateKeep[n].offset,sStateKeep[n].size);
    memcpy(memory->data,image->bytes,(size_t)memory->size);
    sCore.g0=sStateInitialStack; /* resume on a fresh guest stack */
    for(int i=0;i<4096;i++) {
        if(sTex[i])H_tex_free(i);
        sTex[i]=image->textures[i];image->textures[i]=NULL;
        sTexPixels[i]=image->pixels[i];image->pixels[i]=NULL;
        sTexW[i]=image->widths[i];sTexH[i]=image->heights[i];sTexSmooth[i]=image->smooth[i];
    }
    int log=StateLogSlot(), target=image->header.logSlot;
    if(log>=0 && log!=target){sFiles[target]=sFiles[log];sFiles[log]=NULL;strcpy(sFileNames[target],sFileNames[log]);}
    sBlend=(SDL_BlendMode)image->header.blend;
    H_audio_clear();sKeyHead=sKeyTail=0;
    sStateKeepCount=0;
    StateFree(image);
    m4_coreExport18_macosX5FstateX5Floaded(&sCore);
    Log("state: restored; resuming fresh game loop");
}
#else
static int H_state_request(int op,const char* path){(void)op;(void)path;return 0;}
static void H_state_keep(const void* data,uint32_t size){(void)data;(void)size;}
static int H_state_checkpoint(void){return 0;}
#endif
