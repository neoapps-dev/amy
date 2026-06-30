#include "amy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
static void usage(void)
{
    fprintf(stderr, "usage: amy [-w] [-l [N]] <file.amy> [output.wav]\n");
    fprintf(stderr, "  -w    write WAV file instead of playing\n");
    fprintf(stderr, "  -l    loop (WAV: 2x, SDL: infinite)\n");
    fprintf(stderr, "  -l N  loop N times\n");
}

int main(int argc, char **argv) {
    if (argc < 2) { usage(); return 1; }
    int write_wav = 0;
    int loop = 0;
    int repeat = 0;
    const char *inpath = NULL;
    const char *outpath = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-w") == 0) write_wav = 1;
        else if (strcmp(argv[i], "-l") == 0) {
            loop = 1;
            if (i + 1 < argc && isdigit((unsigned char)argv[i+1][0])) repeat = atoi(argv[++i]);
        }
        else if (!inpath) inpath = argv[i];
        else if (!outpath) outpath = argv[i];
    }

    if (!inpath) { usage(); return 1; }
    FILE *f = fopen(inpath, "rb");
    if (!f) { fprintf(stderr, "error: cannot open '%s'\n", inpath); return 1; }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *data = malloc(fsize);
    if (!data) { fclose(f); return 1; }
    if ((long)fread(data, 1, fsize, f) != fsize) { fclose(f); free(data); return 1; }
    fclose(f);
    amy_t amy;
    if (amy_parse(&amy, data, fsize) != 0) {
        fprintf(stderr, "error: parse failed\n");
        free(data);
        return 1;
    }
    free(data);
    printf("channels: %d, duration: %.2fs, metadata: %d\n", amy.nchans, amy_dur(&amy), amy.nmeta);
    if (write_wav) {
        int r = repeat > 0 ? repeat : (loop ? 2 : 1);
        if (r > 1) printf("looping %d times\n", r);
        char wavpath[1024];
        if (outpath) snprintf(wavpath, sizeof(wavpath), "%s", outpath);
        else snprintf(wavpath, sizeof(wavpath), "%.*s.wav", (int)(strlen(inpath) - 4), inpath);
        if (amy_wav(&amy, wavpath, r) != 0) {
            fprintf(stderr, "error: wav write failed\n");
            amy_free(&amy);
            return 1;
        }
        printf("wrote %s\n", wavpath);
    } else {
#ifdef AMY_HAVE_SDL
        int r = repeat > 0 ? repeat : (loop ? 0 : 1);
        if (r == 0) printf("looping\n");
        else if (r > 1) printf("looping %d times\n", r);
        if (amy_play(&amy, r) != 0) {
            fprintf(stderr, "error: playback failed\n");
            amy_free(&amy);
            return 1;
        }
#else
        fprintf(stderr, "error: SDL2 not available, use -w for WAV output\n");
        amy_free(&amy);
        return 1;
#endif
    }

    amy_free(&amy);
    return 0;
}
