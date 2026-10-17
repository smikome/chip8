#pragma once

#include <array>
#include <cstdint>
#include <chrono>
#include <random>

static constexpr std::size_t MEMORY_NUM = 4096;
static constexpr std::size_t PC_INITIAL_VALUE = 0x0200; // PCの初期値
static constexpr std::size_t FONT_PRESET_MEMORY_NUM = 80; // 5バイト * 16文字（0~9, A~F）
static constexpr std::size_t REG_V_NUM = 16;
static constexpr std::size_t STACK_NUM = 16;

struct Chip8State
{
public:
  Chip8State();
  ~Chip8State();

  void FetchCode(uint16_t& opcode);
  void IncrementPC();
  void DecrementPC();
  void UpdateTimer();

  std::array<uint8_t, MEMORY_NUM> memory;
  std::array<uint8_t, REG_V_NUM> reg_v;
  uint16_t reg_i;
  uint8_t delay_timer;
  uint8_t sound_timer;
  uint16_t reg_pc;
  uint8_t reg_sp;
  std::array<uint16_t, STACK_NUM> stack;
  std::chrono::steady_clock::time_point prevTime;
  std::chrono::microseconds elapsedTime;
};

struct Chip8
{
public:
  Chip8();
  ~Chip8();

  bool Load(const char* filePath);
  void EmulateCycle();
  void UpdateTimer();

private:
  void ExecuteOpCode(const uint16_t& opcode);

  void ExecuteOp0(const uint16_t& opcode);
  void ExecuteOp1(const uint16_t& opcode);
  void ExecuteOp2(const uint16_t& opcode);
  void ExecuteOp3(const uint16_t& opcode);
  void ExecuteOp4(const uint16_t& opcode);
  void ExecuteOp5(const uint16_t& opcode);
  void ExecuteOp6(const uint16_t& opcode);
  void ExecuteOp7(const uint16_t& opcode);
  void ExecuteOp8(const uint16_t& opcode);
  void ExecuteOp9(const uint16_t& opcode);
  void ExecuteOpA(const uint16_t& opcode);
  void ExecuteOpB(const uint16_t& opcode);
  void ExecuteOpC(const uint16_t& opcode);
  void ExecuteOpD(const uint16_t& opcode);
  void ExecuteOpE(const uint16_t& opcode);
  void ExecuteOpF(const uint16_t& opcode);

  void HandleUnknownOpcode(const uint16_t& opcode);

private:
  Chip8State state;

  using OpFunc = void (Chip8::*)(const uint16_t&);
  std::array<OpFunc, 0xF> opFuncTable;

  bool updateDraw = false;
  // 64 * 32のディスプレイ描画フラグ, drawFlag[y + x * 64]で取得
  std::array<uint8_t, 2048> drawFlag;
  std::array<uint8_t, 16> key;

  std::mt19937 randEngine;
};
