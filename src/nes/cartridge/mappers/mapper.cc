#include "mapper.h"

#include "mapper_000.h"
#include "mapper_001.h"
#include "mapper_002.h"
#include "mapper_003.h"
#include "mapper_004.h"

Mapper* Mapper::Factory(const ROM_File& rom_file) {
  if (rom_file.is_valid == false)
    return nullptr;

  switch (rom_file.meta.mapper) {
  case 0: return new Mapper_000(rom_file);
  case 1: return new Mapper_001(rom_file);
  case 2: return new Mapper_002(rom_file);
  case 3: return new Mapper_003(rom_file);
  // case 4: return new Mapper_004(rom_file);
  }

  return nullptr;
}
