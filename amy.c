#include "amy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef AMY_HAVE_SDL
#include <SDL2/SDL.h>
#endif
#include <ctype.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
static const char *g_waves[] = {
    "sine", "square", "triangle", "sawtooth", "noise", "silence"
};

static int wcmp(const char *s, int slen, const char *lit)
{
    for (int i = 0; i < slen; i++) {
        if (lit[i] == 0) return -1;
        char a = s[i], b = lit[i];
        if (a >= 'A' && a <= 'Z') a += 32;
        if (b >= 'A' && b <= 'Z') b += 32;
        if (a != b) return a - b;
    }
    return lit[slen] ? -1 : 0;
}

static int wave_type(const char *s, int len)
{
    for (int i = 0; i < 6; i++) if (wcmp(s, len, g_waves[i]) == 0) return i;
    return -1;
}

int amy_parse(amy_t *a, const char *s, size_t n)
{
    memset(a, 0, sizeof(*a));
    int nlines = 0;
    for (const char *p = s; p < s + n; p++) if (*p == '\n') nlines++;
    nlines++;
    const char **lines = malloc(nlines * sizeof(*lines));
    int *lens = malloc(nlines * sizeof(*lens));
    if (!lines || !lens) { free(lines); free(lens); return -1; }
    {
        int idx = 0;
        const char *p = s;
        for (int i = 0; i < nlines; i++) {
            lines[idx] = p;
            int len = 0;
            while (p < s + (int)n && *p != '\n') { p++; len++; }
            while (len > 0 && (lines[idx][len-1] == '\r' || lines[idx][len-1] == ' ')) len--;
            lens[idx] = len;
            idx++;
            if (p < s + (int)n) p++;
        }
        nlines = idx;
    }

    if (nlines < 1) { free(lines); free(lens); return -1; }
    if (lens[0] < 4 || memcmp(lines[0], AMY_MAGIC, 3) != 0 || lines[0][3] != ' ') { free(lines); free(lens); return -1; }
    {
        int pos = 4;
        while (pos < lens[0] && lines[0][pos] == ' ') pos++;
        int nch = 0;
        while (pos < lens[0] && lines[0][pos] >= '0' && lines[0][pos] <= '9') nch = nch * 10 + (lines[0][pos++] - '0');
        if (nch < 1) { free(lines); free(lens); return -1; }
        a->nchans = nch;
    }

    a->chans = calloc(a->nchans, sizeof(amy_chan_t));
    if (!a->chans) { free(lines); free(lens); return -1; }
    int chidx = 0;
    for (int i = 1; i < nlines; i++) {
        const char *ln = lines[i];
        int lnlen = lens[i];
        if (lnlen == 0) continue;
        int wsp = 1;
        for (int j = 0; j < lnlen; j++) if (ln[j] != ' ' && ln[j] != '\t') { wsp = 0; break; }
        if (wsp) continue;
        if (ln[0] == '#') continue;
        if (ln[0] == '@') {
            int eq = -1;
            for (int j = 1; j < lnlen; j++) if (ln[j] == '=') { eq = j; break; }
            if (eq < 0) continue;
            void *tmp = realloc(a->meta, (a->nmeta + 1) * sizeof(char *));
            if (!tmp) continue;
            a->meta = tmp;
            int klen = eq - 1, vlen = lnlen - eq - 1;
            char *kv = malloc(klen + vlen + 2);
            if (!kv) continue;
            memcpy(kv, ln + 1, klen);
            kv[klen] = '=';
            memcpy(kv + klen + 1, ln + eq + 1, vlen);
            kv[klen + vlen + 1] = 0;
            a->meta[a->nmeta++] = kv;
            continue;
        }

        if (chidx >= a->nchans) continue;
        int ntoks = 0;
        for (int j = 0; j < lnlen; ) {
            if (ln[j] == ' ' || ln[j] == '\t') { j++; continue; }
            ntoks++;
            while (j < lnlen && ln[j] != ' ' && ln[j] != '\t') j++;
        }

        if (ntoks == 0 || ntoks % 4 != 0) continue;
        int nsegs = ntoks / 4;
        amy_chan_t *ch = &a->chans[chidx];
        ch->segs = calloc(nsegs, sizeof(amy_seg_t));
        if (!ch->segs) continue;
        ch->nsegs = nsegs;
        int tok = 0;
        double running = 0;
        for (int j = 0; j < lnlen && tok < ntoks; ) {
            if (ln[j] == ' ' || ln[j] == '\t') { j++; continue; }
            int st = j;
            while (j < lnlen && ln[j] != ' ' && ln[j] != '\t') j++;
            int tlen = j - st;
            int mod = tok % 4;
            int sidx = tok / 4;
            if (mod == 0) {
                int wt = wave_type(ln + st, tlen);
                ch->segs[sidx].type = wt >= 0 ? (amy_wave_t)wt : AMY_SILENCE;
            } else if (mod == 1) {
                ch->segs[sidx].freq = atof(ln + st);
            } else if (mod == 2) {
                ch->segs[sidx].amp = atof(ln + st);
            } else {
                ch->segs[sidx].dur = atof(ln + st);
                ch->segs[sidx].start = running;
                running += ch->segs[sidx].dur;
            }
            tok++;
        }
        ch->dur = running;
        chidx++;
    }

    free(lines);
    free(lens);
    return 0;
}

void amy_free(amy_t *a)
{
    for (int i = 0; i < a->nchans; i++) free(a->chans[i].segs);
    free(a->chans);
    for (int i = 0; i < a->nmeta; i++) free(a->meta[i]);
    free(a->meta);
    memset(a, 0, sizeof(*a));
}

double amy_dur(const amy_t *a)
{
    double d = 0;
    for (int i = 0; i < a->nchans; i++) if (a->chans[i].dur > d) d = a->chans[i].dur;
    return d;
}

static double gen(amy_wave_t t, double ph)
{
    switch (t) {
    case AMY_SINE:     return sin(2.0 * M_PI * ph);
    case AMY_SQUARE:   return ph - floor(ph) < 0.5 ? 1.0 : -1.0;
    case AMY_TRIANGLE: return 2.0 * fabs(2.0 * (ph - floor(ph + 0.5))) - 1.0;
    case AMY_SAWTOOTH: return 2.0 * (ph - floor(ph + 0.5));
    case AMY_NOISE:    return 2.0 * ((double)rand() / RAND_MAX) - 1.0;
    default:           return 0.0;
    }
}

void amy_render(const amy_t *a, double t, float *buf, int nchans)
{
    for (int i = 0; i < nchans; i++) buf[i] = 0.0f;
    for (int ci = 0; ci < a->nchans; ci++) {
        amy_chan_t *ch = &a->chans[ci];
        double val = 0.0;
        for (int si = 0; si < ch->nsegs; si++) {
            amy_seg_t *seg = &ch->segs[si];
            if (t >= seg->start && t < seg->start + seg->dur) {
                val = seg->amp * gen(seg->type, seg->freq * (t - seg->start));
                break;
            }
        }
        for (int oc = 0; oc < nchans; oc++)
            buf[oc] += (float)val;
    }
    if (a->nchans > 0) {
        float s = 1.0f / a->nchans;
        for (int i = 0; i < nchans; i++) buf[i] *= s;
    }
}

int amy_wav(const amy_t *a, const char *path, int repeat)
{
    if (repeat < 1) repeat = 1;
    double dur = amy_dur(a);
    int sr = AMY_SR, nc = 2, bits = 16;
    int nf = (int)(dur * sr) + 1;
    int tf = nf * repeat;
    int ds = tf * nc * (bits / 8);
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    int cs = 36 + ds;
    short fmt = 1, chans = nc, ba = nc * (bits / 8), bps = bits;
    int br = sr * ba;
    fwrite("RIFF", 1, 4, f);
    fwrite(&cs, 4, 1, f);
    fwrite("WAVE", 1, 4, f);
    int fs = 16;
    fwrite("fmt ", 1, 4, f);
    fwrite(&fs, 4, 1, f);
    fwrite(&fmt, 2, 1, f);
    fwrite(&chans, 2, 1, f);
    fwrite(&sr, 4, 1, f);
    fwrite(&br, 4, 1, f);
    fwrite(&ba, 2, 1, f);
    fwrite(&bps, 2, 1, f);
    fwrite("data", 1, 4, f);
    fwrite(&ds, 4, 1, f);
    float *buf = malloc(nc * sizeof(float));
    if (!buf) { fclose(f); return -1; }
    for (int i = 0; i < tf; i++) {
        amy_render(a, fmod((double)i / sr, dur), buf, nc);
        for (int c = 0; c < nc; c++) {
            float v = buf[c] * 32767.0f;
            if (v > 32767.0f) v = 32767.0f;
            if (v < -32768.0f) v = -32768.0f;
            short s = (short)v;
            fwrite(&s, 2, 1, f);
        }
    }

    free(buf);
    fclose(f);
    return 0;
}

#ifdef AMY_HAVE_SDL
struct ctx { const amy_t *a; volatile int run; volatile double t; volatile int repeat; };
static void cb(void *ud, Uint8 *st, int len)
{
    struct ctx *c = ud;
    int nc = 2, ns = len / (nc * 2);
    short *out = (short *)st;
    for (int i = 0; i < ns; i++) {
        float b[2];
        amy_render(c->a, c->t, b, nc);
        for (int ch = 0; ch < nc; ch++) {
            float f = b[ch] * 32767.0f;
            if (f > 32767.0f) f = 32767.0f;
            if (f < -32768.0f) f = -32768.0f;
            out[i * nc + ch] = (short)f;
        }
        c->t += 1.0 / AMY_SR;
        if (c->t >= amy_dur(c->a)) {
            if (c->repeat == 0) {
                c->t = 0;
            } else {
                c->repeat--;
                if (c->repeat <= 0) {
                    c->run = 0;
                    int rem = (ns - i - 1) * nc;
                    memset(out + (i + 1) * nc, 0, rem * 2);
                    return;
                }
                c->t = 0;
            }
        }
    }
}

int amy_play(const amy_t *a, int repeat)
{
    if (SDL_Init(SDL_INIT_AUDIO) < 0) return -1;
    struct ctx c = { a, 1, 0.0, repeat };
    SDL_AudioSpec w;
    w.freq = AMY_SR;
    w.format = AUDIO_S16SYS;
    w.channels = 2;
    w.samples = 1024;
    w.callback = cb;
    w.userdata = &c;
    SDL_AudioDeviceID d = SDL_OpenAudioDevice(NULL, 0, &w, NULL, 0);
    if (!d) { SDL_Quit(); return -1; }
    SDL_PauseAudioDevice(d, 0);
    while (c.run) SDL_Delay(10);
    SDL_CloseAudioDevice(d);
    SDL_Quit();
    return 0;
}
#endif
