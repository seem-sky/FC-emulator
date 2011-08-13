#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string>
#include "nes_sys.h"

const int test_command = 200;

class CmdVideo : public Video {
    void drawPixel(int x, int y, T_COLOR color) {
        printf("(%x,%x)=%x\t", x, y, color);
    }
};

void test() {

    using std::string;

    string filename = "rom/Tennis.nes"; //"rom/Dr_Mario.nes";

    CmdVideo video;
    NesSystem fc(&video);

    switch (fc.load_rom(filename)) {
    case LOAD_ROM_SUCCESS:
        goto _LOAD_SUCCESS;

    case ER_LOAD_ROM_PARM:
        printf("parm not null.\n");
        break;
    case ER_LOAD_ROM_OPEN:
        printf("\ncannot open '%s' file.\n", filename.c_str());
        break;
    case ER_LOAD_ROM_HEAD:
        printf("The file '%s' not nes rom. \n", filename.c_str());
        break;
    case ER_LOAD_ROM_TRAINER:
        printf("The 'trainer' not enough '%s'.\n", filename.c_str());
        break;
    case ER_LOAD_ROM_SIZE:
        printf("cannot read enough rom size.\n");
        break;
    case ER_LOAD_ROM_VSIZE:
        printf("cannot read enough vrom size.\n");
        break;
    case ER_LOAD_ROM_BADMAP:
        printf("unsupport map id\n");
        break;
    default:
        printf("unknow error.\n");
    }

    return;

_LOAD_SUCCESS:

    printf("load '%s' over, start..\n\n", filename.c_str());

    cpu_6502* cpu = fc.getCpu();
    PPU *ppu = fc.getPPU();

    int c=0;

    clock_t s = clock();

    for (;;) {
        if (cpu->process() || c++<test_command) {
            //printf(cpu->cmdInfo());
            ppu->drawNextPixel();
        } else {
            break;
        }
    }

    clock_t e = clock();

    printf("执行 %d 条指令使用了 %lf 毫秒\n", c,
           (double)(e - s)*(CLOCKS_PER_SEC/1000));

    printf(cpu->debug());
}

int main()
{
    welcome();
    test();
    system("pause");
    return 0;
}

