#ifndef PPU_H_INCLUDED
#define PPU_H_INCLUDED

#include "mmc.h"
#include "video.h"

#define PPU_DISPLAY_P_WIDTH     256
#define PPU_DISPLAY_P_HEIGHT    240

#define PPU_DISPLAY_N_WIDTH     256
#define PPU_DISPLAY_N_HEIGHT    224

#define PPU_VMIRROR_VERTICAL    0x01
#define PPU_VMIRROR_HORIZONTAL  0x00
#define PPU_VMIRROR_SINGLE      0x03
#define PPU_VMIRROR_4_LAYOUT    0x08

/*---------------------------------------| PAL Info |-------------*/
#define P_HLINE_CPU_CYC       1024       /* ÿ�л�������          */
#define P_HBLANK_CPU_CYC       338       /* ÿ��ˮƽ��������      */
#define P_VLINE_COUNT          312       /* ÿ֡ɨ����            */
#define P_VBLANK_CPU_CYC     35469       /* ��ֱ��������          */

/* ÿ֡����    */
#define P_FRAME_CPU_CYC       \
            ( (P_HLINE_CPU_CYC+P_HBLANK_CPU_CYC) * P_VLINE_COUNT )

/* ÿ��������  */
#define P_PIXEL_CPU_CYC       \
            ( P_HLINE_CPU_CYC / PPU_DISPLAY_P_WIDTH )
/*----------------------------------------------------------------*/

struct BackGround {

    byte name      [0x03C0];
    byte attribute [0x0040];

    inline void write(word offset, byte data) {
        word off = (offset % 0x0400);
        if (off<0x03C0) {
            name[off] = data;
        } else {
            attribute[off-0x03C0] = data;
        }
    }

    inline byte read(word offset) {
        word off = (offset % 0x0400);
        if (off<0x03C0) {
            return name[off];
        } else {
            return attribute[off-0x03C0];
        }
    }
};

/*----------------------------------------| vrom ӳ�� |----*
 * ÿ�� 32�� x 30�� ��ͼ�ε�Ԫ, ����ʾ960����Ԫ            *
 * ÿ��ͼ�ε�Ԫ8x8����,16�ֽ�,ÿ���Ᵽ��256��ͼ�ε�Ԫ      *
 * ÿ��ͬ��64����ͨ��Ԫ(һ��ҳ0x100�ֽ�)                   *
 * ��ͨ�������ڴ���, 4�ֽ�: 1.Y 2.�ֿ���� 3.��״ 4.X      *
 *                                                         *
 * $0000-$0FFF ��ͨͼ�ο�                                  *
 * $1000-$1FFF �����ַ�ͼ��                                *
-*----------------------------------------| vram ӳ�� |----*
 * $2000-$23BF ������һҳӳ�� 960(�ֽ�)��ͼ�ε�Ԫ          *
 * $23C0-$23FF ������һҳ��ɫ�� 64(�ֽ�)����ɫ��Ԫ         *
 *                                            (һ��ӳ��1KB)*
 * $2400-$27BF �����ڶ�ҳӳ��                              *
 * $27C0-$27FF �����ڶ�ҳ��ɫ�� 0x7FF 2KB                  *
 *                                                         *
 * $2800-$2BBF ��������ҳӳ��                              *
 * $2BC0-$2BFF ��������ҳ��ɫ�� 0xBFF 3KB                  *
 *                                                         *
 * $2C00-$2FBF ��������ҳӳ��                              *
 * $2FC0-$2FFF ��������ҳ��ɫ�� 0xFFF 4KB                  *
 *                                                         *
 * $3000-$3EFF $2000 - $2EFF �ľ���                        *
 * $3F00-$3F1F ������ͨ��ɫ�������� ��16�ֽ�               *
 * $3F20-$3FFF Ϊ��  $3F00-$3F1F ��7�ξ���                 *
 * $4000-$FFFF $0000 - $3FFF �ľ���                      *
-*----------------------------------------| vram ӳ�� |----*/
struct PPU {

private:
    BackGround  bg[4];

    BackGround *bg0;
    BackGround *bg1;
    BackGround *bg2;
    BackGround *bg3;

    byte bkPalette[16];     /* ������ɫ��                          */
    byte spPalette[16];     /* ��ͨ��ɫ��                          */

    byte spWorkRam[256];    /* ��ͨ�����ڴ�                        */
    word spWorkOffset;      /* ��ͨ����ҳ���׵�ַ                  */
    word ppu_ram_p;         /* ppu�Ĵ���ָ��                       */
    byte addr_add;          /* ��ַ�����ۼ�ֵ                      */
    enum { pH, pL } ppuSW;  /* д��ppu�Ĵ�����λ�� $0000-$3FFF     */

    word winX;
    word winY;
    enum { wX, wY } w2005;  /* д����һ������                      */

    word spRomOffset;       /* ��ͨ�ֿ��׵�ַ                      */
    word bgRomOffset;       /* �����ֿ��׵�ַ                      */
    enum { t8x8, t8x16 } spriteType;

    byte *NMI;              /* cpu��NMI��ַ�ߣ�������cpu����NMI    */
    byte sendNMI;           /* �Ƿ���ˢ��һ֡����NMI             */
    byte preheating;        /* PPUԤ����Ϊ3                        */
    byte vblankTime;        /* ������ڴ�ֱ����ʱ����Ϊ1           */

    byte bkleftCol;         /* ������ʾ��һ��                      */
    byte spleftCol;         /* ��ͨ��ʾ��һ��                      */
    byte bkAllDisp;         /* ����ȫ��ʾ                          */
    byte spAllDisp;         /* ��ͨȫ��ʾ                          */

    byte hasColor;          /* ����ɫ��                            */
    byte red;               /* ��ɫ��ɫ                            */
    byte green;             /* ��ɫ��ɫ                            */
    byte blue;              /* ��ɫ��ɫ                            */

    byte spOverflow;        /* ��ͨ8�����                         */
    byte spClash;           /* ��ͨ��ͻ?                           */

    MMC   *mmc;
    Video *video;

    void control_2000(byte data);
    void control_2001(byte data);

    void write(byte);       /* д����                              */
    byte read();            /* ������                              */
    BackGround* swBg();     /* ����ppu_ram_p��ֵ�õ���Ӧ�ı���ָ�� */

public:
    PPU(MMC *mmc, Video *video);

    void reset();
    /* cpuͨ��д0x2000~0x2007(0x3FFF)����PPU                       */
    void controlWrite(word addr, byte data);
    /* cpuͨ����0x2000~0x2007(0x3FFF)�õ�PPU״̬                   */
    byte readState(word addr);
    /* �л���Ļ����                                                */
    void swithMirror(byte type);
    /* ����cpu��NMI��ַ��                                          */
    void setNMI(byte* cpu_nmi);
    /* �������Ʊ����ֿ�                                            */
    void drawTileTable();
    /* ����ָ��λ�õ�����                                          */
    void drawPixel(int x, int y);
    /* ��һ֡�������ʱ�����Է����ж�, ϵͳԤ��ʱҲ��Ҫ��������    */
    void oneFrameOver();
    /* ����ʼ����һ���µ�֡ʱ,�÷���������                         */
    void startNewFrame();
    /* ����256�ֽڵ����ݵ�����Ram                                  */
    void copySprite(byte *data);
    /* ����һ֡�еľ��� */
    void drawSprite();
};

#endif // PPU_H_INCLUDED