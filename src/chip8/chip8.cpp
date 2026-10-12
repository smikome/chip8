#include "chip8/chip8.h"

#include <cassert>
#include <vector>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <fmt/core.h>

namespace
{
  constexpr std::chrono::microseconds TIMER_UPDATE_SPAN{1'000'000 / 60}; // 60Hz
  constexpr std::array<uint8_t, FONT_PRESET_MEMORY_NUM> font_preset =
  {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
  };

  std::vector<uint8_t> LoadFile(const std::filesystem::path& path) {
    // ファイルサイズを取得（失敗したら空を返す）
    std::error_code ec;
    std::size_t size = std::filesystem::file_size(path, ec);
    if (ec || size == 0) {
        return {};  // 空の vector を返す
    }

    // ファイルを開く（失敗したら空を返す）
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return {};
    }

    // 読み込み用バッファを確保
    std::vector<uint8_t> buffer(size);

    // 読み込み（失敗したら空を返す）
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    if (!file) {
        return {};
    }

    return buffer;
  }
}

Chip8State::Chip8State()
{
  // 各種変数の初期化
  for (auto i = 0; i < MEMORY_NUM; ++i)
  {
    memory[i] = 0;
  }
  // フォントプリセット
  for (auto i = 0; i < FONT_PRESET_MEMORY_NUM; ++i)
  {
    memory[i] = font_preset[i];
  }
  for (auto i = 0; i < REG_V_NUM; ++i)
  {
    reg_v[i] = 0;
  }
  reg_i = 0;
  delay_timer = 0;
  sound_timer = 0;
  reg_pc = PC_INITIAL_VALUE;
  reg_sp = 0;
  for (int i = 0; i < STACK_NUM; ++i)
  {
    stack[i] = 0;
  }
}

Chip8State::~Chip8State()
{
}

void Chip8State::FetchCode(uint16_t& opcode)
{
  opcode = (memory[reg_pc] << 8) | (memory[reg_pc + 1]);
}

void Chip8State::IncrementPC()
{
  reg_pc += 2;
}

void Chip8State::UpdateTimer()
{
  auto now = std::chrono::steady_clock::now();
  auto deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(now - prevTime);
  prevTime = now;
  elapsedTime += deltaTime;

  while (elapsedTime > TIMER_UPDATE_SPAN)
  {
    elapsedTime -= TIMER_UPDATE_SPAN;
    if (delay_timer > 0)
    {
      --delay_timer;
    }
    if (sound_timer > 0)
    {
      --sound_timer;
    }
  }
}

Chip8::Chip8()
{
  opFuncTable[0x0] = &Chip8::ExecuteOp0;
  opFuncTable[0x1] = &Chip8::ExecuteOp1;
  opFuncTable[0x2] = &Chip8::ExecuteOp2;
  opFuncTable[0x3] = &Chip8::ExecuteOp3;
  opFuncTable[0x4] = &Chip8::ExecuteOp4;
  opFuncTable[0x5] = &Chip8::ExecuteOp5;
  opFuncTable[0x6] = &Chip8::ExecuteOp6;
  opFuncTable[0x7] = &Chip8::ExecuteOp7;
  opFuncTable[0x8] = &Chip8::ExecuteOp8;
  opFuncTable[0x9] = &Chip8::ExecuteOp9;
  opFuncTable[0xA] = &Chip8::ExecuteOpA;
  opFuncTable[0xB] = &Chip8::ExecuteOpB;
  opFuncTable[0xC] = &Chip8::ExecuteOpC;
  opFuncTable[0xD] = &Chip8::ExecuteOpD;
  opFuncTable[0xE] = &Chip8::ExecuteOpE;
  opFuncTable[0xF] = &Chip8::ExecuteOpF;
}

Chip8::~Chip8()
{
}

bool Chip8::Load(const char* filePath)
{
  state = Chip8State();

  // ROMロード処理
  auto tempBuffer = LoadFile(filePath);
  if (tempBuffer.empty())
  {
    fmt::println("Failed to load ROM.");
    return false;
  }
  if (MEMORY_NUM - PC_INITIAL_VALUE < tempBuffer.size())
  {
    fmt::println("ROM size is invalid.");
    return false;
  }

  std::copy(tempBuffer.begin(), tempBuffer.end(), state.memory.begin() + PC_INITIAL_VALUE);
  fmt::println("Chip8 ROM load success. ROM size: {}", tempBuffer.size());
  return true;
}

void Chip8::EmulateCycle()
{
  uint16_t opcode = 0;
  state.FetchCode(opcode);
  state.IncrementPC();
  ExecuteOpCode(opcode);
}

void Chip8::UpdateTimer()
{
  state.UpdateTimer();
}

void Chip8::ExecuteOpCode(const uint16_t& opcode)
{
  auto nibble = (opcode & 0xF000) >> 12;
  assert(nibble < opFuncTable.size() && "nibble out of range");
  (this->*opFuncTable[nibble])(opcode);
}

void Chip8::ExecuteOp0(const uint16_t& opcode)
{
}

void Chip8::ExecuteOp1(const uint16_t& opcode)
{
}

void Chip8::ExecuteOp2(const uint16_t& opcode)
{
}

void Chip8::ExecuteOp3(const uint16_t& opcode)
{
}

void Chip8::ExecuteOp4(const uint16_t& opcode)
{
}

void Chip8::ExecuteOp5(const uint16_t& opcode)
{
}

void Chip8::ExecuteOp6(const uint16_t& opcode)
{
}

void Chip8::ExecuteOp7(const uint16_t& opcode)
{
}

void Chip8::ExecuteOp8(const uint16_t& opcode)
{
}

void Chip8::ExecuteOp9(const uint16_t& opcode)
{
}

void Chip8::ExecuteOpA(const uint16_t& opcode)
{
}

void Chip8::ExecuteOpB(const uint16_t& opcode)
{
}

void Chip8::ExecuteOpC(const uint16_t& opcode)
{
}

void Chip8::ExecuteOpD(const uint16_t& opcode)
{
}

void Chip8::ExecuteOpE(const uint16_t& opcode)
{
}

void Chip8::ExecuteOpF(const uint16_t& opcode)
{
}
