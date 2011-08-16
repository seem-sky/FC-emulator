#include "ppu.h"
#include <stdio.h>
#include <string.h>

#define DC(b, x)    (b), (b+x), (b+x+x), (b+x+x+x)
#define WHI(x)      (x + (x<<8) +(x<<16))
#define BLK(x)      (x), (x+32), (x+64)

/* ��ɫӳ��, ����ΪFCϵͳ��ɫ, ֵΪRGB��ɫ */
T_COLOR ppu_color_table[0x40] = {
     WHI(0x7F)
    ,DC(0x000080, 0x200000)
    ,DC(0x800000, 0x002000)
    ,DC(0x008000, 0x000020)
    ,BLK(0)

    ,WHI(0xA9)
    ,DC(0x0000FF, 0x200000)
    ,DC(0xFF0000, 0x002000)
    ,DC(0x00FF00, 0x000020)
    ,BLK(8)

    ,WHI(0xD3)
    ,DC(0x4040FF, 0x200000)
    ,DC(0xFF4040, 0x002000)
    ,DC(0x40FF40, 0x000020)
    ,BLK(16)

    ,WHI(0xFF)
    ,DC(0x8080FF, 0x200000)
    ,DC(0xFF8080, 0x002000)
    ,DC(0x80FF80, 0x000020)
    ,BLK(24)
};

#undef DC
#undef WHI
#undef BLK

PPU::PPU(MMC *_mmc, Video *_video)
    : addr_add(1), ppuSW(pH), w2005(wX), mmc(_mmc)
    , video(_video)
{
}

void PPU::controlWrite(word addr, byte data) {
    switch (addr % 0x08) {

    case 0: /* 0x2000                      */
        control_2000(data);
        break;
    case 1: /* 0x2001                      */
        control_2001(data);
        break;
    case 3: /* 0x2003 �޸Ŀ�ͨ�����ڴ�ָ�� */
        spWorkOffset = data;
printf("�޸ľ���ָ��:%x\n", data);
        break;
    case 4: /* 0x2004 д�뿨ͨ�����ڴ�     */
printf("д�뾫������: %x = %x\n", spWorkOffset, data);
        spWorkRam[spWorkOffset++] = data;
        break;

    case 5: /* 0x2005 ���ô�������         */
        if (w2005==wX) {
            winX &= 0xFF00;
            winX |= data;
            w2005 = wY;
        } else {
            winY &= 0xFF00;
            winY |= data;
            w2005 = wX;
        }
        break;

    case 6: /* 0x2006 PPU��ַָ��           */
        if (ppuSW==pH) {
            ppu_ram_p = (ppu_ram_p & 0x00FF) | (data<<8);
            //ppu_ram_p %= 0x4000;
            ppuSW = pL;
        } else {
            ppu_ram_p = (ppu_ram_p & 0xFF00) | data;
            ppuSW = pH;
        }
printf("�޸�PPUָ��:%04x\n", ppu_ram_p);
        break;

    case 7: /* 0x2007 д���ݼĴ���          */
printf("��PPUд����:%04X = %04X\n", ppu_ram_p, data);
        write(data);
        break;
    }
}

#define _BIT(x)  (data & (1<<x))

inline void PPU::control_2000(byte data) {
    if (_BIT(0)) {
        winX |= 0x0100;
    } else {
        winX &= 0x00FF;
    }

    if (_BIT(1)) {
        winY |= 0x0100;
    } else {
        winY &= 0x00FF;
    }

    addr_add    = _BIT(2) ? 0x20   : 0x01;
    spRomOffset = _BIT(3) ? 0x1000 : 0;
    bgRomOffset = _BIT(4) ? 0x1000 : 0;
    spriteType  = _BIT(5) ? t8x16  : t8x8;
    sendNMI     = _BIT(7) ? 1      : 0;
    // D6λ PPU ��/��ģʽ, û����NES��ʹ��
}

inline void PPU::control_2001(byte data) {
    hasColor    = _BIT(0) ? 0 : 1;
    bkleftCol   = _BIT(1) ? 0 : 1;
    spleftCol   = _BIT(2) ? 0 : 1;
    bkAllDisp   = _BIT(3) ? 0 : 1;
    spAllDisp   = _BIT(4) ? 0 : 1;
    red         = _BIT(5) ? 1 : 0;
    green       = _BIT(6) ? 1 : 0;
    blue        = _BIT(7) ? 1 : 0;
}

#undef _BIT

byte PPU::readState(word addr) {
    byte r = 0xFF;

    switch (addr % 0x08) {

    case 2: /* 0x2002                       */
        r = 0;
        if ( spOverflow ) r |= ( 1<<5 );
        if ( spClash    ) r |= ( 1<<6 );
        if ( vblankTime ) r |= ( 1<<7 );
        vblankTime = 0;
        ppuSW      = pH;
        w2005      = wX;
        break;

    case 7: /* 0x2007 д���ݼĴ���          */
        r = read();
        break;
    }

    return r;
}

BackGround* PPU::swBg() {
    word offs = (ppu_ram_p % 0x3000 - 0x2000) / 0x400;
    switch (offs) {
    case 0:
        return bg0;
    case 1:
        return bg1;
    case 2:
        return bg2;
    case 3:
        return bg3;
    }
    return NULL;
}

inline byte PPU::read() {
    byte res = 0xFF;

    if (ppu_ram_p<0x2000) {
        res = mmc->readVRom(ppu_ram_p);
    } else

    if (ppu_ram_p<0x3EFF) {
        BackGround* pBg = swBg();
        res = pBg->read(ppu_ram_p);
    } else

    if (ppu_ram_p<0x3FFF) {
        word off = ppu_ram_p % 0x20;
        if (off<0x10) {
            res = bkPalette[off     ];
        } else {
            res = spPalette[off-0x10];
        }
    }

    ppu_ram_p += addr_add;
    return res;
}

inline void PPU::write(byte data) {
    if (ppu_ram_p<0x2000) {
#ifdef SHOW_ERR_MEM_OPERATE
        printf("PPU: can't write vrom $%04x=%02x.\n", ppu_ram_p, data);
//        return;
#endif
    } else

    if (ppu_ram_p<0x3EFF) {
        BackGround* pBg = swBg();
        pBg->write(ppu_ram_p, data);
    } else

    if (ppu_ram_p<0x3FFF) {
        word off = ppu_ram_p % 0x20;
        if (off<0x10) {
            bkPalette[off     ] = data;
        } else {
            spPalette[off-0x10] = data;
        }
    }

    ppu_ram_p += addr_add;
}

void PPU::swithMirror(byte type) {
    switch (type) {

    case PPU_VMIRROR_VERTICAL:
        bg0 = bg2 = &bg[0];
        bg1 = bg3 = &bg[1];
    break;

    case PPU_VMIRROR_HORIZONTAL:
        bg0 = bg1 = &bg[0];
        bg2 = bg3 = &bg[1];
    break;

    case PPU_VMIRROR_SINGLE:
        bg0 = bg1 = bg2 = bg3 = &bg[0];
    break;

    case PPU_VMIRROR_4_LAYOUT:
        bg0 = &bg[0];
        bg1 = &bg[1];
        bg2 = &bg[2];
        bg3 = &bg[3];
    break;
    }
}

void PPU::reset() {
    spOverflow = 0;
    spClash    = 0;
    *NMI       = 0;
    preheating = 3;
}

void PPU::setNMI(byte* cpu_nmi) {
    NMI = cpu_nmi;
}

void PPU::drawTileTable() {
    byte dataH, dataL;
    byte colorIdx;
    int x=0, y=0;

    for (int p=0; p<512; ++p) {
        for (int i=0; i<8; ++i) {
            dataL = mmc->readVRom(p * 16 + i);
            dataH = mmc->readVRom(p * 16 + i + 8);

            for (int d=7; d>=0; --d) {
                colorIdx = 0;
                if (dataL>>d & 1) {
                    colorIdx |= 1;
                }
                if (dataH>>d & 1) {
                    colorIdx |= 2;
                }
                video->drawPixel(x++, y, ppu_color_table[colorIdx]);
            }
            x-= 8;
            y++;
        }
        x+= 8;
        y-= 8;
        if (p && p%32==0) {
            x = 0;
            y+= 8;
        }
    }
}

void PPU::drawSprite() {
    byte dataH, dataL;
    byte colorIdx;
    int  i;
    for (int sp=63; sp>=0; --sp) {
        i = sp * 4;
        byte x    = spWorkRam[i  ];
        byte y    = spWorkRam[i+3] - 1;
        byte tile = spWorkRam[i+1];
        byte ctrl = spWorkRam[i+2];
        word tileAddr =  tile*16;

        for (int r=0; r<8; ++r) {
            dataL   = mmc->readVRom(tileAddr + r    );
            dataH   = mmc->readVRom(tileAddr + r + 8);

            for (int d=7; d>=0; --d) {
                colorIdx = 0;
                if (dataL>>d & 1) {
                    colorIdx |= 1;
                }
                if (dataH>>d & 1) {
                    colorIdx |= 2;
                }
                //if (colorIdx) {
                    colorIdx |= ((ctrl & 0x03) << 2);
                    video->drawPixel(x++, y, ppu_color_table[colorIdx]);
                //}
            }
            x-=8;
            y++;
        }
    }
}

void PPU::drawPixel(int X, int Y) {
/*-----------------| ���Ʊ��� |-------------------*/
    word nameIdx = (X/8) * (Y/8);
    word tileIdx = bg0->name[nameIdx];
    word tileAddr= bgRomOffset + tileIdx*16 + Y%8;

    byte tile0 = mmc->readVRom(tileAddr    );
    byte tile1 = mmc->readVRom(tileAddr + 8);
    byte tileX = 1<<(7 - X%8);
/*
 if (tileIdx&&0)
printf("x:%03d \t y:%03d \t nameIdx:%d \t tileIdx:%d \t bgoff:%d\n"
       , X, Y, nameIdx, tileIdx, bgRomOffset);
*/
    byte paletteIdx = 0;
    if (tile0 & tileX) paletteIdx |= 0x01;
    if (tile1 & tileX) paletteIdx |= 0x02;

    if (paletteIdx) {
        byte attrIdx = X/8 + Y/8;
        paletteIdx |= bg0->attribute[attrIdx] << 2;
        byte colorIdx = bkPalette[paletteIdx];
        video->drawPixel(X, Y, ppu_color_table[paletteIdx]);
    }
/*----------------| end |-------------------------*/
}

void PPU::oneFrameOver() {
    if (sendNMI || preheating) {
        *NMI = 1;
        preheating >>= 1;
#ifdef NMI_DEBUG
        printf("PPU::�����жϵ�CPU\n");
#endif
    }
    vblankTime = 1;
}

void PPU::startNewFrame() {
    vblankTime = 0;
}

void PPU::copySprite(byte *data) {
    memcpy(spWorkRam, data, 256);
}