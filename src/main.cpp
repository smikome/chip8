#include <fmt/core.h>
#include <stdlib.h>
#include <cstdint>

#include "MiniFB.h"
#include "chip8/chip8.h"

static constexpr uint32_t WIDTH = 64;
static constexpr uint32_t HEIGHT = 32;
static constexpr uint32_t PIXEL_SIZE = 16;
static constexpr uint32_t WINDOW_WIDTH = (WIDTH * PIXEL_SIZE);
static constexpr uint32_t WINDOW_HEIGHT = (HEIGHT * PIXEL_SIZE);

int main() {
  fmt::print("Hello, vcpkg + CMakea!\n");

  struct mfb_window *window = mfb_open("chip8", WINDOW_WIDTH, WINDOW_HEIGHT);
  if (window == NULL)
  {
    return 0;
  }
  // 60Hz
  mfb_set_target_fps(60);
  uint32_t *buffer = (uint32_t*)malloc(WINDOW_WIDTH * WINDOW_HEIGHT * 4);

  auto chip8 = Chip8();
  chip8.Load("roms/PONG");

  std::array<uint8_t, 2048> drawFlag;
  mfb_update_state state;
  do {
    for (int i = 0; i < 50; ++i)
    {
      chip8.EmulateCycle();
    }
    chip8.UpdateTimer();
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

  free(buffer);
  buffer = NULL;
  window = NULL;


  return 0;
}
