#include <stdint.h>
/*definitions of elf header*/

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

typedef struct
{
	Elf64_Off p_offset;
	Elf64_Addr p_vaddr;
	Elf64_Xword p_filesz;
	Elf64_Xword p_memsz;
	Elf64_Xword p_align;
	Elf64_Addr p_vaddr_end;
} Segment;


#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define handle_error(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define PT_LOAD 1
#define MAX_SEGMENTS 40

#define PAGE_SIZE 0x1000U
#define PAGEUP(x) (((x) + PAGE_SIZE - 1) & (~(PAGE_SIZE - 1))) 

#define SHT_STRTAB 3
#define SHT_DYNSYM 11
#define SHT_SYMTAB 2 

uint64_t get_symbol_address(char* elf, char* name)
{
    Elf64_Ehdr *ehdr = (Elf64_Ehdr*)elf;
    Elf64_Shdr *shdr = (Elf64_Shdr*)(elf + ehdr->e_shoff);
    int shnum = ehdr->e_shnum;
    int addr = 0, num  = 0, entsize  = 0, i, s;
    int len = strlen(name);
    char *strtab = 0, *dynsym = 0;

    for (i = 0; i < shnum; ++i) {
        if (shdr[i].sh_type == SHT_DYNSYM) {
            dynsym = shdr[i].sh_offset + elf;
            num = shdr[i].sh_size / shdr[i].sh_entsize;
            entsize = shdr[i].sh_entsize;
            strtab = shdr[ shdr[i].sh_link ].sh_offset + elf;
        }   

        if (strtab != 0 && dynsym != 0) {
	    for (s = 0; s < num; s++) {
	        Elf64_Sym *sym = (Elf64_Sym*)(dynsym + entsize * s); 
	        if ( strncmp( sym->st_name + strtab, name, len ) == 0) {
	            addr = sym->st_value;
	            return addr;
	        }
	    }
	    strtab = 0;
	}
    }   


    return 0;
}


Segment PROG_SEGMENTS[ MAX_SEGMENTS ] = {0};
const char usage[] = "\
Usage: procimage-gen <input file> <output file> \n\
The program creates a process image from elf file <input file> \n\
and saves it in the output file <output_file>.\n\
if flag -b is specified, the first byte of generated image is replaced by 0. ";

int main(int argc, char *argv[])
{
	char *addr;
	int fd;
	struct stat sb;
	int b_flag = 0;
	int arg_index = 1;
	char *input_file = NULL;
	char *output_file = NULL;


	if (argc < 2 ) {
error_usage:	printf("%s",usage);
		exit(1);
	} 

	if (argc == 4 ) {/*check for flag*/
		if(strcmp("-b", argv[1]) == 0) b_flag = 1; 			
		else  {
			printf("invalid flag %s\n\n", argv[1]);
			goto error_usage;
		}
		arg_index = 2;
	}
	
	input_file = argv[arg_index];
	output_file = argv[arg_index+1];
	
	if (strncmp(input_file,"-",1) == 0) {
		printf("invalid argument\n");
		goto error_usage;
	}
	
	fd = open(input_file, O_RDONLY);
	if (fd == -1)
		handle_error("open");

	if (fstat(fd, &sb) == -1)           /* To obtain file size */
		handle_error("fstat");


	addr = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (addr == MAP_FAILED)
		handle_error("mmap");
	close(fd);
	uint64_t test_elf_ptr = 0;
	test_elf_ptr = get_symbol_address((char*) addr,(char*)"user_binary");


	Elf64_Ehdr *ehdr = (Elf64_Ehdr*) addr;

	Elf64_Phdr *phdr = (Elf64_Phdr*) ( (uint64_t)addr + ehdr->e_phoff);

	uint64_t lowest_virt_addr = (uint64_t)-1;
	Elf64_Half phdr_count = 0;

	for ( Elf64_Half i=0; i < ehdr->e_phnum; i++ ) {
		if ( phdr[i].p_type == PT_LOAD) {

			PROG_SEGMENTS[phdr_count].p_offset = phdr[i].p_offset;
			PROG_SEGMENTS[phdr_count].p_vaddr  = phdr[i].p_vaddr;
			PROG_SEGMENTS[phdr_count].p_filesz = phdr[i].p_filesz;
			PROG_SEGMENTS[phdr_count].p_memsz  = phdr[i].p_memsz;
			PROG_SEGMENTS[phdr_count].p_align  = phdr[i].p_align;
			if ( phdr[i].p_vaddr < lowest_virt_addr ) {
				lowest_virt_addr = phdr[i].p_vaddr;
			}
			PROG_SEGMENTS[phdr_count].p_vaddr_end = 
				PAGEUP( phdr[i].p_vaddr +  phdr[i].p_memsz );
			phdr_count++;
		}
	}

	uint64_t map_size = 0;
	uint64_t highest_vaddr = 0;
	for ( Elf64_Half i=0; i <= phdr_count; i++) {
		if (PROG_SEGMENTS[i].p_vaddr > highest_vaddr) {
			highest_vaddr = PROG_SEGMENTS[i].p_vaddr_end;
		}	
	}

	map_size = highest_vaddr - lowest_virt_addr;

	int output_filefd = open(output_file, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRGRP | S_ISUID);
	if (output_filefd < 0 ) {
		perror("failed to open output file\n");
		goto exit_error;
	}

	char zero_page[PAGE_SIZE] = {0};
	for (uint64_t i=0; i < map_size/PAGE_SIZE; i++ ) {
		write(output_filefd, zero_page, PAGE_SIZE);
	}

	if( lseek(output_filefd, 0 , SEEK_SET) != 0 ) {
		perror("cannot reset the file position\n");
		exit(1);
	}
	void *new_map = mmap(NULL, map_size, PROT_READ|PROT_WRITE, MAP_SHARED, output_filefd, 0);

	if (new_map == MAP_FAILED ) {
		perror("failed to mmap");
		close(output_filefd);
		goto exit_error;
	}
	/*assume for now that all segments appear linearly */	
	void *src, *dst;
	size_t to_write = 0;

	for (Elf64_Half i=0; i < (phdr_count-1); i++) {
		src = (void*) (PROG_SEGMENTS[i].p_offset + (uint64_t)addr);
		dst = (void*) ((uint64_t)PROG_SEGMENTS[i].p_vaddr - lowest_virt_addr  + (uint64_t)new_map);
		to_write = (uint64_t)PROG_SEGMENTS[i+1].p_vaddr - (uint64_t)PROG_SEGMENTS[i].p_vaddr;

		if ((PROG_SEGMENTS[i].p_offset + PROG_SEGMENTS[i].p_filesz) < PROG_SEGMENTS[i+1].p_memsz) 
			to_write = PAGEUP(to_write);

		printf("writing to segment %d address 0x%lx size 0x%lx\n", i, PROG_SEGMENTS[i].p_vaddr, to_write);
		memcpy(dst, src, to_write);
	}
	
	src = (void*) (PROG_SEGMENTS[phdr_count - 1].p_offset + (uint64_t)addr);
	dst = (void*) ((uint64_t)PROG_SEGMENTS[phdr_count - 1].p_vaddr - lowest_virt_addr  + (uint64_t)new_map);
	to_write = ((sb.st_size - PROG_SEGMENTS[phdr_count - 1].p_offset) < PROG_SEGMENTS[phdr_count - 1].p_memsz) ?
		   (sb.st_size - PROG_SEGMENTS[phdr_count - 1].p_offset) : PROG_SEGMENTS[phdr_count - 1].p_memsz; 
	printf("writing to segment %d address 0x%lx size 0x%lx\n", phdr_count-1, PROG_SEGMENTS[phdr_count-1].p_vaddr, to_write);
	memcpy(dst, src, to_write);
		
	/*include sections in the image*/
	/*copy user data into the image*/
	int test_fd = open("../user/test.elf", O_RDONLY);
	if (test_fd) {
		uint64_t test_count = lseek(test_fd, 0, SEEK_END);
		lseek(test_fd, 0, SEEK_SET);
		if (test_count > 8192) {
			printf("WARNING, THE SIZE of test.elf is greater than 8K\n");
			test_count = 8192;		
		}
		if (test_elf_ptr) {
			char user_buffer[8192];
			if( read(test_fd, user_buffer, test_count) < test_count) {
				printf("WARNING, could not read all data\n");
			}
			memcpy((void*)(test_elf_ptr + new_map), user_buffer, test_count);
		}
		close(test_fd);
	}
	/*end copy user data*/	

	if (b_flag == 1) 
		*(uint8_t*)new_map = 0;
	
	fsync(output_filefd);
	munmap(new_map, map_size);
	close(output_filefd);	
exit_error:
	munmap(addr, sb.st_size);
	exit(EXIT_SUCCESS);
}
