#include "chip8/chip8.h"

#include <cassert>
#include <vector>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <fmt/core.h>
#include <random>

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

void Chip8State::DecrementPC()
{
  reg_pc -= 2;
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

  std::random_device seed_gen;
  randEngine = std::mt19937(seed_gen());
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
  fmt::println("pc: {:x}, opcode: {:x}, delay_timer: {}, sound_timer: {}",
    state.reg_pc,
    opcode,
    state.delay_timer,
    state.sound_timer);
  state.IncrementPC();
  ExecuteOpCode(opcode);
}

void Chip8::UpdateTimer()
{
  state.UpdateTimer();
}

bool Chip8::ShouldUpdateDraw(std::array<uint8_t, 2048>& outDrawFlag)
{
  if (updateDraw)
  {
    outDrawFlag = drawFlag;
    return true;
  }
  return false;
}

void Chip8::ExecuteOpCode(const uint16_t& opcode)
{
  auto nibble = (opcode & 0xF000) >> 12;
  assert(nibble < opFuncTable.size() && "nibble out of range");
  (this->*opFuncTable[nibble])(opcode);
}

void Chip8::ExecuteOp0(const uint16_t& opcode)
{
  switch (opcode)
  {
    case 0x00E0:
    {
      // 00E0 - CLS
      fmt::println("clear display");
      break;
    }
    case 0x00EE:
    {
      // 00EE - RET
      // サブルーチンから戻る。
      // プログラムカウンタにスタックの一番上のアドレスをセットし、スタックポインタから1を引く。
      state.reg_pc = state.stack[state.reg_sp];
      state.reg_sp--;
      break;
    }
    default:
    {
      HandleUnknownOpcode(opcode);
      break;
    }
  }
}

void Chip8::ExecuteOp1(const uint16_t& opcode)
{
  // 1nnn - JP addr
  // アドレスnnnにジャンプする。インタプリタはプログラムカウンタをnnnにする。
  auto nnn = opcode & 0x0FFF;
  state.reg_pc = nnn;
}

void Chip8::ExecuteOp2(const uint16_t& opcode)
{
  // 2nnn - CALL addr
  // アドレスnnnのサブルーチンをCallする。
  // インタプリタはスタックポインタをインクリメントし、それから現在のプログラムカウンタをスタックの一番上に置く。
  // さらにプログラムカウンタにnnnをセットする。
  auto nnn = opcode & 0x0FFF;
  state.reg_sp++;
  state.stack[state.reg_sp] = state.reg_pc;
  state.reg_pc = nnn;
}

void Chip8::ExecuteOp3(const uint16_t& opcode)
{
  // 3xkk - SE Vx, byte
  // Vx = kkの場合、次の命令をスキップする。
  // インタプリタはレジスタVxとkkを比較し、二つが等しいならプログラムカウンタを2進める。
  auto x = opcode & 0x0F00 >> 8;
  auto kk = opcode & 0x00FF;
  if (state.reg_v[x] == kk)
  {
    state.IncrementPC();
  }
}

void Chip8::ExecuteOp4(const uint16_t& opcode)
{
  // 4xkk - SNE Vx, byte
  // Vx != kkの場合、次の命令をスキップする。
  // インタプリタはレジスタVxとkkを比較し、二つが異なるならプログラムカウンタを2進める。
  auto x = opcode & 0x0F00 >> 8;
  auto kk = opcode & 0x00FF;
  if (state.reg_v[x] != kk)
  {
    state.IncrementPC();
  }
}

void Chip8::ExecuteOp5(const uint16_t& opcode)
{
  // 5xy0 - SE Vx, Vy
  // Vx = Vyの場合、次の命令をスキップする。
  // インタプリタはレジスタVxとVyを比較し、二つが等しいならプログラムカウンタを2進める。
  auto x = opcode & 0x0F00 >> 8;
  auto y = opcode & 0x00F0 >> 4;
  if (state.reg_v[x] == state.reg_v[y])
  {
    state.IncrementPC();
  }
}

void Chip8::ExecuteOp6(const uint16_t& opcode)
{
  // 6xkk - LD Vx, byte
  // Vxにkkをセットする。
  // インタプリタはレジスタVxにkkの値をセットする。
  auto x = opcode & 0x0F00 >> 8;
  auto kk = opcode & 0x00FF;
  state.reg_v[x] = kk;
}

void Chip8::ExecuteOp7(const uint16_t& opcode)
{
  // 7xkk - ADD Vx, byte
  // VxにVx + kkをセットする。
  // インタプリタはレジスタVxにkkの値を加算する。
  auto x = opcode & 0x0F00 >> 8;
  auto kk = opcode & 0x00FF;
  state.reg_v[x] += kk;
}

void Chip8::ExecuteOp8(const uint16_t& opcode)
{
  auto nibble = opcode & 0x000F;
  auto x = opcode & 0x0F00 >> 8;
  auto y = opcode & 0x00F0 >> 4;
  switch (nibble)
  {
    case 0x0000:
    {
      // 8xy0 - LD Vx, Vy
      // VxにVyをセットする。
      // インタプリタはレジスタVxにVyの値をセットする。
      state.reg_v[x] = state.reg_v[y];
      break;
    }
    case 0x0001:
    {
      // 8xy1 - OR Vx, Vy
      // VxにVx OR Vyをセットする。
      // VxとVyのビット和をとり、Vxにセットする。
      // ビット和は二つの値のビットを比較し、どちらかひとつでも1の場合は1、両方とも0の場合は0とする。
      state.reg_v[x] = state.reg_v[x] | state.reg_v[y];
      break;
    }
    case 0x0002:
    {
      // 8xy2 - AND Vx, Vy
      // VxにVx AND Vyをセットする。
      // VxとVyのビット積をとり、Vxにセットする。
      // ビット積は二つの値のビットを比較し、両方とも1の場合は1、それ以外の場合は0とする。
      state.reg_v[x] = state.reg_v[x] & state.reg_v[y];
      break;
    }
    case 0x0003:
    {
      // 8xy3 - XOR Vx, Vy
      // VxにVx XOR Vyをセットする。
      // VxとVyの排他的ビット和をとり、Vxにセットする。
      // 排他的ビット和は二つの値のビットを比較し、等しくない場合は1、それ以外の場合は0とする。
      state.reg_v[x] = state.reg_v[x] ^ state.reg_v[y];
      break;
    }
    case 0x0004:
    {
      // 8xy4 - ADD Vx, Vy
      // VxにVx + Vyをセットする。
      // 結果が8bit(255)より大きい場合はVfに1、それ以外の場合は0をセットする。
      // Vxには下位8bitのみ保持する。
      uint16_t tempAddResult = state.reg_v[x] + state.reg_v[y];
      // NOTE: x == fの場合はフラグでなく結果の値を優先している（どっちを優先すべきなのかわからん）
      state.reg_v[0xF] = tempAddResult > 0xFF;
      state.reg_v[x] = static_cast<uint8_t>(tempAddResult);
      break;
    }
    case 0x0005:
    {
      // 8xy5 - SUB Vx, Vy
      // Vx > Vyの場合はVfに1、それ以外の場合は0をセットする。
      // VxにVx - Vyをセットする。
      // NOTE: x == fの場合はフラグでなく結果の値を優先している（どっちを優先すべきなのかわからん）
      state.reg_v[0xF] = state.reg_v[x] > state.reg_v[y];
      state.reg_v[x] = state.reg_v[x] - state.reg_v[y];
      break;
    }
    case 0x0006:
    {
      // 8xy6 - SHR Vx {, Vy}
      // Vxの最下位ビットが1だった場合はVfに1、それ以外の場合は0をセットする。
      // VxにVxを2で割った(または右ビットシフト)値をセットする。
      // NOTE: x == fの場合はフラグでなく結果の値を優先している（どっちを優先すべきなのかわからん）
      state.reg_v[0xF] = state.reg_v[x] & 0x1;
      state.reg_v[x] >>= 1;
      break;
    }
    case 0x0007:
    {
      // 8xy7 - SUBN Vx, Vy
      // Vy > Vxの場合Vfに1、それ以外の場合は0をセットする。
      // VxにVy - Vxをセットする。
      // NOTE: x == fの場合はフラグでなく結果の値を優先している（どっちを優先すべきなのかわからん）
      state.reg_v[0xF] = state.reg_v[y] > state.reg_v[x];
      state.reg_v[x] = state.reg_v[y] - state.reg_v[x];
      break;
    }
    case 0x000E:
    {
      // 8xyE - SHL Vx {, Vy}
      // Vxの最上位ビットが1の場合Vfに1、それ以外の場合は0をセットする。
      // Vxに2をかけた(または左ビットシフト)値をセットする。
      // NOTE: x == fの場合はフラグでなく結果の値を優先している（どっちを優先すべきなのかわからん）
      state.reg_v[0xF] = state.reg_v[x] & 0x80;
      state.reg_v[x] <<= 1;
      break;
    }
    default:
    {
      HandleUnknownOpcode(opcode);
      break;
    }
  }
}

void Chip8::ExecuteOp9(const uint16_t& opcode)
{
  // 9xy0 - SNE Vx, Vy
  // Vx != Vyの場合、次の命令をスキップする。
  // つまり、プログラムカウンタを2インクリメントする。
  auto x = opcode & 0x0F00 >> 8;
  auto y = opcode & 0x00F0 >> 4;
  if (state.reg_v[x] != state.reg_v[y])
  {
    state.IncrementPC();
  }
}

void Chip8::ExecuteOpA(const uint16_t& opcode)
{
  // Annn - LD I, addr
  // Iにnnnをセットする。
  auto nnn = opcode & 0x0FFF;
  state.reg_i = nnn;
}

void Chip8::ExecuteOpB(const uint16_t& opcode)
{
  // Bnnn - JP V0, addr
  // nnn + V0のアドレスにジャンプする。つまり、プログラムカウンタにnnn + V0をセットする。
  auto nnn = opcode & 0x0FFF;
  state.reg_pc = nnn + state.reg_v[0x0];
}

void Chip8::ExecuteOpC(const uint16_t& opcode)
{
  // Cxkk - RND Vx, byte
  // Vxに0~255の乱数 AND kkをセットする。AND演算については8xy2を参照。
  auto x = opcode & 0x0F00 >> 8;
  auto kk = opcode & 0x00FF;
  std::uniform_int_distribution<int> distribution(0x0, 0xFF);
  auto rand = distribution(randEngine);
  state.reg_v[x] = rand & kk;
}

void Chip8::ExecuteOpD(const uint16_t& opcode)
{
  // Dxyn - DRW Vx, Vy, nibble
  // アドレスIのnバイトのスプライトを(Vx, Vy)に描画する。
  // Vfにはcollision(後述)をセットする。
  // アドレスIのnバイトのスプライトを読み出し、スプライトとして(Vx, Vy)に描画する。
  // スプライトは画面にXORする。
  // このとき、消されたピクセルが一つでもある場合はVfに1、それ以外の場合は0をセットする。
  // スプライトの一部が画面からはみ出る場合は、逆方向に折り返す。
  // XORについては8xy3に詳しい。
  // また、ディプレイの仕様に関しては2.4 Displayを参照のこと。
  auto x = opcode & 0x0F00 >> 8;
  auto y = opcode & 0x00F0 >> 4;
  auto n = opcode & 0x000F;
  bool collision = false;
  for (int offsetY = 0; offsetY < n; ++offsetY)
  {
    auto spriteByte = state.memory[state.reg_i + offsetY];
    for (int offsetX = 0; offsetX < 8; ++offsetX)
    {
      bool shouldDraw = (spriteByte << offsetX) & 0x80;
      if (shouldDraw)
      {
        // TODO: マジックナンバーを定数に
        auto drawX = (x + offsetX) % 64;
        auto drawY = (y + offsetY) % 32;
        auto drawIndex = drawY + drawX * 64;
        collision = collision || drawFlag[drawIndex]; // すでに描画されている
        drawFlag[drawY + drawX * 64] ^= 1; // XOR
        updateDraw = true;
      }
    }
  }
  state.reg_v[0xF] = collision ? 1 : 0;
}

void Chip8::ExecuteOpE(const uint16_t& opcode)
{
  auto lastByte = opcode & 0x00FF;
  auto x = (opcode & 0x0F00) >> 8;
  switch (lastByte)
  {
    case 0x009E:
    {
      // Ex9E - SKP Vx
      // Vxが押された場合、次の命令をスキップする。
      // キーボードををチェックし、Vxの値のキーが押されていればプログラムカウンタを2インクリメントする。
      if (key[state.reg_v[x]])
      {
        state.IncrementPC();
      }
      break;
    }
    case 0x00A1:
    {
      // ExA1 - SKNP Vx
      // Vxが押されてない場合、次の命令をスキップする。
      // キーボードををチェックし、Vxの値のキーが押されていなければプログラムカウンタを2インクリメントする。
      if (!key[state.reg_v[x]])
      {
        state.IncrementPC();
      }
      break;
    }
    default:
    {
      HandleUnknownOpcode(opcode);
      break;
    }
  }
}

void Chip8::ExecuteOpF(const uint16_t& opcode)
{
  auto lastByte = opcode & 0x00FF;
  auto x = (opcode & 0x0F00) >> 8;
  switch (lastByte)
  {
    case 0x0007:
    {
      // Fx07 - LD Vx, DT
      // VxにDelay timerの値dtをセットする。
      state.reg_v[x] = state.delay_timer;
      break;
    }
    case 0x000A:
    {
      // Fx0A - LD Vx, K
      // 押されたキーをVxにセットする。
      // キーが入力されるまで全ての実行をストップする。
      // キーが押されるとその値をVxにセットする。
      bool pressedAnyKey = false;
      for (auto i = 0; i < key.size(); ++i)
      {
        if (key[i])
        {
          // MEMO: これで合ってる？
          pressedAnyKey = true;
          state.reg_v[x] = i;
          break;
        }
      }
      if (!pressedAnyKey)
      {
        // PCの位置をもとに戻す
        state.DecrementPC();
      }
      break;
    }
    case 0x0015:
    {
      // Fx15 - LD DT, Vx
      // Delay timer dtにVxをセットする。
      state.delay_timer = state.reg_v[x];
      break;
    }
    case 0x0018:
    {
      // Fx18 - LD ST, Vx
      // Sound timer stにVxをセットする。
      state.sound_timer = state.reg_v[x];
      break;
    }
    case 0x001E:
    {
      // Fx1E - ADD I, Vx
      // IにI + Vxをセットする。
      state.reg_i = state.reg_i + state.reg_v[x];
      break;
    }
    case 0x0029:
    {
      // Fx29 - LD F, Vx
      // IにVxのスプライト(fontset)のアドレスをセットする。
      // Iの値にVxの値に対応するスプライト(fontset)のアドレスをセットする。
      // 詳しくはDisplayを参照。
      // MEMO: フォントは1文字5バイト, 0~Fを文字にする。
      // TODO: マジックナンバーを定数に
      state.reg_i = state.reg_v[x] * 5;
      break;
    }
    case 0x0033:
    {
      // Fx33 - LD B, Vx
      // アドレスI, I+1、I+2にVxのBCDをセットする。
      // アドレスIにVxの下位3桁目の値をセットする。I+1には下位2桁目の値をセットし、I+2には下位1桁目の値をセットする。
      // MEMO: 他の人の実装見ると、おそらく10進数で考えるっぽい
      state.memory[state.reg_i] = state.reg_v[x] / 100;
      state.memory[state.reg_i + 1] = (state.reg_v[x] % 100) / 10;
      state.memory[state.reg_i + 2] = (state.reg_v[x] % 10);
      break;
    }
    case 0x0055:
    {
      // Fx55 - LD [I], Vx
      // V0からVxまでの値をIから始まるアドレスにセットする。
      for (auto i = 0; i <= x; ++i)
      {
        state.memory[state.reg_i + i] = state.reg_v[i];
      }
      break;
    }
    case 0x0065:
    {
      // Fx65 - LD Vx, [I]
      // アドレスIから読んだ値をV0からVxにセットする。
      for (auto i = 0; i <= x; ++i)
      {
        state.reg_v[i] = state.memory[state.reg_i + i];
      }
      break;
    }
    default:
    {
      HandleUnknownOpcode(opcode);
      break;
    }
  }
}

void Chip8::HandleUnknownOpcode(const uint16_t& opcode)
{
  fmt::println("unknown opcode: {:x}", opcode);
}
