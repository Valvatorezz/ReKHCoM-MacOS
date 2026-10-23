#ifndef REKHCOM_MACOS_CRASH_DIAGNOSTICS_H
#define REKHCOM_MACOS_CRASH_DIAGNOSTICS_H
/* Native faults are recorded, then delivered to macOS's normal crash reporter.
 * Do not turn an arbitrary native fault into a successful game-memory access. */
#include <signal.h>
#include <sys/ucontext.h>
#include <fcntl.h>
static int sCrashFd=-1;
static unsigned char sCrashStack[65536] __attribute__((aligned(16)));
static char* CrashText(char* out,const char* s) { while(*s)*out++=*s++;return out; }
static char* CrashNumber(char* out,uintptr_t n) {
    static const char digits[]="0123456789abcdef";
    out=CrashText(out,"0x");
    for(int shift=(int)(sizeof(n)*8)-4;shift>=0;shift-=4)*out++=digits[(n>>shift)&15];
    return out;
}
static void MacCrashRecord(int signalNumber,siginfo_t* info,void* context) {
    char message[256],*out=message;
    out=CrashText(out,"native crash: signal=");out=CrashNumber(out,(uintptr_t)signalNumber);
    out=CrashText(out," fault=");out=CrashNumber(out,info?(uintptr_t)info->si_addr:0);
#if defined(__APPLE__) && defined(__aarch64__)
    if(context) {
        ucontext_t* uc=context;
        out=CrashText(out," pc=");out=CrashNumber(out,uc->uc_mcontext->__ss.__pc);
        out=CrashText(out," lr=");out=CrashNumber(out,uc->uc_mcontext->__ss.__lr);
    }
#endif
    *out++='\n';
    if(sCrashFd>=0)(void)write(sCrashFd,message,(size_t)(out-message));
    (void)write(STDERR_FILENO,message,(size_t)(out-message));
}
static void MacCrashSignal(int signalNumber,siginfo_t* info,void* context) {
    MacCrashRecord(signalNumber,info,context);
    /* SA_RESETHAND restored the default disposition. For a synchronous CPU
       fault return to the original instruction: macOS then reports its real
       faulting context. User-delivered signals need explicit re-delivery. */
    if(!info || info->si_code<=0)kill(getpid(),signalNumber);
}
static void MacCrashInstall(const char* data) {
    char path[4096];snprintf(path,sizeof(path),"%s/kh_com_crash.log",data);
    sCrashFd=open(path,O_WRONLY|O_CREAT|O_APPEND,0600);
    stack_t stack={0};stack.ss_sp=sCrashStack;stack.ss_size=sizeof(sCrashStack);sigaltstack(&stack,NULL);
    struct sigaction action={0};sigemptyset(&action.sa_mask);
    action.sa_sigaction=MacCrashSignal;action.sa_flags=SA_SIGINFO|SA_ONSTACK|SA_RESETHAND;
    const int signals[]={SIGSEGV,SIGBUS,SIGILL,SIGFPE,SIGABRT};
    for(size_t i=0;i<sizeof(signals)/sizeof(signals[0]);i++)sigaction(signals[i],&action,NULL);
}
#endif
