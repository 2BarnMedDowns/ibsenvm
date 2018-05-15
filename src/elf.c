#include <elf.h>
#include <string.h>
#include <errno.h>
#include <noravm_list.h>
#include <noravm_output.h>
#include <noravm_image.h>
#include <stdlib.h>
#include <stdio.h>


#define LINUX_ENTRY 0x400000



int noravm_elf_write(FILE* fp, const struct noravm_image* image, const void* bytecode)
{
    if (image->vm_entry_point != LINUX_ENTRY) {
        return EINVAL;
    }

    size_t ph_off = sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr) * image->num_segments;
    size_t data_start = NORAVM_ALIGN_ADDR(ph_off, image->page_size);
    
    size_t data_end = data_start + image->file_size;

    char strs[] = "\0.data\0.text\0.text\0.data\0.bss\0.rodata\0.shstrtab\0";

    uint32_t idxs[] = {
        1, 7, 13, 19, 25, 30, 38, 0
    };

    size_t sh_start = data_end + sizeof(strs);

    Elf64_Ehdr ehdr;
    ehdr.e_ident[EI_MAG0] = ELFMAG0;
    ehdr.e_ident[EI_MAG1] = ELFMAG1;
    ehdr.e_ident[EI_MAG2] = ELFMAG2;
    ehdr.e_ident[EI_MAG3] = ELFMAG3;
    ehdr.e_ident[EI_CLASS] = ELFCLASS64;
    ehdr.e_ident[EI_DATA] = ELFDATA2LSB;
    ehdr.e_ident[EI_VERSION] = EV_CURRENT;
    ehdr.e_ident[EI_OSABI] = ELFOSABI_LINUX; 
    ehdr.e_ident[EI_ABIVERSION] = 0;
    ehdr.e_ident[EI_PAD] = 0;
    ehdr.e_type = ET_EXEC;
    ehdr.e_machine = EM_X86_64;
    ehdr.e_version = EV_CURRENT;
    ehdr.e_entry = image->vm_entry_point + data_start;
    ehdr.e_phoff = sizeof(ehdr);
    ehdr.e_shoff = sh_start;
    ehdr.e_flags = 0;
    ehdr.e_ehsize = sizeof(ehdr);
    ehdr.e_phentsize = sizeof(Elf64_Phdr);
    ehdr.e_phnum = (ph_off - sizeof(Elf64_Ehdr)) / sizeof(Elf64_Phdr);
    ehdr.e_shentsize = sizeof(Elf64_Shdr);
    ehdr.e_shnum = 2 + image->num_sections;
    ehdr.e_shstrndx = 1 + image->num_sections;

    fwrite(&ehdr, sizeof(ehdr), 1, fp);

    // Write program headers to file
    noravm_list_foreach(const struct noravm_segment, segment, &image->segments) {
        Elf64_Phdr phdr;
        phdr.p_type = PT_LOAD;
        phdr.p_flags = 0;
        phdr.p_offset = data_start + segment->file_start;
        phdr.p_filesz = segment->file_size;
        phdr.p_vaddr = segment->vm_start;
        phdr.p_paddr = phdr.p_vaddr;
        phdr.p_memsz = segment->vm_size;
        phdr.p_align = segment->vm_align;

        switch (segment->type) {
            case NORAVM_SEG_NULL:
                break;

            case NORAVM_SEG_NORAVM:
                // Include ELF header and program headers
                phdr.p_offset = 0;
                phdr.p_filesz = data_start + segment->file_start + segment->file_size;
                phdr.p_memsz = data_start + segment->file_start + segment->file_size;
                phdr.p_flags = PF_R | PF_X;
                break;

            case NORAVM_SEG_DATA:
                phdr.p_flags = PF_R | PF_W;
                break;
        }

        fwrite(&phdr, sizeof(phdr), 1, fp);
    }

    // Write padding to file
    for (size_t i = 0; i < data_start - ph_off; ++i) {
        fputc('\x00', fp);
    }

    // Write data to file
    noravm_list_foreach(const struct noravm_segment, segment, &image->segments) {

        noravm_list_foreach(const struct noravm_section, section, &segment->sections) {
            const void* ptr = NULL;

            switch (section->type) {
                case NORAVM_SECT_BYTECODE:
                    ptr = bytecode;
                    break;

                default:
                    ptr = section->data;
                    break;
            }

            fwrite(ptr, section->size, 1, fp);

            for (size_t i = 0; i < section->file_padding; ++i) {
                fputc('\x00', fp);
            }
        }
    }

    // Write string table contents
    fwrite(strs, sizeof(strs), 1, fp);

    // Write NULL section header
    Elf64_Shdr nshdr;
    nshdr.sh_name = SHN_UNDEF;
    nshdr.sh_type = SHT_NULL;
    nshdr.sh_flags = 0;
    nshdr.sh_addr = 0;
    nshdr.sh_offset = 0;
    nshdr.sh_size = 0;
    nshdr.sh_link = 0;
    nshdr.sh_info = 0;
    nshdr.sh_addralign = 0;
    nshdr.sh_entsize = 0;
    fwrite(&nshdr, sizeof(nshdr), 1, fp);

    // Write section headers
    noravm_list_foreach(const struct noravm_segment, segment, &image->segments) {

        noravm_list_foreach(const struct noravm_section, section, &segment->sections) {
            Elf64_Shdr shdr;
            shdr.sh_name = idxs[section->type];
            shdr.sh_type = 0;
            shdr.sh_flags = 0;
            shdr.sh_offset = data_start + segment->file_start + section->file_offset_to_seg;
            shdr.sh_size = NORAVM_ALIGN_ADDR(section->size, section->file_align);
            shdr.sh_link = 0;
            shdr.sh_info = 0;
            shdr.sh_addr = segment->vm_start + section->vm_offset_to_seg;
            shdr.sh_addralign = section->vm_align;
            shdr.sh_entsize = 0;

            switch (section->type) {
                case NORAVM_SECT_DATA:
                    shdr.sh_type = SHT_PROGBITS;
                    shdr.sh_flags = SHF_WRITE | SHF_ALLOC;
                    break;

                case NORAVM_SECT_BYTECODE:
                    shdr.sh_type = SHT_PROGBITS;
                    shdr.sh_flags = SHF_WRITE | SHF_ALLOC;
                    break;

                case NORAVM_SECT_CONST:
                    shdr.sh_type = SHT_PROGBITS;
                    shdr.sh_flags = SHF_ALLOC;
                    break;

                case NORAVM_SECT_TEXT:
                case NORAVM_SECT_CODE:
                    shdr.sh_type = SHT_PROGBITS;
                    shdr.sh_addr += shdr.sh_offset;
                    shdr.sh_flags = SHF_ALLOC | SHF_EXECINSTR;
                    break;

                case NORAVM_SECT_BSS:
                    shdr.sh_type = SHT_NOBITS;
                    shdr.sh_flags = SHF_ALLOC | SHF_WRITE;
                    break;
            }

            fwrite(&shdr, sizeof(shdr), 1, fp);
        }
    }

    // Write string table
    Elf64_Shdr strtab;
    strtab.sh_name = idxs[6];
    strtab.sh_type = SHT_STRTAB;
    strtab.sh_flags = 0;
    strtab.sh_addr = 0;
    strtab.sh_offset = data_end;
    strtab.sh_size = sizeof(strs);
    strtab.sh_link = 0;
    strtab.sh_info = 0;
    strtab.sh_addralign = 1;
    strtab.sh_entsize = 0;// sizeof(strs);
    
    fwrite(&strtab, sizeof(strtab), 1, fp);


    return 0;
}
