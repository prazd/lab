#include "Bus.hpp"
#include "Constants.hpp"

void Bus::write(unsigned short addr, unsigned char value) {
    if (addr == constants::PPUCTRL)   {ppuctrl   =   {value, true}; return;}
    if (addr == constants::PPUMASK)   {ppumask   =   {value, true}; return;}
    if (addr == constants::OAMADDR)   {oamaddr   =   {value, true}; return;}
    if (addr == constants::OAMDATA)   {oamdata   =   {value, true}; return;}
    if (addr == constants::PPUSCROLL) {ppuscroll =   {value, true}; return;}
    if (addr == constants::PPUADDR)   {ppuaddr   =   {value, true}; return;}
    if (addr == constants::PPUDATA)   {ppudata   =   {value, true}; return;}
    if (addr == constants::OAMDMA)    {
        dma_address = value * 256;
        dma_active = true;
        dma_index = 0;
        return;
    }
    if (addr == constants::CONTROLLER1) {if (value == 1) c.latch1 = true;}
    if (addr == constants::CONTROLLER1) {if (value == 0) c.update_status1();}

    if (addr >= 0x8000 && addr <= 0xffff){
        if (mapper.mtype == Mapper::MapperType::MMC1){
            mapper.mmc1.handle(addr, value);
        }
    }
}

// All regs for debug
unsigned char Bus::read(unsigned short addr) {
    if (addr == constants::PPUCTRL)
        return ppuctrl.reg;
    if (addr == constants::PPUMASK)
        return ppumask.reg;
    if (addr == constants::PPUSTATUS)
        return ppustatus.reg;
    if (addr == constants::OAMADDR)
        return oamaddr.reg;
    if (addr == constants::OAMDATA)
        return oamdata.reg;
    if (addr == constants::PPUSCROLL)
        return ppuscroll.reg;
    if (addr == constants::PPUADDR)
        return ppuaddr.reg;
    if (addr == constants::PPUDATA)
        return ppudata.reg;
    if (addr == constants::CONTROLLER1)
        return c.read_status1();
    if (addr >= 0x8000 && addr <= 0xffff){
        return *mapper.read_rom(addr);
    }
    // dummy
    return 0xff;
}

unsigned char* Bus::read_ptr(unsigned short addr) {
    // todo: maybe return dummy reg addr
    if (addr == constants::PPUCTRL)
        return &ppuctrl.reg;
    if (addr == constants::PPUMASK)
        return &ppumask.reg;
    if (addr == constants::PPUSTATUS)
        return &ppustatus.reg;
    if (addr == constants::OAMADDR)
        return &oamaddr.reg;
    if (addr == constants::OAMDATA)
        return &oamdata.reg;
    if (addr == constants::PPUSCROLL)
        return &ppuscroll.reg;
    if (addr == constants::PPUADDR)
        return &ppuaddr.reg;
    if (addr == constants::PPUDATA)
        return &ppudata.reg;
    // TODO: check for controller
    if (addr >= 0x8000 && addr <= 0xffff){
        return mapper.read_rom(addr);
    }
    return nullptr;
}

void Bus::dma_tick(unsigned char val) {
    if (dma_active){
        oam[dma_index++] = val;
        dma_active = (dma_index != 256);
    }
}