#include <elf.h>
#include <string.h>
#include <noravm_list.h>
#include <noravm_output.h>
#include <noravm_image.h>
#include <stdlib.h>
#include <stdio.h>

#define ELF_LOL_WTF_HACK    0x78



int noravm_elf_write(FILE* fp, const struct noravm_image* image, const void* bytecode)
{
    size_t data_start = sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr) * (image->num_sections);
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
    ehdr.e_ident[EI_OSABI] = ELFOSABI_LINUX; // Maybe standalone ?
    ehdr.e_ident[EI_ABIVERSION] = 0;
    ehdr.e_ident[EI_PAD] = 0;
    ehdr.e_type = ET_EXEC;
    ehdr.e_machine = EM_X86_64;
    ehdr.e_version = EV_CURRENT;
    ehdr.e_entry = image->vm_entry_point + ELF_LOL_WTF_HACK;
    ehdr.e_phoff = sizeof(ehdr);
    ehdr.e_shoff = sh_start;
    ehdr.e_flags = 0;
    ehdr.e_ehsize = sizeof(ehdr);
    ehdr.e_phentsize = sizeof(Elf64_Phdr);
    ehdr.e_phnum = (data_start - sizeof(Elf64_Ehdr)) / sizeof(Elf64_Phdr);
    ehdr.e_shentsize = sizeof(Elf64_Shdr);
    ehdr.e_shnum = 2 + image->num_sections;
    ehdr.e_shstrndx = 1 + image->num_sections;

    fwrite(&ehdr, sizeof(ehdr), 1, fp);

    // Write program headers to file
    size_t curr_segment = 0;
    noravm_list_foreach(const struct noravm_segment, segment, &image->segments) {
        noravm_list_foreach(const struct noravm_section, section, &segment->sections) {
            Elf64_Phdr phdr;
            phdr.p_type = PT_LOAD;
            phdr.p_flags = 0;

            switch (section->type) {
                case NORAVM_SECT_DATA:
                    phdr.p_flags = PF_R | PF_W;
                    break;

                case NORAVM_SECT_TEXT:
                    phdr.p_flags = PF_R;
                    break;

                case NORAVM_SECT_CODE:
                    phdr.p_flags = PF_X | PF_R;
                    break;

                case NORAVM_SECT_BYTECODE:
                    phdr.p_flags = PF_R | PF_W;
                    break;

                case NORAVM_SECT_BSS:
                    phdr.p_flags = PF_R | PF_W;
                    break;

                case NORAVM_SECT_CONST:
                    phdr.p_flags = PF_R;
                    break;
            }

            phdr.p_offset = data_start + section->file_start;
            phdr.p_vaddr = segment->vm_start + section->vm_offset_to_seg;
            
            if (section->type == NORAVM_SECT_CODE) {
                phdr.p_vaddr += ELF_LOL_WTF_HACK;
            }

            phdr.p_paddr = phdr.p_vaddr;
            phdr.p_filesz = section->size + section->file_padding;
            phdr.p_memsz = section->vm_size;
            phdr.p_align = 4ULL << 20;
            ++curr_segment;

            fwrite(&phdr, sizeof(phdr), 1, fp);
        }
    }

    // Write data to file
    noravm_list_foreach(const struct noravm_segment, segment, &image->segments) {

        noravm_list_foreach(const struct noravm_section, section, &segment->sections) {
            const void* ptr = section->data;

            if (ptr == NULL && section->type == NORAVM_SECT_BYTECODE) {
                ptr = bytecode;
            }

            if (ptr != NULL) {
                fwrite(ptr, section->size, 1, fp);
            }
            
            for (size_t i = 0; i < section->file_padding; ++i) {
                fputc('\x00', fp);
            }
        }
    }

    // Write string table
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
                case NORAVM_SECT_TEXT:
                    shdr.sh_type = SHT_PROGBITS;
                    shdr.sh_flags = SHF_ALLOC;
                    break;

                case NORAVM_SECT_CODE:
                    shdr.sh_type = SHT_PROGBITS;
                    shdr.sh_flags = SHF_ALLOC | SHF_EXECINSTR;
                    break;

                case NORAVM_SECT_BSS:
                    shdr.sh_type = SHT_NOBITS;
                    shdr.sh_flags = SHF_ALLOC | SHF_WRITE;
                    break;
            }

            shdr.sh_offset = data_start + section->file_start;
            shdr.sh_size = section->size + section->file_padding;
            shdr.sh_link = 0;
            shdr.sh_info = 0;
            shdr.sh_addr = segment->vm_start + section->vm_offset_to_seg;
            shdr.sh_addralign = 1;
            shdr.sh_entsize = 0;//sizeof(shdr);

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
