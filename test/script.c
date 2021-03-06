OUTPUT_FORMAT("ecoff-littlemips")
ENTRY(__start)
SECTIONS
{
  .text  0 : {
     _ftext = . ;
    *(.init)
     eprol  =  .;
    *(.text)
    *(.fini)
     etext  =  .;
     _etext  =  .;
  }
  .rdata  . : {
    *(.rdata)
  }
   _fdata = .;
   _fdata_align = ((_fdata + PageSize - 1) & ~(PageSize - 1));
   . = _fdata_align;
  .data  . : {
    *(.data)
    CONSTRUCTORS
  }
   edata  =  .;
   _edata  =  .;
   _fbss = .;
  .sbss  . : {
    *(.sbss)
    *(.scommon)
  }
  .bss  . : {
    *(.bss)
    *(COMMON)
  }
   end = .;
   _end = .;
}
 
