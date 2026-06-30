# AMY (Audio Media Yielding)

A tiny, human-readable audio format and player. Write music as plain text. Definitely not named after my girlfriend.

## Format

```
AMY <num_channels>
@key=value
# comments start with #
<chan0> <wave> <freq> <amp> <dur>  [<wave> <freq> <amp> <dur> ...]
<chan1> <wave> <freq> <amp> <dur>  ...
...
```

Each line after the header is a channel. Segments are space-separated 4-tuples: waveform, frequency (Hz), amplitude (0–1), duration (seconds).

| Waveform    | Description |
|-------------|-------------|
| `sine`      | Sine wave |
| `square`    | Square wave |
| `triangle`  | Triangle wave |
| `sawtooth`  | Sawtooth wave |
| `noise`     | White noise |
| `silence`   | Silence |

## Build

```sh
make
```

Requires `gcc` and `libm`. Real-time playback needs SDL2; without it, use `-w` to export WAV.

## Usage

```sh
amy <file.amy>          # play via SDL2
amy -w <file.amy>       # write to <file>.wav
amy -w <file.amy> out.wav  # write to out.wav
```

## Example

A C Major chord with bass and arpeggio:

```
AMY 3
@title=Chord C Major
@author=neo
sine 261.63 0.3 3.0 sine 329.63 0.3 3.0 sine 392.00 0.3 3.0
# bass channel
sawtooth 130.81 0.4 3.0
square 261.63 0.2 0.5 square 293.66 0.2 0.5 square 329.63 0.2 0.5 square 349.23 0.2 0.5 square 392.00 0.2 0.5 square 440.00 0.2 0.5
```

## API

```c
int    amy_parse(amy_t *a, const char *s, size_t n);
void   amy_free(amy_t *a);
double amy_dur(const amy_t *a);
void   amy_render(const amy_t *a, double t, float *buf, int nchans);
int    amy_wav(const amy_t *a, const char *path);
int    amy_play(const amy_t *a);  // requires SDL2
```
