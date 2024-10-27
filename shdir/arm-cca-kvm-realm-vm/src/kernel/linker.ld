ENTRY(_start)

MEMORY {
	RAM(rwx): ORIGIN = 0x40080000 , LENGTH = (96M - 0x80000) /*SYSTEM_RAM_BASE*/
};
ENTRY(_start);

SECTIONS 
{
		/*. = 0xFFFFF08000000000; KERNEL_VA_BASE;*/
		
		_TEXT_START = .;
		_binary_start = .;
		.vector 0xFFFFF08000000000 : AT(0x40080000) {  exception.o  } 
		. = ADDR(.vector) + SIZEOF(.vector);
		. = ALIGN(0x10);

		.text . : AT( LOADADDR(.vector) + . - ADDR(.vector) ){ *(.text .text.* .gnu.linkonce.t*) } 
		. = ALIGN(0x1000);
		_TEXT_END = .;

		_RO_START = .;
		.rodata  : AT( LOADADDR(.text) + . - ADDR(.text) ){ *(.rodata .rodata.* .gnu.linkonce.r*) } 
		. = ALIGN(0x1000);
		_RO_END = .;


		_DATA_START = .;
		PROVIDE(_data = .);
		.data . :AT( LOADADDR(.rodata) + . - ADDR(.rodata) ) { 
			*(.data .data.* .gnu.linkonce.d* .got .got.*) 
			. = ALIGN(0x1000);
		} 
		_DATA_END = .;

		_BSS_START = .; 
		.bss . :AT( LOADADDR(.data) + . - ADDR(.data) ) { 
			*(.bss .bss.*)
			. = ALIGN(0x1000);
		} 
		_BSS_END = .;

		.eh_frame_hdr . : AT( LOADADDR(.bss) + . - ADDR(.bss) ){
			*(.eh_frame .eh_frame.*)
		}
		
		.eh_frame . : AT( LOADADDR(.eh_frame_hdr) + . - ADDR(.eh_frame_hdr) ){
			*(.eh_frame .eh_frame.*)
		}
		. = ALIGN(0x1000);
		.stack . : AT( LOADADDR(.eh_frame) + . - ADDR(.eh_frame) ){
			_sstack = .;
			*(.stack);	
			_stack = .;
			_stack_base = .;
		} 
	
		_PAGE_TABLE_START = .;
		.page_tables . :AT( LOADADDR(.stack) + . - ADDR(.stack) ) { *(.page_tables) } 
		_PAGE_TABLE_END = .;

		. = ALIGN(0x1000);
		.bitmap_section . :AT( LOADADDR(.page_tables) + . - ADDR(.page_tables) ) {
			_BITMAP_NS_START = .;
			. = . + 0x4000;
			_BITMAP_NS_END = .;
			_BITMAP_START = .; 
			. = . + 0x4000;	
			_BITMAP_END = .;
			. = ALIGN(0x1000);
			_binary_end = .;
		} 
		.shared_ns_memory . : AT( LOADADDR(.bitmap_section) + . - ADDR(.bitmap_section) ){ /*:AT(KERNEL_PA_BASE) */

		. = . + 0x80000 ;
		}  =0 

		.empty_section . :AT( LOADADDR(.shared_ns_memory) + . - ADDR(.shared_ns_memory) ) {
		. = ALIGN(0x1000000);/*align 16MB*/
		_ram_end = .;
		}  = 0
}
 
