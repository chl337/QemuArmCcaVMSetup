#ifndef ELF_H
#define ELF_H

#include <stdint.h>

#define EI_NIDENT	16

#define ET_EXEC 2
#define ET_DYN 3

/* p_type */
#define PT_LOAD 1

/* p_flags */
#define PF_R		0x4
#define PF_W		0x2
#define PF_X		0x1

#define ELF64HDR_IDENT_OFFSET           0
#define ELF64HDR_IDENT_LENGTH           16
#define ELF64HDR_TYPE_OFFSET            16
#define ELF64HDR_TYPE_LENGTH            2       
#define ELF64HDR_MACHINE_OFFSET         18
#define ELF64HDR_MACHINE_LENGTH         2
#define ELF64HDR_VERSION_OFFSET         20
#define ELF64HDR_VERSION_LENGTH         4
#define ELF64HDR_ENTRY_OFFSET           24
#define ELF64HDR_ENTRY_LENGTH           8
#define ELF64HDR_PHOFF_OFFSET           32
#define ELF64HDR_PHOFF_LENGTH           8
#define ELF64HDR_SHOFF_OFFSET           40
#define ELF64HDR_SHOFF_LENGTH           8
#define ELF64HDR_FLAGS_OFFSET           48
#define ELF64HDR_FLAGS_LENGTH           4
#define ELF64HDR_EHSIZE_OFFSET          52
#define ELF64HDR_EHSIZE_LENGTH          2
#define ELF64HDR_PHENTSIZE_OFFSET       54
#define ELF64HDR_PHENTSIZE_LENGTH       2
#define ELF64HDR_PHNUM_OFFSET           56
#define ELF64HDR_PHNUM_LENGTH           2
#define ELF64HDR_SHENTSIZE_OFFSET       58
#define ELF64HDR_SHENTSIZE_LENGTH       2
#define ELF64HDR_SHNUM_OFFSET           60
#define ELF64HDR_SHNUM_LENGTH           2
#define ELF64HDR_SHSTRNDX_OFFSET        62
#define ELF64HDR_SHSTRNDX_LENGTH        2

/*definitions of elf section*/

#define ELF64_SH_NAME_OFFSET            0       
#define ELF64_SH_NAME_LENGTH            4
#define ELF64_SH_TYPE_OFFSET            (ELF64_SH_NAME_OFFSET + ELF64_SH_NAME_LENGTH)
#define ELF64_SH_TYPE_LENGTH            4       
#define ELF64_SH_FLAGS_OFFSET           (ELF64_SH_TYPE_OFFSET + ELF64_SH_TYPE_LENGTH)
#define ELF64_SH_FLAGS_LENGTH           8       
#define ELF64_SH_ADDR_OFFSET            (ELF64_SH_FLAGS_OFFSET + ELF64_SH_FLAGS_LENGTH)
#define ELF64_SH_ADDR_LENGTH            8
#define ELF64_SH_OFFSET_OFFSET          (ELF64_SH_ADDR_OFFSET + ELF64_SH_ADDR_LENGTH)
#define ELF64_SH_OFFSET_LENGTH          8
#define ELF64_SH_SIZE_OFFSET            (ELF64_SH_OFFSET_OFFSET + ELF64_SH_OFFSET_LENGTH)
#define ELF64_SH_SIZE_LENGTH            8
#define ELF64_SH_LINK_OFFSET            (ELF64_SH_SIZE_OFFSET + ELF64_SH_SIZE_LENGTH)
#define ELF64_SH_LINK_LENGTH            4
#define ELF64_SH_INFO_OFFSET            (ELF64_SH_LINK_OFFSET + ELF64_SH_LINK_LENGTH)
#define ELF64_SH_INFO_LENGTH            4
#define ELF64_SH_ADDRALIGN_OFFSET       (ELF64_SH_INFO_OFFSET + ELF64_SH_INFO_LENGTH)
#define ELF64_SH_ADDRALIGN_LENGTH       4
#define ELF64_SH_ENTSIZE_OFFSET         (ELF64_SH_ADDRALIGN_OFFSET + ELF64_SH_ADDRALIGN_LENGTH)
#define ELF64_SH_ENTSIZE_LENGTH         4

/*definitions of elf symbol*/

#define ELF64_ST_NAME_OFFSET    0
#define ELF64_ST_NAME_LENGTH    4
#define ELF64_ST_INFO_OFFSET    (ELF64_ST_NAME_OFFSET + ELF64_ST_NAME_LENGTH)
#define ELF64_ST_INFO_LENGTH    1
#define ELF64_ST_OTHER_OFFSET   (ELF64_ST_INFO_OFFSET + ELF64_ST_INFO_LENGTH)
#define ELF64_ST_OTHER_LENGTH   1
#define ELF64_ST_SHNDX_OFFSET   (ELF64_ST_OTHER_OFFSET + ELF64_ST_OTHER_LENGTH)
#define ELF64_ST_SHNDX_LENGTH   2
#define ELF64_ST_VALUE_OFFSET   (ELF64_ST_SHNDX_OFFSET + ELF64_ST_SHNDX_LENGTH)
#define ELF64_ST_VALUE_LENGTH   8
#define ELF64_ST_SIZE_OFFSET    (ELF64_ST_VALUE_OFFSET + ELF64_ST_VALUE_LENGTH)
#define ELF64_ST_SIZE_LENGTH    8

/*definitions of elf relocation rel*/
#define ELF64_REL_R_OFFSET_OFFSET       0
#define ELF64_REL_R_OFFSET_LENGTH       8
#define ELF64_REL_R_INFO_OFFSET         8
#define ELF64_REL_R_INFO_LENGTH         8

/*definitions of elf relocation rela*/
#define ELF64_RELA_R_OFFSET_OFFSET      0
#define ELF64_RELA_R_OFFSET_LENGTH      8
#define ELF64_RELA_R_INFO_OFFSET        8
#define ELF64_RELA_R_INFO_LENGTH        8
#define ELF64_RELA_R_ADDEND_OFFSET      16
#define ELF64_RELA_R_ADDEND_LENGTH      8

/*section types*/

#define ELF64_SHT_RELA  4
#define ELF64_SHT_REL   9


/*relocation types*/
#define        RAARCH64_ABS64		 257
#define        RAARCH64_COPY             1024
#define        RAARCH64_GLOB_DATA        1025
#define        RAARCH64_JUMP_SLOT        1026
#define        RAARCH64_RELATIVE         1027
#define        RAARCH64_TLS_DTP_REL64    1028
#define        RAARCH64_TLS_DTP_MOD64    1029
#define        RAARCH64_TLS_TP_REL64     1030
#define        RAARCH64_TLS_DESC         1031
#define        RAARCH64_IRELATIVE        1032

/*define registers to use */
#define RELOC_ELF_ADDR_REG		x10
#define RELOC_SEC_ADDR_REG		x11
#define RELOC_SEC_SIZE_REG		x12
#define RELOC_SEC_ENUM_REG		x13
#define RELOC_RELA_ADDR_REG		x14
#define RELOC_RELA_ENTS_REG		x15
#define RELOC_RELA_ENTC_REG		x16
#define RELOC_SYMBL_ADDR_REG		x17
#define RELOC_SYMBL_SIZE_REG		x18
#define RELOC_SYMBL_COUNT_REG		x19

typedef uint64_t Elf64_Addr;	// 8
typedef uint64_t Elf64_Off;	// 8
typedef uint16_t Elf64_Half;	// 2
typedef uint32_t Elf64_Word;	// 4
typedef uint32_t Elf64_Sword;	// 4
typedef uint64_t Elf64_Xword;	// 8
typedef uint64_t Elf64_Sxword;	// 8
typedef int16_t  Elf64_SHalf;

typedef struct elf64_phdr {
  Elf64_Word p_type;
  Elf64_Word p_flags;
  Elf64_Off p_offset;
  Elf64_Addr p_vaddr;
  Elf64_Addr p_paddr;
  Elf64_Xword p_filesz;
  Elf64_Xword p_memsz;
  Elf64_Xword p_align;
} Elf64_Phdr;

typedef struct
{
	uint8_t	  e_ident[16];
	Elf64_Half e_type;
	Elf64_Half e_machine;
	Elf64_Word e_version;
	Elf64_Addr e_entry;
	Elf64_Off e_phoff;
	Elf64_Off e_shoff;
	Elf64_Word e_flags;
	Elf64_Half e_ehsize;
	Elf64_Half e_phentsize;
	Elf64_Half e_phnum;
	Elf64_Half e_shentsize;
	Elf64_Half e_shnum;
	Elf64_Half e_shstrndx;
} Elf64_Ehdr;

typedef struct
{
	Elf64_Word sh_name;
	Elf64_Word sh_type;
	Elf64_Xword sh_flags;
	Elf64_Addr sh_addr;
	Elf64_Off sh_offset;
	Elf64_Xword sh_size;
	Elf64_Word sh_link;
	Elf64_Word sh_info;
	Elf64_Xword sh_addralign;
	Elf64_Xword sh_entsize;
} Elf64_Shdr;

typedef struct
{
	Elf64_Word st_name;
	unsigned char st_info;
	unsigned char st_other;
	Elf64_Half st_shndx;
	Elf64_Addr st_value;
	Elf64_Xword st_size;
} Elf64_Sym;

#define ELF64_R_SYM(i) ((i) >> 32)
#define ELF64_R_TYPE(i) ((i) & 0xffffffffL)
#define ELF64_R_INFO(s, t) (((s) << 32) + ((t) & 0xffffffffL))
typedef struct
{
	Elf64_Addr r_offset;
	Elf64_Xword r_info;
} Elf64_Rel;

typedef struct
{
	Elf64_Addr r_offset;
	Elf64_Xword r_info;
	Elf64_Sxword r_addend;
} Elf64_Rela;



#endif
