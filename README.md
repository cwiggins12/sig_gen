# sig_gen
I built this to test my current project fully and figured it might help out anyone looking for a good, cheap, high-quality and minimalist way to test any audio analysis tools or audio device output.

sig_gen is an audio signal generator with a minimal graphical interface, built with C and SDL2. 


![sig_gen screenshot placeholder](sig_gen_screenshot.png)

## Features
- **7 waveform types:** Sine, Square, Saw, Triangle, White Noise, Pink Noise, and silence
- **Real-time parameter control** via keyboard or by clicking and typing values directly
- **Smooth parameter transitions** — all parameter changes are interpolated to avoid clicks and pops
- **Embedded font** — no external font files needed at runtime

## Technical Implementation

While the interface is minimal, the underlying audio engine is designed with real-time safety and signal integrity in mind:

- **Per-sample parameter smoothing**  
  Frequency changes use a multiplicative ramp over a fixed time constant, which produces perceptually even glides across the frequency range since pitch is logarithmic. Amplitude changes use a linear ramp over the same time constant. Both approaches skip the smoothing math entirely when no change is in progress, avoiding unnecessary per-sample computation.

- **Artifact-free waveform transitions**  
  When switching waveforms, amplitude is smoothly ramped to zero before the waveform change occurs and then ramped back up, avoiding transient artifacts.
  
- **Anti-aliasing via oversampling and FIR decimation**  
  Signal generation runs internally at 4× the device native sample rate. Waveform transitions are pre-conditioned using PolyBLEP and PolyBLAMP, and a 256-tap windowed FIR filter is applied before decimation. Together these produce a clean spectrum comparable to a hardware reference generator.
  
- **Device-native audio format**  
  The audio device is opened at the system's native sample rate and channel count using `SDL_GetAudioDeviceSpec`, avoiding unnecessary resampling and ensuring phase-accurate output at all frequencies.

- **Gaussian white noise generation**  
  White noise is generated using a Box–Muller transform driven by a xorshift32 pseudo-random number generator.

- **Pink noise synthesis**  
  Pink noise is produced using a multi-stage filtered noise model to approximate a 1/f spectral distribution. Output is clamped to [-1, 1] to prevent clipping from filter state accumulation.

- **Embedded font system**  
  The UI font is compiled directly into the binary using `xxd -i`, removing runtime font dependencies and keeping the executable self-contained.

You can find more implementation for the application here: [codywigginsdev.neocities.org/sig_gen](https://codywigginsdev.neocities.org/sig_gen)

## Controls
| Input | Action |
|---|---|
| `↑` / `↓` | Increase / decrease frequency by 10 Hz |
| `←` / `→` | Decrease / increase amplitude by 0.05 |
| `0` | Silence |
| `1` | Sine wave |
| `2` | Square wave |
| `3` | Sawtooth wave |
| `4` | Triangle wave |
| `5` | White noise |
| `6` | Pink noise |
| `ESC` | Quit |

You can also **click** on the Frequency or Amplitude fields, **type** a value, and press **Enter** to set it directly. 

Valid ranges are 20–20000 Hz for frequency and 0.0–1.0 for amplitude.

## Download
Pre-built binaries for Linux and Windows are available on the [Releases](../../releases) page.

- **Windows:** Fully statically linked — no dependencies or installation required, just download and run. Tested on Windows 11 x64.
- **Linux:** Built on Ubuntu 22.04. Fully statically linked. Most modern distros (Ubuntu 22.04+, Debian 12+, Fedora 36+) will work out of the box.

## Building
Both builds are statically linked, producing a single self-contained executable with no runtime dependencies.

### Dependencies
| Platform | Requirements |
|---|---|
| **Linux** | `cmake`, a C compiler, `libsdl2-dev`, `libsdl2-ttf-dev` |
| **Windows** | MinGW-w64, `cmake`, [vcpkg](https://github.com/microsoft/vcpkg) with `sdl2` and `sdl2-ttf` installed for the `x64-mingw-static` triplet |

### Linux
```sh
cmake --preset linux
cmake --build build
./build/sig_gen
```

### Windows
Set the `VCPKG_ROOT` environment variable to your vcpkg installation path, then:
```powershell
cmake --preset windows
cmake --build build
.\build\sig_gen.exe
```

## Switching Fonts(Linux)
The font is compiled directly into the binary. To use a different `.ttf` font:
```sh
xxd -i yourfont.ttf > src/font_embedded.c
```
Then update the variable names in `font_embedded.c` to match the `extern` declarations at the top of `renderer.c`, and make sure both arrays are declared `const`.

## License
MIT

> **Note:** The embedded font, Digital-7 by Sizenko Alexander ([Style-7](http://www.styleseven.com)), is freeware for personal and educational use. It is **not** covered by the MIT license. For commercial use, a separate license must be purchased from the author or you must use the steps above to switch to a different font that allows for commercial use.

## Authors
**Cody Wiggins**
- Website: [codywigginsdev.neocities.org](https://codywigginsdev.neocities.org)
- Email: [codywigginsdev@gmail.com](mailto:codywigginsdev@gmail.com)

**Digital-7 Font** — Sizenko Alexander / Style-7
- [http://www.styleseven.com](http://www.styleseven.com)
