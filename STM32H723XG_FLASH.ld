/*
******************************************************************************
**
**  File        : LinkerScript.ld
**
**  Author		: STM32CubeMX
**
**  Abstract    : Linker script for STM32H723VGTx series
**                1024Kbytes FLASH and 560Kbytes RAM
**
**                Set heap size, stack size and stack location according
**                to application requirements.
**
**                Set memory bank area and size if external memory is used.
**
**  Target      : STMicroelectronics STM32
**
**  Distribution: The file is distributed "as is," without any warranty
**                of any kind.
**
*****************************************************************************
** @attention
**
** <h2><center>&copy; COPYRIGHT(c) 2025 STMicroelectronics</center></h2>
**
** 版权声明和许可条款
**
*****************************************************************************
*/

/* 程序入口点 - 指定复位处理程序为程序的起始执行点 */
ENTRY(Reset_Handler)

/* 定义内存区域 */
MEMORY
{
/* 数据紧耦合内存 (Data TCM) - 128KB，零等待周期，用于高性能数据访问 */
DTCMRAM (xrw)      : ORIGIN = 0x20000000, LENGTH = 128K
/* D1域RAM (AXI SRAM) - 320KB，主系统内存 */
RAM_D1 (xrw)      : ORIGIN = 0x24000000, LENGTH = 320K
/* D2域RAM (AHB SRAM) - 32KB，用于外设数据缓冲 */
RAM_D2 (xrw)      : ORIGIN = 0x30000000, LENGTH = 32K
/* D3域RAM (备份域RAM) - 16KB，在低功耗模式下保持数据 */
RAM_D3 (xrw)      : ORIGIN = 0x38000000, LENGTH = 16K
/* 指令紧耦合内存 (Instruction TCM) - 64KB，零等待周期，用于高性能代码执行 */
ITCMRAM (xrw)      : ORIGIN = 0x00000000, LENGTH = 64K
/* 主闪存 - 1024KB，存储程序代码和常量数据 */
FLASH (rx)      : ORIGIN = 0x8000000, LENGTH = 1024K
}

/* 定义用户模式栈的最高地址（栈向低地址增长） */
_estack = ORIGIN(DTCMRAM) + LENGTH(DTCMRAM);    /* RAM的末尾作为栈起始位置 */

/* 如果堆和栈不能放入RAM则生成链接错误 */
_Min_Heap_Size = 0x4000;      /* required amount of heap  */
_Min_Stack_Size = 0x4000; /* required amount of stack */

/* 定义输出段 */
SECTIONS
{
  /* 启动代码首先放入FLASH */
  .isr_vector :
  {
    . = ALIGN(4);  /* 4字节对齐 */
    KEEP(*(.isr_vector)) /* 保留中断向量表（启动代码） */
    . = ALIGN(4);
  } >FLASH

  /* 程序代码和其他数据放入FLASH */
  .text :
  {
    . = ALIGN(4);
    *(.text)           /* 所有.text段（代码） */
    *(.text*)          /* 所有.text*段（代码） */
    *(.glue_7)         /* ARM和Thumb代码粘合段 */
    *(.glue_7t)        /* Thumb和ARM代码粘合段 */
    *(.eh_frame)       /* 异常处理帧信息 */

    KEEP (*(.init))    /* 保留初始化代码 */
    KEEP (*(.fini))    /* 保留终止化代码 */

    . = ALIGN(4);
    _etext = .;        /* 定义代码结束的全局符号 */
  } >FLASH

  /* 常量数据放入FLASH */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)         /* .rodata段（常量、字符串等） */
    *(.rodata*)        /* .rodata*段（常量、字符串等） */
    . = ALIGN(4);
  } >FLASH

  /* ARM异常处理表段 */
  .ARM.extab (READONLY) : /* "READONLY"关键字仅在GCC11及以后版本支持，如果使用GCC10或更早版本请移除 */
  {
    . = ALIGN(4);
    *(.ARM.extab* .gnu.linkonce.armextab.*)
    . = ALIGN(4);
  } >FLASH

  /* ARM异常索引表段 */
  .ARM (READONLY) : /* "READONLY"关键字仅在GCC11及以后版本支持，如果使用GCC10或更早版本请移除 */
  {
    . = ALIGN(4);
    __exidx_start = .;  /* 异常索引表开始 */
    *(.ARM.exidx*)
    __exidx_end = .;    /* 异常索引表结束 */
    . = ALIGN(4);
  } >FLASH

  /* 预初始化数组段 */
  .preinit_array (READONLY) : /* "READONLY"关键字仅在GCC11及以后版本支持，如果使用GCC10或更早版本请移除 */
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
    . = ALIGN(4);
  } >FLASH

  /* 初始化数组段 */
  .init_array (READONLY) : /* "READONLY"关键字仅在GCC11及以后版本支持，如果使用GCC10或更早版本请移除 */
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))  /* 排序的初始化数组 */
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
    . = ALIGN(4);
  } >FLASH

  /* 终止化数组段 */
  .fini_array (READONLY) : /* "READONLY"关键字仅在GCC11及以后版本支持，如果使用GCC10或更早版本请移除 */
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))  /* 排序的终止化数组 */
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
    . = ALIGN(4);
  } >FLASH

  /* 用于启动时初始化数据的加载地址 */
  _sidata = LOADADDR(.data);

  /* 已初始化数据段放入RAM，但在FLASH中保留加载镜像 */
  .data :
  {
    . = ALIGN(4);
    _sdata = .;        /* 在数据开始处创建全局符号 */
    *(.data)           /* .data段 */
    *(.data*)          /* .data*段 */
    *(.RamFunc)        /* 在RAM中执行的函数段 */
    *(.RamFunc*)       /* 在RAM中执行的函数段 */

    . = ALIGN(4);
    _edata = .;        /* 在数据结束处定义全局符号 */
  } >DTCMRAM AT> FLASH  /* VMA在DTCMRAM，LMA在FLASH */

  /* 已初始化TLS（线程局部存储）数据段 */
  .tdata : ALIGN(4)
  {
    *(.tdata .tdata.* .gnu.linkonce.td.*)
    . = ALIGN(4);
    PROVIDE(__tdata_end = .);
  } >DTCMRAM AT> FLASH

  /* 提供TLS相关符号供启动代码和运行时使用 */
  PROVIDE( __tdata_start = ADDR(.tdata) );
  PROVIDE( __tdata_size = __tdata_end - __tdata_start );

  PROVIDE( __data_start = ADDR(.data) );
  PROVIDE( __data_end = _edata );
  PROVIDE( __data_size = __data_end - __data_start );

  PROVIDE( __tdata_source = LOADADDR(.tdata) );
  PROVIDE( __tdata_source_end = LOADADDR(.tdata) + SIZEOF(.tdata) );
  PROVIDE( __tdata_source_size = __tdata_source_end - __tdata_source );

  PROVIDE( __data_source = LOADADDR(.data) );
  PROVIDE( __data_source_end = LOADADDR(.data) + SIZEOF(.data) );
  PROVIDE( __data_source_size = __data_source_end - __data_source );

  /* 未初始化的TLS BSS段 */
  .tbss (NOLOAD) : ALIGN(4)
  {
     /* 启动代码使用此符号来初始化.bss段 */
    _sbss = .;         /* 在bss开始处定义全局符号 */
    __bss_start__ = _sbss;
    *(.tbss .tbss.*)
    . = ALIGN(4);
    PROVIDE( __tbss_end = . );
  } >DTCMRAM

  /* 提供TLS BSS相关符号 */
  PROVIDE( __tbss_start = ADDR(.tbss) );
  PROVIDE( __tbss_size = __tbss_end - __tbss_start );
  PROVIDE( __tbss_offset = ADDR(.tbss) - ADDR(.tdata) );

  /* 提供TLS基础信息 */
  PROVIDE( __tls_base = __tdata_start );
  PROVIDE( __tls_end = __tbss_end );
  PROVIDE( __tls_size = __tls_end - __tls_base );
  PROVIDE( __tls_align = MAX(ALIGNOF(.tdata), ALIGNOF(.tbss)) );
  PROVIDE( __tls_size_align = (__tls_size + __tls_align - 1) & ~(__tls_align - 1) );
  PROVIDE( __arm32_tls_tcb_offset = MAX(8, __tls_align) );
  PROVIDE( __arm64_tls_tcb_offset = MAX(16, __tls_align) );

  /* 常规BSS段（未初始化数据） */
  .bss (NOLOAD) : ALIGN(4)
  {
    *(.bss)
    *(.bss*)
    *(COMMON)          /* 未初始化的全局变量 */

    . = ALIGN(4);
    _ebss = .;         /* 在bss结束处定义全局符号 */
    __bss_end__ = _ebss;
    PROVIDE( __bss_end = .);
  } >DTCMRAM

  PROVIDE( __non_tls_bss_start = ADDR(.bss) );

  /* 提供完整的BSS信息（包含TLS和非TLS） */
  PROVIDE( __bss_start = _sbss );
  PROVIDE( __bss_size = __bss_end - __bss_start );

  /* 用户堆栈段，用于检查剩余RAM是否足够 */
  ._user_heap_stack (NOLOAD) :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );      /* 堆起始位置 */
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;   /* 分配堆空间 */
    . = . + _Min_Stack_Size;  /* 分配栈空间 */
    . = ALIGN(8);
  } >DTCMRAM

  /* 从标准库中移除调试信息 */
  /DISCARD/ :
  {
    libc.a:* ( * )    /* 丢弃C库的调试信息 */
    libm.a:* ( * )    /* 丢弃数学库的调试信息 */
    libgcc.a:* ( * )  /* 丢弃GCC库的调试信息 */
  }
}
