/* macOS services for the upstream Dear ImGui launcher. */
#import <Cocoa/Cocoa.h>
#include <CommonCrypto/CommonDigest.h>
#include <spawn.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <chrono>
#include <ctime>
extern char** environ;
static fs::path MacDataRoot() {
    if(const char* p=SDL_getenv("KHCOM_DATA")) return fs::u8path(p);
    char* p=SDL_GetPrefPath("ReKHCoM","ReKHCoM");
    if(!p) throw std::runtime_error("Cannot locate Application Support");
    fs::path result=fs::u8path(p);SDL_free(p);return result;
}
static fs::path MacAssetsRoot() { return fs::u8path(SDL_GetBasePath())/"assets"; }
static fs::path MacExecutableRoot() { return fs::path([[[NSBundle mainBundle] executablePath] fileSystemRepresentation]).parent_path(); }
static fs::path MacPickFile(bool save) {
    @autoreleasepool {
        NSOpenPanel* panel=[NSOpenPanel openPanel];
        [panel setCanChooseFiles:YES];[panel setCanChooseDirectories:NO];[panel setAllowsMultipleSelection:NO];
        [panel setMessage:save?@"Choose a GBA save to import. Existing saves are backed up first.":@"Choose your own Kingdom Hearts: Chain of Memories cartridge dump."];
        if([panel runModal]==NSModalResponseOK) return fs::path([[panel URL] fileSystemRepresentation]);
        return {};
    }
}
static void MacReveal(const fs::path& path) {
    @autoreleasepool { [[NSWorkspace sharedWorkspace] activateFileViewerSelectingURLs:@[[NSURL fileURLWithPath:[NSString stringWithUTF8String:path.c_str()]]]]; }
}
static void MacDefaults(const fs::path& root) {
    fs::create_directories(root);
    if(fs::exists(root/"kh_com.ini"))return;
    std::ofstream f(root/"kh_com.ini");
    f<<"config_version=10\nview_mode=0\naspect=6\nfilter=0\nscale_mode=0\nhd_text=0\nsmooth_audio=0\n"
       "dodge_button=0\nstock_button=0\ndiscord_presence=0\nhd_movies=0\nhd_music=0\nhd_subtitles=0\n";
}
