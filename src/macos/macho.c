#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <ivm_list.h>
#include <ivm_image.h>
#include <mach-o/loader.h>


struct dy_cmds
{
    uint32_t        ncmds;
    uint32_t        sizeofcmds;
    unsigned char   cmds[];
};



static size_t count_sections(const struct ivm_segment* segment)
{
    size_t num_sections = 0;

    if (segment != NULL) {
        ivm_list_foreach(const struct ivm_section, section, &segment->sections) {
            ++num_sections;
        }
    }

    return num_sections;
}



static struct segment_command_64* create_segment(const struct ivm_segment* segment, size_t file_start, uint64_t* entryoff)
{
    size_t num_sects = count_sections(segment);
    size_t size = sizeof(struct segment_command_64) + sizeof(struct section_64) * num_sects;

    struct segment_command_64* cmd = malloc(size);
    if (cmd == NULL) {
        return NULL;
    }

    memset(cmd, 0, size);
    cmd->cmd = LC_SEGMENT_64;
    cmd->cmdsize = size;
    cmd->maxprot = VM_PROT_NONE;
    cmd->initprot = VM_PROT_NONE;

    if (segment == NULL) {
        // No reason to continue
        return cmd;
    }

    cmd->vmaddr = segment->vm_start;
    cmd->vmsize = segment->vm_size;
    cmd->fileoff = file_start + segment->file_start;
    cmd->filesize = segment->file_size;
    cmd->nsects = num_sects;
    cmd->flags = 0;

    switch (segment->type) {
        case IVM_SEG_CODE:
            strcpy(cmd->segname, SEG_TEXT);
            cmd->fileoff = 0;
            cmd->filesize = file_start + segment->file_start + segment->file_size;
            cmd->vmsize = file_start + segment->file_start + segment->file_size;
            cmd->maxprot = VM_PROT_ALL;
            cmd->initprot = VM_PROT_READ | VM_PROT_EXECUTE;
            break;

        case IVM_SEG_DATA:
            strcpy(cmd->segname, SEG_DATA);
            cmd->maxprot = VM_PROT_ALL;
            cmd->initprot = VM_PROT_READ | VM_PROT_WRITE;
            break;
    }

    struct section_64* sects = (void*) (((unsigned char*) cmd) + sizeof(struct segment_command_64));
    size_t curr_section = 0;
    ivm_list_foreach(const struct ivm_section, section, &segment->sections) {
        struct section_64* lsect = &sects[curr_section++];

        strcpy(lsect->segname, cmd->segname);
        lsect->addr = segment->vm_start + section->vm_offset_to_seg;
        lsect->size = section->size + section->file_padding;
        lsect->align = 12;
        lsect->offset = file_start + section->file_offset_to_seg + segment->file_start;
        lsect->flags = S_REGULAR;

        switch (section->type) {
            case IVM_SECT_CONST:
            case IVM_SECT_BYTECODE:
            case IVM_SECT_DATA:
                strcpy(lsect->sectname, SECT_DATA);
                break;

            case IVM_SECT_BSS:
                strcpy(lsect->sectname, SECT_BSS);
                lsect->flags |= S_ZEROFILL;
                break;

            case IVM_SECT_TEXT:
            case IVM_SECT_CODE:
                strcpy(lsect->sectname, SECT_TEXT);
                lsect->flags = S_ATTR_PURE_INSTRUCTIONS | S_ATTR_SOME_INSTRUCTIONS;
                lsect->addr += lsect->offset;
                *entryoff = lsect->offset;
                break;
        }
    }

    return cmd;
}


static void* command_offset(struct dy_cmds* dycmds, uint32_t cmd, uint32_t cmdsize)
{
    struct load_command* ptr = (void*) (dycmds->cmds + dycmds->sizeofcmds);
    dycmds->ncmds++;
    dycmds->sizeofcmds += cmdsize;

    memset(ptr, 0, cmdsize);
    ptr->cmd = cmd;
    ptr->cmdsize = cmdsize;
    
    return ptr;
}



static struct dy_cmds* create_dynamic_stuff()
{
    size_t size = 0;
    size += sizeof(struct dyld_info_command);
    size += sizeof(struct dysymtab_command);
    size += sizeof(struct dylinker_command) + 20;
    size += sizeof(struct dylib_command) + 32;
    size += sizeof(struct symtab_command);

    struct dy_cmds* ptr = malloc(sizeof(struct dy_cmds) + size);
    if (ptr == NULL) {
        return NULL;
    }
    ptr->ncmds = 0;
    ptr->sizeofcmds = 0;

    command_offset(ptr, LC_DYLD_INFO_ONLY, sizeof(struct dyld_info_command));
    command_offset(ptr, LC_DYSYMTAB, sizeof(struct dysymtab_command));
    command_offset(ptr, LC_SYMTAB, sizeof(struct symtab_command));
    struct dylinker_command* dylinker = command_offset(ptr, LC_LOAD_DYLINKER, sizeof(struct dylinker_command) + 20);
    struct dylib_command* dylib = command_offset(ptr, LC_LOAD_DYLIB, sizeof(struct dylib_command) + 32);

    // Specify dynamic linker
    dylinker->name.offset = sizeof(*dylinker);
    strcpy(((char*) dylinker) + sizeof(*dylinker), "/usr/lib/dyld");

    // Specify dynamic library
    dylib->dylib.name.offset = sizeof(*dylib);
    dylib->dylib.timestamp = 2; // FIXME: WTF?
    dylib->dylib.current_version = 0x4ca0a01; // FIXME: This is a hack
    dylib->dylib.compatibility_version = 0x10000; // FIXME: This is a hack
    strcpy(((char*) dylib) + sizeof(*dylib), "/usr/lib/libSystem.B.dylib");

    return ptr;
}


static size_t write_load_command(FILE* fp, const void* cmd)
{
    return fwrite(cmd, ((const struct load_command*) cmd)->cmdsize, 1, fp);
}


int ivm_image_write(FILE* fp, const struct ivm_image* image, const void* bytecode)
{
    struct mach_header_64 mhdr;
    struct dy_cmds* ds = NULL;
    size_t curr_segment;
    struct segment_command_64* segments[image->num_segments + 2];
    struct entry_point_command entry_point;
    int err = 0;

    memset(segments, 0, sizeof(struct segment_command_64*) * (image->num_segments + 2));

    ds = create_dynamic_stuff();
    if (ds == NULL) {
        err = errno;
        goto leave;
    }

    entry_point.cmd = LC_MAIN;
    entry_point.cmdsize = sizeof(entry_point);
    entry_point.entryoff = 0;
    entry_point.stacksize = 0;

    // Create Mach-O header
    mhdr.magic = MH_MAGIC_64;
    mhdr.cputype = CPU_TYPE_X86_64;
    mhdr.cpusubtype = CPU_SUBTYPE_X86_64_ALL | CPU_SUBTYPE_I386_ALL;
    mhdr.filetype = MH_EXECUTE;
    mhdr.ncmds = ds->ncmds + 3 + image->num_segments;
    mhdr.sizeofcmds = ds->sizeofcmds 
        + sizeof(struct entry_point_command) 
        + sizeof(struct section_64) * image->num_sections 
        + sizeof(struct segment_command_64) * (image->num_segments + 2);
    mhdr.flags = MH_NOUNDEFS;
    mhdr.reserved = 0;

    size_t data_start = IVM_ALIGN_ADDR(sizeof(mhdr) + mhdr.sizeofcmds, image->page_size);

    // Create empty pagezero segment
    segments[0] = create_segment(NULL, data_start, NULL);
    if (segments[0] == NULL) {
        err = errno;
        goto leave;
    }
    strcpy(segments[0]->segname, SEG_PAGEZERO);
    segments[0]->vmaddr = 0;
    segments[0]->vmsize = image->vm_entry_point;
    segments[0]->fileoff = data_start;

    // Create segment load commands
    curr_segment = 1;
    ivm_list_foreach(const struct ivm_segment, segment, &image->segments) {
        segments[curr_segment] = create_segment(segment, data_start, &entry_point.entryoff);
        if (segments[curr_segment] == NULL) {
            err = errno;
            goto leave;
        }

        curr_segment++;
    }

    // Create empty linkedit segment
    segments[curr_segment] = create_segment(NULL, data_start, NULL);
    if (segments[curr_segment] == NULL) {
        err = errno;
        goto leave;
    }
    strcpy(segments[curr_segment]->segname, SEG_LINKEDIT);
    segments[curr_segment]->fileoff = data_start + image->file_size;

    // Write header and load commands to file
    fwrite(&mhdr, sizeof(mhdr), 1, fp);
    for (curr_segment = 0; curr_segment < image->num_segments + 2; ++curr_segment) {
        write_load_command(fp, segments[curr_segment]);
    }
    fwrite(ds->cmds, ds->sizeofcmds, 1, fp);
    write_load_command(fp, &entry_point);

    // Write padding to file
    for (size_t i = 0; i < data_start - (sizeof(mhdr) + mhdr.sizeofcmds); ++i) {
        fputc('\x00', fp);
    }

    // Write data to file
    ivm_list_foreach(const struct ivm_segment, segment, &image->segments) {
        ivm_list_foreach(const struct ivm_section, section, &segment->sections) {
            const void* ptr = NULL;

            switch (section->type) {
                case IVM_SECT_BYTECODE:
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

leave:
    for (curr_segment = 0; curr_segment < image->num_segments + 2; ++curr_segment) {
        free(segments[curr_segment]);
    }
    free(ds);
    return err;
}

