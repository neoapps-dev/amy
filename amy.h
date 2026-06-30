#ifndef AMY_H
#define AMY_H
#include <stddef.h>
#define AMY_MAGIC "AMY"
#define AMY_SR 44100
typedef enum {
    AMY_SINE,
    AMY_SQUARE,
    AMY_TRIANGLE,
    AMY_SAWTOOTH,
    AMY_NOISE,
    AMY_SILENCE,
} amy_wave_t;
typedef struct {
    amy_wave_t type;
    double freq;
    double amp;
    double dur;
    double start;
} amy_seg_t;
typedef struct {
    amy_seg_t *segs;
    int nsegs;
    double dur;
} amy_chan_t;
typedef struct {
    int nchans;
    amy_chan_t *chans;
    char **meta;
    int nmeta;
} amy_t;
int amy_parse(amy_t *a, const char *s, size_t n);
void amy_free(amy_t *a);
double amy_dur(const amy_t *a);
void amy_render(const amy_t *a, double t, float *buf, int nchans);
int amy_wav(const amy_t *a, const char *path, int repeat);
#ifdef AMY_HAVE_SDL
int amy_play(const amy_t *a, int repeat);
#endif
#endif
