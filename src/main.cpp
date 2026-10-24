#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <fmt/core.h>
#include <cstring>

#include "MiniFB.h"
#include "chip8/chip8.h"

static constexpr uint32_t WIDTH = 64;
static constexpr uint32_t HEIGHT = 32;
static constexpr uint32_t PIXEL_SIZE = 16;
static constexpr uint32_t WINDOW_WIDTH = (WIDTH * PIXEL_SIZE);
static constexpr uint32_t WINDOW_HEIGHT = (HEIGHT * PIXEL_SIZE);

struct AudioContext {
    bool buzzerOn = false;
    float phase = 0.0f;
};

static void audio_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    (void)pInput;
    AudioContext* pCtx = static_cast<AudioContext*>(pDevice->pUserData);
    float* pOutputF32 = static_cast<float*>(pOutput);

    if (!pCtx->buzzerOn) {
        std::memset(pOutput, 0, frameCount * sizeof(float) * 2);
        return;
    }

    const float frequency = 440.0f;
    const float sampleRate = static_cast<float>(pDevice->sampleRate);
    const float amplitude = 0.2f;

    for (ma_uint32 i = 0; i < frameCount; ++i)
    {
        // 矩形波
        float sample = (pCtx->phase < 0.5f) ? amplitude : -amplitude;
        pOutputF32[i * 2 + 0] = sample; // L
        pOutputF32[i * 2 + 1] = sample; // R

        pCtx->phase += frequency / sampleRate;
        if (pCtx->phase >= 1.0f) pCtx->phase -= 1.0f;
    }
}

class Chip8Input {
public:
  Chip8* chip8 = nullptr;

  void keyboard(struct mfb_window *window, mfb_key key, mfb_key_mod mod, bool isPressed)
  {
    int chip8Key = -1;

    switch (key)
    {
        case MFB_KB_KEY_1: chip8Key = 0x1; break;
        case MFB_KB_KEY_2: chip8Key = 0x2; break;
        case MFB_KB_KEY_3: chip8Key = 0x3; break;
        case MFB_KB_KEY_4: chip8Key = 0xC; break;
        case MFB_KB_KEY_Q: chip8Key = 0x4; break;
        case MFB_KB_KEY_W: chip8Key = 0x5; break;
        case MFB_KB_KEY_E: chip8Key = 0x6; break;
        case MFB_KB_KEY_R: chip8Key = 0xD; break;
        case MFB_KB_KEY_A: chip8Key = 0x7; break;
        case MFB_KB_KEY_S: chip8Key = 0x8; break;
        case MFB_KB_KEY_D: chip8Key = 0x9; break;
        case MFB_KB_KEY_F: chip8Key = 0xE; break;
        case MFB_KB_KEY_Z: chip8Key = 0xA; break;
        case MFB_KB_KEY_X: chip8Key = 0x0; break;
        case MFB_KB_KEY_C: chip8Key = 0xB; break;
        case MFB_KB_KEY_V: chip8Key = 0xF; break;
        case MFB_KB_KEY_ESCAPE:
            if (isPressed) mfb_close(window);
            return;
        default:
            break;
    }

    if (chip8 == nullptr || chip8Key < 0)
    {
        return;
    }

    auto& refKey = chip8->GetKey();
    refKey[chip8Key] = isPressed;
  }
};

int main(int argc, char* argv[]) {
  if (argc < 2)
  {
    fmt::println("Usage: {} <rom_file>", argv[0]);
    return -1;
  }

  struct mfb_window *window = mfb_open("chip8", WINDOW_WIDTH, WINDOW_HEIGHT);
  if (window == NULL)
  {
    return 0;
  }

  mfb_set_target_fps(60);

  uint32_t *buffer = static_cast<uint32_t*>(calloc(WINDOW_WIDTH * WINDOW_HEIGHT, sizeof(uint32_t)));

  auto chip8 = Chip8();
  if (!chip8.Load(argv[1])) {
    fmt::println("Failed to load ROM: {}", argv[1]);
    free(buffer);
    return -1;
  }

  Chip8Input input;
  input.chip8 = &chip8;
  mfb_set_keyboard_callback(window, &input, &Chip8Input::keyboard);

  // オーディオ初期化
  AudioContext audioCtx;
  ma_device_config config = ma_device_config_init(ma_device_type_playback);
  config.playback.format = ma_format_f32;
  config.playback.channels = 2;
  config.sampleRate = 0; // デバイスのネイティブレートを使用
  config.dataCallback = audio_callback;
  config.pUserData = &audioCtx;

  ma_device audioDevice;
  ma_result result = ma_device_init(NULL, &config, &audioDevice);
  if (result == MA_SUCCESS)
  {
    ma_device_start(&audioDevice);
  }
  else
  {
    fmt::println("Warning: Failed to initialize audio device.");
  }

  std::array<uint8_t, 2048> drawFlag;
  mfb_update_state state;
  do {
    for (int i = 0; i < 10; ++i)
    {
      chip8.EmulateCycle();
    }
    chip8.UpdateTimer();

    // ブザー状態を反映
    audioCtx.buzzerOn = chip8.ShouldSoundBuzzer();

    if (chip8.ShouldUpdateDraw(drawFlag))
    {
      for (int y = 0; y < WINDOW_HEIGHT; ++y)
      {
        for (int x = 0; x < WINDOW_WIDTH; ++x)
        {
          auto pixelX = x / PIXEL_SIZE;
          auto pixelY = y / PIXEL_SIZE;
          buffer[y * WINDOW_WIDTH + x] = drawFlag[pixelY * WIDTH + pixelX] ? 0xFF00 : 0x0;
        }
      }
    }

    state = mfb_update(window, buffer);

    if (state != MFB_STATE_OK)
    {
      break;
    }
  } while(mfb_wait_sync(window));

  if (result == MA_SUCCESS)
  {
    ma_device_stop(&audioDevice);
    ma_device_uninit(&audioDevice);
  }

  free(buffer);
  buffer = NULL;
  window = NULL;

  return 0;
}
