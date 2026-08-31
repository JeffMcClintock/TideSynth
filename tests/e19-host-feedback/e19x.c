// e19x -- the minimum X11 automation E19's linux cell needs, by WINDOW ID.
//
// _NET_CLIENT_LIST is absent under weston (docs/ci/headless-gui-verification.md),
// so nothing here looks a window up by WM_CLASS: ids come from `xwininfo -root
// -tree`, and every verb takes one.
//
//   e19x shot   <winid> <out.ppm>          XGetImage the window, write P6
//   e19x geom   <winid>                    "x y w h" in root coords
//   e19x click  <winid> <x> <y> <n> [btn]  XTest click at window-relative x,y
//   e19x key    <keysym-name> [<n>]        XTest key press/release
//
// A ROOT grab returns garbage under XWayland; a per-window XGetImage does not.
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int shot(Display* d, Window w, const char* out)
{
    XWindowAttributes a;
    if (!XGetWindowAttributes(d, w, &a)) { fprintf(stderr, "no such window\n"); return 2; }
    XImage* im = XGetImage(d, w, 0, 0, a.width, a.height, AllPlanes, ZPixmap);
    if (!im) { fprintf(stderr, "XGetImage failed\n"); return 2; }
    FILE* f = fopen(out, "wb");
    if (!f) { perror("fopen"); return 2; }
    fprintf(f, "P6\n%d %d\n255\n", a.width, a.height);
    for (int y = 0; y < a.height; ++y)
        for (int x = 0; x < a.width; ++x)
        {
            unsigned long p = XGetPixel(im, x, y);
            unsigned char rgb[3] = {
                (unsigned char)((p & im->red_mask)   >> 16),
                (unsigned char)((p & im->green_mask) >> 8),
                (unsigned char)( p & im->blue_mask)
            };
            fwrite(rgb, 1, 3, f);
        }
    fclose(f);
    XDestroyImage(im);
    printf("%d %d\n", a.width, a.height);
    return 0;
}

int main(int argc, char** argv)
{
    if (argc < 2) { fprintf(stderr, "usage: e19x shot|geom|click|key ...\n"); return 2; }
    Display* d = XOpenDisplay(NULL);
    if (!d) { fprintf(stderr, "cannot open display\n"); return 2; }
    int rc = 0;

    if (!strcmp(argv[1], "shot") && argc == 4)
        rc = shot(d, (Window)strtoul(argv[2], NULL, 0), argv[3]);
    else if (!strcmp(argv[1], "geom") && argc == 3)
    {
        Window w = (Window)strtoul(argv[2], NULL, 0);
        XWindowAttributes a; Window child; int rx, ry;
        if (!XGetWindowAttributes(d, w, &a)) { fprintf(stderr, "no such window\n"); rc = 2; }
        else
        {
            XTranslateCoordinates(d, w, DefaultRootWindow(d), 0, 0, &rx, &ry, &child);
            printf("%d %d %d %d\n", rx, ry, a.width, a.height);
        }
    }
    else if (!strcmp(argv[1], "click") && (argc == 6 || argc == 7))
    {
        Window w = (Window)strtoul(argv[2], NULL, 0);
        int x = atoi(argv[3]), y = atoi(argv[4]), n = atoi(argv[5]);
        int btn = (argc == 7) ? atoi(argv[6]) : 1;
        Window child; int rx, ry;
        XTranslateCoordinates(d, w, DefaultRootWindow(d), x, y, &rx, &ry, &child);
        XTestFakeMotionEvent(d, DefaultScreen(d), rx, ry, 0);
        XFlush(d); usleep(60000);
        for (int i = 0; i < n; ++i)
        {
            XTestFakeButtonEvent(d, btn, True, 0);
            XTestFakeButtonEvent(d, btn, False, 0);
            XFlush(d);
            usleep(40000);
        }
        printf("clicked btn%d %d,%d root %d,%d x%d\n", btn, x, y, rx, ry, n);
    }
    else if (!strcmp(argv[1], "key") && argc >= 3)
    {
        KeySym ks = XStringToKeysym(argv[2]);
        if (ks == NoSymbol) { fprintf(stderr, "unknown keysym %s\n", argv[2]); rc = 2; }
        else
        {
            KeyCode kc = XKeysymToKeycode(d, ks);
            int n = (argc > 3) ? atoi(argv[3]) : 1;
            for (int i = 0; i < n; ++i)
            {
                XTestFakeKeyEvent(d, kc, True, 0);
                XTestFakeKeyEvent(d, kc, False, 0);
                XFlush(d);
                usleep(60000);
            }
            printf("key %s x%d\n", argv[2], n);
        }
    }
    else { fprintf(stderr, "bad verb/arity\n"); rc = 2; }

    XCloseDisplay(d);
    return rc;
}
