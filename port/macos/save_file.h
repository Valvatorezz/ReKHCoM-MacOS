#ifndef REKHCOM_SAVE_FILE_H
#define REKHCOM_SAVE_FILE_H
#include <filesystem>
#include <vector>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
namespace RekhcomSave {
/* Caller makes the backup first. Stage on the same filesystem, sync, then rename. */
inline bool Replace(const std::filesystem::path& target,const std::vector<unsigned char>& data) {
    std::string pattern=target.string()+".import-XXXXXX";
    std::vector<char> name(pattern.begin(),pattern.end());name.push_back(0);
    int fd=mkstemp(name.data());if(fd<0)return false;
    size_t done=0;bool ok=true;
    while(done<data.size()) {
        ssize_t n=write(fd,data.data()+done,data.size()-done);
        if(n<0&&errno==EINTR)continue;
        if(n<=0){ok=false;break;}done+=(size_t)n;
    }
    if(ok)ok=fsync(fd)==0;
    if(close(fd))ok=false;
    if(ok)ok=rename(name.data(),target.c_str())==0;
    if(!ok)unlink(name.data());
    return ok;
}
}
#endif
