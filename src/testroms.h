#ifndef TESTROMS_H_INCLUDED
#define TESTROMS_H_INCLUDED

#define TEST_ROM "rom/NEStress.nes"
//#define TEST_ROM "rom/Tennis.nes"
//#define TEST_ROM "rom/dkk.nes"
//#define TEST_ROM "rom/fighter_f8000.nes"
//#define TEST_ROM "rom/test/NES_Test_Cart.nes"

/* E765 程序处连续写入2005端口,导致同时修改x,y,显示不正确 */
#define TEST_ROM "rom/F-1.nes"

//#define TEST_ROM "H:\\VROMS\\FC_ROMS\\霸王的大陆.nes"
//#define TEST_ROM "H:\\VROMS\\FC_ROMS\\吞噬天地2.nes"
//#define TEST_ROM "H:\\VROMS\\FC_ROMS\\吞噬天地.nes"
//#define TEST_ROM "H:\\VROMS\\FC_ROMS\\魂斗罗.nes" // 7900断点

//#define TEST_ROM "rom\\test\\blargg_ppu_tests_2005.09.15b\\vram_access.nes"

#endif // TESTROMS_H_INCLUDED
