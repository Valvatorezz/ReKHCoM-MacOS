"""Adapt platform services of the upstream launcher, preserving its actual UI.

The checked function replacements are staged under build/. Deferred HD imports
and online achievements have explicit explanatory pages, not fake working controls.
"""
from pathlib import Path
import concurrent.futures
import re
import subprocess

ROOT=Path(__file__).resolve().parents[2]
HERE=Path(__file__).resolve().parent
UI=ROOT/'port/decompui'


def replace_function(text, signature, replacement):
    start=text.index(signature);brace=text.index('{',start);depth=0
    tokens=re.finditer(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]',text[brace:],re.S)
    for match in tokens:
        if match[0]=='{':depth+=1
        if match[0]=='}':
            depth-=1
            if depth==0:return text[:start]+replacement+text[brace+match.end():]
    raise RuntimeError('Unterminated upstream function: '+signature)


def prepare(out):
    out.mkdir(parents=True,exist_ok=True)
    text=(UI/'decompui.cpp').read_text()
    start=text.index('#define WIN32_LEAN_AND_MEAN');end=text.index('#include <SDL3/SDL.h>',start)
    text=text[:start]+text[end:]
    text=text.replace('namespace fs = std::filesystem;','namespace fs = std::filesystem;\n#include "launcher_platform.mm"\nstatic fs::path sAssets;')
    text=text.replace('#include "../achievements/ra_client.h"','')
    text=replace_function(text,'static std::string Sha1Hex(','''static std::string Sha1Hex(const std::vector<unsigned char>& data) {
        unsigned char digest[20];CC_SHA1(data.data(),(CC_LONG)data.size(),digest);
        std::string hex;char b[3];for(auto v:digest){std::snprintf(b,3,"%02x",v);hex+=b;}return hex;
    }''')
    text=replace_function(text,'static void ScanInstall(','''static void ScanInstall() {
        for(int v=0;v<kVersionCount;v++) {
            std::vector<unsigned char> bytes;
            sState[v].installed=ReadFile(RomPath(v),bytes)&&Sha1Hex(bytes)==kVersions[v].sha1;
            sState[v].exe=MacExecutableRoot()/(std::string("khcom_")+kVersions[v].id);
        }
    }''')
    text=replace_function(text,'static void PickRom(','''static void PickRom(SDL_Window*) { auto p=MacPickFile(false);if(!p.empty())InstallRom(p); }''')
    text=replace_function(text,'static void ImportSave(','''static void ImportSave(SDL_Window*,int v) { auto p=MacPickFile(true);if(!p.empty())ImportSavePath(p,v); }''')
    text=replace_function(text,'static std::vector<std::pair<int, int>> DisplayModes(','''static std::vector<std::pair<int,int>> DisplayModes() { return {}; }''')
    text=text.replace('PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_SPANISH', '[[[NSLocale preferredLanguages] firstObject] hasPrefix:@"es"]')
    text=replace_function(text,'static bool BackupSave(','''static bool BackupSave(int v) {
        auto now=std::chrono::system_clock::now();auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        fs::path folder=sRoot/"backups";std::error_code ec;fs::create_directories(folder,ec);
        static unsigned serial=0;
        auto target=folder/(SavePath(v).stem().string()+"-"+std::to_string(ms)+"-"+std::to_string(++serial)+".sav");
        bool ok=fs::copy_file(SavePath(v),target,fs::copy_options::none,ec);
        Notice(ok?"Save backed up to "+target.string():"Save backup failed; original preserved.",!ok);return ok;
    }''')
    text=text.replace('    wchar_t windir[MAX_PATH];\n    GetWindowsDirectoryW(windir, MAX_PATH);\n    fs::path fonts = fs::path(windir) / "Fonts";\n    std::string regular = (fonts / "segoeui.ttf").string();\n    std::string semibold = (fonts / "seguisb.ttf").string();',
                      '    std::string regular="/System/Library/Fonts/Supplemental/Arial.ttf";\n    std::string semibold="/System/Library/Fonts/Supplemental/Arial Bold.ttf";')
    start=text.index('            FILETIME ft, local;')
    end=re.search(r'^        } else \{',text[start:],re.M).start()+start
    text=text[:start]+'            struct stat details;\n            if(!stat(save.c_str(),&details)) {\n                struct tm when;localtime_r(&details.st_mtime,&when);\n                std::snprintf(line,sizeof(line),"%s %s - %02d/%02d/%04d %02d:%02d",T("Partida:","Save:"),\n                    save.filename().string().c_str(),when.tm_mday,when.tm_mon+1,when.tm_year+1900,when.tm_hour,when.tm_min);\n            } else std::snprintf(line,sizeof(line),"%s %s",T("Partida:","Save:"),save.filename().string().c_str());\n'+text[end:]
    start=text.index('        DEVMODEW cur;');end=text.index('        if (Ini("fullscreen")',start)
    text=text[:start]+'        const SDL_DisplayMode* current=SDL_GetDesktopDisplayMode(SDL_GetPrimaryDisplay());\n        if(current){W=current->w;H=current->h;}\n'+text[end:]
    text=text.replace('sRoot / "assets"','sAssets')
    text=text.replace('ShellExecuteW(nullptr, L"explore", sRoot.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL);','MacReveal(sRoot);')
    # Game host currently offers desktop fullscreen, not exclusive resolution switching.
    start=text.index('    {\n        /* fullscreen resolution:');end=text.index('    int ws =',start)
    text=text[:start]+'    ImGui::TextDisabled("Fullscreen uses the current display desktop resolution.");\n'+text[end:]
    start=text.index('    changed |= CycleRow(T("Estado en Discord"');end=text.index('    ImGui::EndChild();',start)
    text=text[:start]+text[end:]
    text=text.replace('#include "hd_import.inc"','''static bool HdModCard() {
        ImGui::TextWrapped("Replacement HD music and movies are deferred on macOS. Original cartridge audio and movies remain available.");return false;
    }''')
    text=text.replace('#include "achievements_page.inc"','''static int sAchievementFilter;
    static bool sAchievementPreview,sRaAccountShot;static Uint64 sAchievementPreviewStart;
    static void AchievementPreviewDraw() {}
    static void PageAchievements() { ImGui::TextWrapped("RetroAchievements integration is deferred on macOS. Shared achievement code and data are retained for future work."); }
    ''')
    text=text.replace('    KhRaPump(sRaUi, 0);','')
    text=text.replace('    sRaUi = KhRaCreate(sRoot.u8string().c_str(), nullptr, nullptr, nullptr);','')
    for line in ['    KhRaDestroy(sRaUi);','    sRaUi = nullptr;','    AchievementBadgesDestroy();','    SecureZeroMemory(sRaPassword, sizeof(sRaPassword));']:text=text.replace(line,'')
    text=replace_function(text,'static void CreateDesktopShortcut() {','''static void CreateDesktopShortcut() { MacReveal(MacExecutableRoot().parent_path().parent_path()); }''')
    text=text.replace('"Desktop shortcut"','"Show app in Finder"').replace('"A \\"ReKHCoM\\" shortcut that opens the game without the launcher."','"Reveal the application in Finder; make an alias or drag it to the Dock."')
    text=replace_function(text,'static void Launch(','''static void Launch(SDL_Window* window) {
        const auto& st=sState[sSelected];SaveIni();SaveLauncherState();
        std::string executable=st.exe.string(),data=sRoot.string(),rom=RomPath(sSelected).string();
        char* args[]={executable.data(),(char*)"--data-dir",data.data(),(char*)"--rom",rom.data(),nullptr};
        pid_t child;int error=posix_spawn(&child,executable.c_str(),nullptr,nullptr,args,environ);
        if(error){Notice(std::string("Cannot launch game: ")+strerror(error),true);return;}
        SDL_HideWindow(window);int status=0;
        while(waitpid(child,&status,WNOHANG)==0){SDL_Event e;while(SDL_PollEvent(&e)){} SDL_Delay(50);}
        SDL_ShowWindow(window);SDL_RaiseWindow(window);LoadIni();
        if(!WIFEXITED(status)||WEXITSTATUS(status))Notice("The game stopped with an error. See kh_com_host.log and kh_com_crash.log.",true);
    }''')
    text=replace_function(text,'static fs::path ExeDir(','''static fs::path ExeDir() { return MacDataRoot(); }''')
    text=replace_function(text,'static void UnblockOwnFiles(','''static void UnblockOwnFiles() {} /* normal macOS per-app approval; never remove quarantine */''')
    start=text.index('    /* ReKHCoM.exe --hd-import');end=text.index('    ScanInstall();',start)
    text=text[:start]+text[end:]
    text=text.replace('    sRoot = ExeDir();','''    sRoot=ExeDir();sAssets=MacAssetsRoot();MacDefaults(sRoot);
        for(int j=1;j+1<argc;j++)if(!strcmp(argv[j],"--rom"))InstallRom(fs::u8path(argv[++j]));
        if(const char* p=SDL_getenv("DECOMPUI_INSTALL"))InstallRom(fs::u8path(p));''')
    text=text.replace('    SaveLauncherState(); /* the first time, keeps the language chosen from the system */','''    SaveLauncherState(); /* the first time, keeps the language chosen from the system */
    if(SDL_getenv("DECOMPUI_CHECK")) {
        if(SDL_getenv("DECOMPUI_TEST_HD")){Ini("hd_text")=1;SaveIni();if(ReadIniFile()["hd_text"]!=1)return 2;}
        bool ok=sState[sSelected].installed&&fs::exists(sState[sSelected].exe)&&fs::exists(sAssets/"fonts/KHMenu.otf");
        fprintf(stdout,"Launcher ROM, executable, font and settings check: %s\\n",ok?"PASS":"FAIL");return ok?0:1;
    }''')
    # macOS requires a forward-compatible 3.2 core context (GLSL 150).
    text=text.replace('SDL_GL_CONTEXT_MINOR_VERSION, 0','SDL_GL_CONTEXT_MINOR_VERSION, 2')
    text=text.replace('    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);','''    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);''')
    text=text.replace('"#version 130"','"#version 150"')
    # Screenshots should use physical drawable dimensions, but the UI is in logical points.
    text=text.replace('    sScale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());','    sScale = 1.0f;')
    file=out/'launcher.mm';file.write_text(text)
    return file


def build(out,exe,sdl,jobs,run):
    source=prepare(out/'launcher')
    imgui=UI/'third_party/imgui'
    sources=[source,*[imgui/f for f in ['imgui.cpp','imgui_draw.cpp','imgui_tables.cpp','imgui_widgets.cpp']],
             imgui/'backends/imgui_impl_sdl3.cpp',imgui/'backends/imgui_impl_opengl3.cpp']
    flags=['-arch','arm64','-mmacosx-version-min=15.0','-std=c++17','-O2','-Wno-deprecated-declarations',
           '-I'+str(HERE),'-I'+str(UI),'-I'+str(imgui),'-I'+str(imgui/'backends'),'-I'+str(UI/'third_party'),'-I'+str(sdl/'include')]
    def one(src):
        obj=out/'launcher'/(src.stem+'.o');run(['clang++',*flags,'-c',src,'-o',obj],stderr=subprocess.PIPE);return obj
    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:objects=list(pool.map(one,sources))
    run(['clang++','-arch','arm64','-mmacosx-version-min=15.0',*objects,'-L'+str(sdl/'lib'),'-lSDL3','-framework','Cocoa',
         '-framework','OpenGL','-Wl,-rpath,@executable_path/../Frameworks','-o',exe])
