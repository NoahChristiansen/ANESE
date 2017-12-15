#pragma once

#include "common/util.h"
#include "common/interfaces/memory.h"
#include "common/bitfield.h"

#include "color.h"
#include "dma.h"
#include "nes/wiring/interrupt_lines.h"

namespace PPURegisters {
  enum Reg {
    PPUCTRL   = 0x2000,
    PPUMASK   = 0x2001,
    PPUSTATUS = 0x2002,
    OAMADDR   = 0x2003,
    OAMDATA   = 0x2004,
    PPUSCROLL = 0x2005,
    PPUADDR   = 0x2006,
    PPUDATA   = 0x2007,
    OAMDMA    = 0x4014,
  };
} // PPURegisters

// Picture Processing Unit
// This guy is NOT cycle-accurate at the moment!
// http://wiki.nesdev.com/w/index.php/PPU_programmer_reference
class PPU final : public Memory {
private:

  /*----------  "Hardware"  ----------*/
  // In quotes because technically, these things aren't located on the PPU, but
  // by coupling them with the PPU, it makes the emulator code cleaner

  // CPU WRAM -> PPU OAM Direct Memory Access (DMA) Unit
  DMA& dma;

  /*-----------  Hardware  -----------*/

  // ---- Core Hardware ---- //

  InterruptLines& interrupts;
  Memory& mem; // PPU 16 bit address space (should be wired to ppu_mmu)

  // ---- Sprite Hardware ---- //

  Memory& oam;  // Primary OAM - 256 bytes (Object Attribute Memory)
  Memory& oam2; // Secondary OAM - 32 bytes (8 sprites to render on scanline)

  // ---- Background Hardware ---- //

  // TBD

  // ---- Memory Mapped Registers ---- //

  u8 cpu_data_bus; // PPU <-> CPU data bus (filled on any register write)

  bool latch; // Controls which byte to write to in PPUADDR and PPUSCROLL
              // 0 = write to hi, 1 = write to lo

  struct { // Registers
    // PPUCTRL - 0x2000 - PPU control register
    union {
      u8 raw;
      BitField<7> V; // NMI enable
      BitField<6> P; // PPU master/slave
      BitField<5> H; // sprite height
      BitField<4> B; // background tile select
      BitField<3> S; // sprite tile select
      BitField<2> I; // increment mode
      BitField<0, 2> N; // nametable select
    } ppuctrl;

    // PPUMASK - 0x2001 - PPU mask register
    union {
      u8 raw;
      BitField<7> B; // color emphasis Blue
      BitField<6> G; // color emphasis Green
      BitField<5> R; // color emphasis Red
      BitField<4> s; // sprite enable
      BitField<3> b; // background enable
      BitField<2> M; // sprite left column enable
      BitField<1> m; // background left column enable
      BitField<0> g; // greyscale
    } ppumask;

    // PPUSTATUS - 0x2002 - PPU status register
    union {
      u8 raw;
      BitField<7> V; // vblank
      BitField<6> S; // sprite 0 hit
      BitField<5> O; // sprite overflow
      // the rest are irrelevant
    } ppustatus;

    u8 oamaddr; // OAMADDR - 0x2003 - OAM address port
    u8 oamdata; // OAMDATA - 0x2004 - OAM data port

    // PPUSCROLL - 0x2005 - PPU scrolling position register
    // PPUADDR   - 0x2006 - PPU VRAM address register
    //     Note: Both 0x2005 and 0x2006 map to the same internal register, v
    //     0x2005 is simply a convenience used to set scroll more easily!

    u8 ppudata; // PPUDATA - 0x2007 - PPU VRAM data port

    union {
      u16 val;            // the actual address
      BitField<8, 8> hi;  // hi byte of addr
      BitField<0, 8> lo;  // lo byte of addr

      // https://wiki.nesdev.com/w/index.php/PPU_scrolling
      BitField<0,  5> coarse_x;
      BitField<5,  5> coarse_y;
      BitField<10, 2> nametable;
      BitField<12, 3> fine_y;

      // quality-of-life method (so i don't have to use v.val _all_ the time)
      operator u16() const { return this->val; }
    } v, t;

    // v is the true vram address
    // t is the temp vram address

    u3 x; // fine x-scroll register
  } reg;

  // What about OAMDMA - 0x4014 - PPU DMA register?
  //
  // Well, it's not a register... hell, it's not even located on the PPU!
  //
  // DMA is handled by an external unit (this->dma), an object that has
  // references to both CPU WRAM and PPU OAM.
  // To make the emulator code simpler, the PPU object handles the 0x4014 call,
  // but all that actually happens is that it calls this->dma.transfer(), pushes
  // that data to OAMDATA, and cycles itself for the requisite number of cycles

  /*---- Helper Functions ----*/

  struct Pixel {
    Color color;    // pixel color (color.a determines if pixel is visible)
    bool  priority; // sprite priority bit (unused in brg_pixels)
  };

  struct {
    u8 nt_byte;
    u8 at_byte;
    u8 tile_lo;
    u8 tile_hi;
  } bgr;
  Pixel get_bgr_pixel();
  Pixel get_spr_pixel();

  void bgr_fetch();
  void spr_eval();

  /*----  Emulation Vars and Methods  ----*/

  uint cycles; // total PPU cycles
  uint frames; // total frames rendered

  // framebuffer
  u8* framebuff;
  void draw_dot(Color color);

  // scanline tracker
  struct {
    uint line;  // 0 - 261
    uint cycle; // 0 - 340
  } scan;

#ifdef DEBUG_PPU
  /*---------------  Debug  --------------*/
  // Implementation in debug.cc
  void   init_debug_windows();
  void update_debug_windows();
#endif // DEBUG_PPU

public:
  ~PPU();
  PPU(
    Memory& mem,
    Memory& oam,
    Memory& oam2,
    DMA& dma,
    InterruptLines& interrupts
  );

  // <Memory>
  u8 read(u16 addr) override;
  u8 peek(u16 addr) const override;
  void write(u16 addr, u8 val) override;
  // <Memory/>

  void power_cycle();
  void reset();

  void cycle();

  const u8* getFramebuff() const;
  uint getFrames() const;

  // NES color palette (static, for the time being)
  static const Color palette [64];
};
