#include "nes_sys.h"
#include "debug.h"
#include <stdio.h>

NesSystem::NesSystem(Video* video, PlayPad *_pad) : pad(_pad) {
    mmc = new MMC();
    ppu = new PPU(mmc, video);
    ram = new memory(mmc, ppu, pad);
    cpu = new cpu_6502(ram);
    rom = new nes_file();
    ppu->setNMI(&cpu->NMI);
    state = 0;
}

NesSystem::~NesSystem() {
    delete cpu;
    delete ram;
    delete ppu;
    delete mmc;
    delete rom;
    delete pad;
}

void NesSystem::drawFrame() {
    static int cpu_cyc = 0;

    ppu->startNewFrame();
    int line = 0;
    int x = 0, y = 0;

    while (line++<PPU_DISPLAY_P_HEIGHT) {
        /* 绘制一行 */
        int ppu_cyc = 0, cyc = 0;

        while (cpu_cyc<P_HLINE_CPU_CYC) {
            cyc = cpu->process();
            cpu_cyc += cyc;
            ppu_cyc += cyc;

            if (ppu_cyc>=P_PIXEL_CPU_CYC) {
                ppu->drawPixel(x++, y);
                ppu_cyc -= P_PIXEL_CPU_CYC;
            }
        }
        cpu_cyc -= P_HLINE_CPU_CYC;
        x = 0; y++;

        /* 水平消隐周期 */
        while (cpu_cyc<P_HBLANK_CPU_CYC) {
            cpu_cyc += cpu->process();
        }
        cpu_cyc -= P_HBLANK_CPU_CYC;
    }
    ppu->drawSprite();
    ppu->oneFrameOver();

    /* 垂直消隐周期 */
    while (cpu_cyc<P_VBLANK_CPU_CYC) {
        cpu_cyc += cpu->process();
    }
    cpu_cyc -= P_VBLANK_CPU_CYC;
}

int NesSystem::load_rom(string filename) {
    int res = ::load_rom(rom, &filename);

    if (!res) {
        rom->romInfo();

        if (mmc->loadNes(rom)) {
            printf("INT vector(0xFFFA-0xFFFF): ");
            rom->printRom(0xFFFA - 0x8000, 6);
            ram->reset();
            ppu->swithMirror(rom->t1 & 0x0B);
            ppu->reset();
            cpu->RES = 1;
        } else {
            res = ER_LOAD_ROM_BADMAP;
        }
        state = res;
    }

    return res;
}

cpu_6502* NesSystem::getCpu() {
    if (state) {
        printf("> NesSystem::尚未初始化rom.\n");
    }
    return cpu;
}

PPU* NesSystem::getPPU() {
    return ppu;
}

memory* NesSystem::getMem() {
    return ram;
}

void NesSystem::debug() {
    static int debuging = 0;

    if (!debuging) {
        debuging = 1;
        printf("NES::start step debug.\n");
        debugCpu(this);
        debuging = 0;
    }
}
