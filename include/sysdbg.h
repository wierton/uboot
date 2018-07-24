#ifndef NEMU_MIPS32_SYSDBG
#define NEMU_MIPS32_SYSDBG

int sysdbg(const char *fmt, ...);

#define logd(fmt, ...) sysdbg("\e[31m[DEBUG]\e[0m %s:%s:%d: " fmt, __FILE__, __func__, __LINE__, ##__VA_ARGS__)

#endif
