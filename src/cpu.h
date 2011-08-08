#ifndef CPU_H_INCLUDED
#define CPU_H_INCLUDED

#include "type.h"
#include "mem.h"

struct cpu_6502 {

/* NTSC制式机型运行频率为1.7897725 MHz
 * PAL制式机型运行频率为1.773447 MHz                       */
#define CPU_NTSC    1.7897725
#define CPU_PAL     1.773447

    byte A; 	    /* 累加器                              */
    byte Y;	    	/* 索引暂存器                          */
    byte X;	    	/* 索引暂存器                          */

    union {
        struct { byte PCL; byte PCH; };
        word PC;
    };  			/* 程序计数器,指向下一个要执行的指令   */

    word SP;		/* 堆叠指示器 0x0100-0x01FF
                     * 递减的, 指向'空', 每次2字节         */

#define CPU_FLAGS_NEGATIVE    (1<<7)  /* 负值              */
#define CPU_FLAGS_OVERFLOW    (1<<6)  /* 有溢出            */
#define CPU_FLAGS_BREAK       (1<<4)  /* 有中断            */
#define CPU_FLAGS_DECIMAL     (1<<3)  /* 十进制            */
#define CPU_FLAGS_INTERDICT   (1<<2)  /* 禁止中断          */
#define CPU_FLAGS_ZERO        (1<<1)  /* 结果为零          */
#define CPU_FLAGS_CARRY       1       /* 有进位            */

    byte FLAGS;	    /* 状态暂存器  N V 1 B D I Z C
					 * N负值 	V溢出		B中断命令
					 * D十进制	I插断禁能	Z零值
					 * C进位                               */
    memory *ram;    /* 内存                                */


    cpu_6502(memory* ram);

    void    reset();            /* 重置cpu状态             */
    void    push(byte d);       /* 向堆栈中压数            */
    byte    pop();              /* 从堆栈中取数            */
    void    debug();            /* 打印cpu状态             */
    byte    process();          /* 处理当前命令并指向下一条命
                                 * 令,返回使用的处理器周期 */

    /* 如果value最高位为1 则N=1,否则为0,
     * 如果value==0 则Z=1,否则为0 */
    inline void checkNZ(byte value) {
        FLAGS &= 0xFF ^ CPU_FLAGS_NEGATIVE;
        FLAGS |= value ? CPU_FLAGS_NEGATIVE : 0;

        FLAGS &= 0xFF ^ CPU_FLAGS_ZERO;
        FLAGS |= value ? CPU_FLAGS_ZERO : 0;
    }

    /* 如果value>0xFF则c=1,否则=0
     * 如果value与beforeOper的最高位不同,则v=1否则为0
     * beforeOper是参与运算之前的累加器的值 */
    inline void checkCV(word value, byte beforeOper) {
        if (value>0xFF) {
            FLAGS |= CPU_FLAGS_CARRY;
        } else {
            FLAGS &= 0xFF ^ CPU_FLAGS_CARRY;
        }
        if (value & beforeOper & 0x80) {
            FLAGS |= CPU_FLAGS_OVERFLOW;
        } else {
            FLAGS &= 0xFF ^ CPU_FLAGS_OVERFLOW;
        }
    }
};

/* 向命令处理函数传递参数          */
struct command_parm {

    memory   *ram;
    cpu_6502 *cpu;

    byte op; /* 命令的代码         */
    byte p1; /* 第一个参数(如果有) */
    byte p2; /* 第二个参数(如果有) */

static const byte ADD_MODE_$zpgX$  = 0x00;
static const byte ADD_MODE_zpg     = 0x04;
static const byte ADD_MODE_imm     = 0x08;
static const byte ADD_MODE_abs     = 0x0C;
static const byte ADD_MODE_$zpg$Y  = 0x10;
static const byte ADD_MODE_zpgX    = 0x14;
static const byte ADD_MODE_absY    = 0x18;
static const byte ADD_MODE_absX    = 0x1C;

    /* 按照内存寻址方式返回该地址的上的数据            *
     * 不同寻址类型的相同指令编码品偏移量相同(也有例外)*
     * 于是,可以用cpu指令减去偏移得到该指令的寻址类型  */
    inline byte read(const byte addressing_mode) {
        word offset = 0;

        switch (addressing_mode) {

        case ADD_MODE_imm:
            return p1;

        case ADD_MODE_abs:
            offset = abs();
            break;

        case ADD_MODE_zpg:
            offset = zpg();
            break;

        case ADD_MODE_absX:
            offset = absX();
            break;

        case ADD_MODE_absY:
            offset = absY();
            break;

        case ADD_MODE_zpgX:
            offset = zpgX();
            break;

        case ADD_MODE_$zpg$Y:
            offset = $zpg$Y();
            break;

        case ADD_MODE_$zpgX$:
            offset = $zpgX$();
            break;
        }

        return ram->read( offset );
    }

/* 寻址算法定义, 函数返回地址      */
    inline word abs() {
        return (p2<<8) | p1;
    }
    inline word absX() {
        return abs() + cpu->X;
    }
    inline word absY() {
        return abs() + cpu->Y;
    }
    inline word zpg() {
        return p1 & 0x00FF;
    }
    inline word zpgX() {
        return (p1 + cpu->X) & 0x00FF;
    }
    inline word zpgY() {
        return (p1 + cpu->Y) & 0x00FF;
    }
    inline word $zpg$Y() {
        return $$$(0, cpu->Y);
    }
    inline word $zpgX$() {
        return $$$(cpu->X, 0);
    }
    inline word $$$(byte x, byte y) {
        word offset = 0;
        byte l = 0;
        byte h = 0;

        offset = ram->read( (p1 + x) & 0x00FF );
        l = ram->read( offset   );
        h = ram->read( offset+1 ) + y;
        return h<<8 | l;
    }
};

struct command_6502 {

    char    name[4];
    byte    time;
    byte    len;

    enum value_type {
      vt_op_not = 0
    , vt_op_imm   /* 立即1              */
    , vt_op_zpg   /* 零页2 , 高位为0x00 */
    , vt_op_rel   /* 相对3              */
    , vt_op_abs   /* 绝对4              */
    , vt_op_ind   /* 间接5              */
    } type;

    /* 指向处理函数的指针, cmd 是命令描述*/
    void (*op_func)(command_6502* cmd, command_parm* parm);
};

#endif // CPU_H_INCLUDED
