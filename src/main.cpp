#include <fmt/core.h>
#include <stdlib.h>
#include <cstdint>

#include "MiniFB.h"

#define WIDTH 64
#define HEIGHT 32
#define PIXEL_SIZE 16
#define WINDOW_WIDTH (WIDTH * PIXEL_SIZE)
#define WINDOW_HEIGHT (HEIGHT * PIXEL_SIZE)

int main() {
  fmt::print("Hello, vcpkg + CMakea!\n");

  struct mfb_window *window = mfb_open("chip8", WINDOW_WIDTH, WINDOW_HEIGHT);
  if (window == NULL)
  {
    return 0;
  }
  uint32_t *buffer = (uint32_t*)malloc(WINDOW_WIDTH * WINDOW_HEIGHT * 4);

  // test
  for (int y = 0; y < WINDOW_HEIGHT; ++y)
  {
    for (int x = 0; x < WINDOW_WIDTH; ++x)
    {
      auto pixelX = x / PIXEL_SIZE;
      auto pixelY = y / PIXEL_SIZE;
      auto pink = pixelY % 4 ==  0 || pixelX % 4 == 0;
      buffer[y * WINDOW_WIDTH + x] = pink ? 0xF0A0A0 : 0xFFFFFF;
    }
  }

  mfb_update_state state;
  do {
    // TODO: add some fancy rendering to the buffer of size 800 * 600

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
