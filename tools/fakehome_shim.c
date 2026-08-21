/* Redirect getpwuid()/getpwuid_r() to a scratch home so a runtime test cannot
   write into the developer's real one. BundleInfo::getUserDocumentFolder()
   deliberately uses getpwuid rather than $HOME, so HOME= is not enough. */
#define _GNU_SOURCE
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

static struct passwd shim;
static char dirbuf[4096];

struct passwd *getpwuid(uid_t uid)
{
    static struct passwd *(*real)(uid_t);
    if (!real) real = dlsym(RTLD_NEXT, "getpwuid");
    struct passwd *p = real(uid);
    if (!p) return p;
    shim = *p;
    const char *fake = getenv("FAKE_HOME");
    if (fake) { strncpy(dirbuf, fake, sizeof dirbuf - 1); shim.pw_dir = dirbuf; }
    return &shim;
}

int getpwuid_r(uid_t uid, struct passwd *pwd, char *buf, size_t buflen, struct passwd **result)
{
    static int (*real)(uid_t, struct passwd *, char *, size_t, struct passwd **);
    if (!real) real = dlsym(RTLD_NEXT, "getpwuid_r");
    int rc = real(uid, pwd, buf, buflen, result);
    const char *fake = getenv("FAKE_HOME");
    if (rc == 0 && *result && fake) { strncpy(dirbuf, fake, sizeof dirbuf - 1); (*result)->pw_dir = dirbuf; }
    return rc;
}

/*
 * Build and use (BACKLOG S7):
 *
 *   gcc -shared -fPIC -o fakehome.so tools/fakehome_shim.c -ldl
 *   LD_PRELOAD=./fakehome.so FAKE_HOME=/tmp/scratch HOME=/tmp/scratch ./TIDE-Rack
 *
 * Why it exists: BundleInfo::getUserDocumentFolder() resolves the user's home
 * through getpwuid(getuid())->pw_dir and NOT $HOME -- deliberately, so a
 * sandboxed macOS app sees the real home rather than its container. That means
 * HOME= alone does not sandbox a runtime test, and any test of "what does this
 * write into the user's home" would write into the DEVELOPER's home instead.
 *
 * Validate it before trusting a result from it: a probe calling getpwuid should
 * print FAKE_HOME with the shim preloaded and the real home without it.
 */
