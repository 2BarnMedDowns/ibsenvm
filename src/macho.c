#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ibsen/list.h>
#include <ibsen/macho.h>
#include <ibsen/image.h>
#include <mach-o/loader.h>


static size_t write_load_command(FILE* fp, void* cmd)
{
    return fwrite(cmd, ((struct load_command*) cmd)->cmdsize, 1, fp);
}



static struct segment_command_64* create_segment(struct mach_header_64* mh, bool has_section)
{
    size_t size = sizeof(struct segment_command_64) + sizeof(struct section_64) * !!has_section;
    struct segment_command_64* segment = malloc(size);
    if (segment == NULL) {
        return NULL;
    }

    segment->cmd = LC_SEGMENT_64;
    segment->cmdsize = size;

    segment->vmaddr = 0;
    segment->vmsize = 0;
    segment->fileoff = 0;
    segment->filesize = 0;
    segment->maxprot = VM_PROT_NONE;
    segment->initprot = VM_PROT_NONE;
    segment->nsects = !!has_section;
    segment->flags = 0;

    struct section_64* section = (struct section_64*) (((unsigned char*) segment) + sizeof(struct segment_command_64));
    section->addr = 0;
    section->size = 0;
    section->align = 0;
    section->reloff = 0;
    section->nreloc = 0;
    section->flags = 0;
    section->reserved1 = 0;
    section->reserved2 = 0;
    section->reserved3 = 0;

    mh->ncmds += 1;
    mh->sizeofcmds += size;

    return segment;
}


static int set_segment(struct segment_command_64* lc_seg, struct ibsen_segment* segment, size_t header_offset)
{
    switch (segment->type) {
        case SEGMENT_NULL:
            strcpy(lc_seg->segname, SEG_PAGEZERO);
            break;

        case SEGMENT_CODE:
        case SEGMENT_TEXT:
            strcpy(lc_seg->segname, SEG_TEXT);
            lc_seg->maxprot = VM_PROT_ALL; // I have no idea what I am doing
            lc_seg->initprot = VM_PROT_READ | VM_PROT_EXECUTE;
            break;

        case SEGMENT_DATA:
            strcpy(lc_seg->segname, SEG_DATA);
            lc_seg->maxprot = VM_PROT_READ | VM_PROT_WRITE;
            lc_seg->initprot = VM_PROT_READ | VM_PROT_WRITE;
            break;

        default:
            return -1;
    }

    lc_seg->vmaddr = segment->vm_addr;
    lc_seg->vmsize = segment->vm_size;
    lc_seg->fileoff = header_offset + segment->file_start;
    lc_seg->filesize = segment->file_size;

    if (lc_seg->nsects > 0) {
        struct section_64* lc_sect = (struct section_64*) (((char*) lc_seg) + sizeof(struct segment_command_64));

        strcpy(lc_sect->segname, lc_seg->segname);
        lc_sect->addr = lc_seg->vmaddr;
        lc_sect->size = lc_seg->filesize;
        lc_sect->align = 4;

        switch (segment->type) {
            case SEGMENT_CODE:
            case SEGMENT_TEXT:
                strcpy(lc_sect->sectname, SECT_TEXT);
                lc_sect->flags = S_ATTR_PURE_INSTRUCTIONS | S_ATTR_SOME_INSTRUCTIONS;
                break;

            default:
                // S_ZEROFILL ?
                strcpy(lc_sect->sectname, SECT_DATA);
                lc_sect->flags = S_REGULAR;
                break;
        }
    }

    return 0;
}


static size_t create_dynamic_stuff(struct mach_header_64* mh, void** handle)
{
    size_t size = 0;
    size += sizeof(struct dyld_info_command); 
    size += sizeof(struct dysymtab_command);
    size += sizeof(struct dylinker_command) + 20;
    size += sizeof(struct dylib_command) + 32;
    size += sizeof(struct symtab_command);

    unsigned char* ptr = malloc(size);
    if (ptr == NULL) {
        *handle = NULL;
        return 0;
    }
    memset(ptr, 0, size);

    size_t offset = 0;
    size_t ncmds = 0;

    struct dyld_info_command* ldinfo = (struct dyld_info_command*) (ptr + offset);
    offset += sizeof(*ldinfo);
    ++ncmds;

    struct dysymtab_command* symtab = (struct dysymtab_command*) (ptr + offset);
    offset += sizeof(*symtab);
    ++ncmds;

    struct dylinker_command* ld = (struct dylinker_command*) (ptr + offset);
    offset += sizeof(*ld) + 20;
    ++ncmds;

    struct dylib_command* lib = (struct dylib_command*) (ptr + offset);
    offset += sizeof(*lib) + 32;
    ++ncmds;

    struct symtab_command* st = (struct symtab_command*) (ptr + offset);
    offset += sizeof(*st);
    ++ncmds;

    st->cmd = LC_SYMTAB;
    st->cmdsize = sizeof(*st);

    ldinfo->cmd = LC_DYLD_INFO_ONLY;
    ldinfo->cmdsize = sizeof(*ldinfo);

    symtab->cmd = LC_DYSYMTAB;
    symtab->cmdsize = sizeof(*symtab);

    ld->cmd = LC_LOAD_DYLINKER;
    ld->cmdsize = sizeof(*ld) + 20;
    ld->name.offset = 12; // FIXME is this sizeof(ld)?
    strcpy(((char*) ld) + sizeof(*ld), "/usr/lib/dyld/");

    lib->cmd = LC_LOAD_DYLIB;
    lib->cmdsize = sizeof(*lib) + 32;
    lib->dylib.name.offset = 24; // FIXME is this sizeof(lib)
    lib->dylib.timestamp = 2; // FIXME: WTF?
    lib->dylib.current_version = 0x4ca0a01; // FIXME: This is a hack
    lib->dylib.compatibility_version = 0x10000; // FIXME: This is a hack
    strcpy(((char*) lib) + sizeof(*lib), "/usr/lib/libSystem.B.dylib");

    mh->ncmds += ncmds;
    mh->sizeofcmds += offset;

    *handle = ptr;
    return size;
}


int ibsen_write_macho(FILE* fp, struct ibsen_image* image, const void* bytecode)
{
    struct mach_header_64 header;
    header.magic = MH_MAGIC_64;
    header.cputype = CPU_TYPE_X86_64;
    header.cpusubtype = CPU_SUBTYPE_LIB64 | CPU_SUBTYPE_I386_ALL;
    header.filetype = MH_EXECUTE;
    header.ncmds = 0;
    header.sizeofcmds = 0;
    header.flags = MH_NOUNDEFS;
    header.reserved = 0;

    // Create dynamic linker stuff
    void* dy_cmds = NULL;
    size_t dy_size = create_dynamic_stuff(&header, &dy_cmds);
    if (dy_size == 0) {
        return ENOMEM;
    }

    // Create segments 
    size_t num_segments = ibsen_list_count(&image->segments) + 1; // include linkedit segment
    struct segment_command_64* segment_cmds[num_segments];

    size_t curr_segment = 0;
    ibsen_list_for_each(seg_it, &image->segments) {
        struct ibsen_segment* segment = container_of(seg_it, struct ibsen_segment, list);
        size_t num_sects = ibsen_list_count(&segment->sections);
        segment_cmds[curr_segment] = NULL;

        if (segment->type != SEGMENT_CODE) {
        segment_cmds[curr_segment] = create_segment(&header, num_sects > 0);
        if (segment_cmds[curr_segment] == NULL) {
            for (size_t i = 0; i < curr_segment; ++i) {
                free(segment_cmds[i]);
            }
            free(dy_cmds);
            return ENOMEM;
        }
        }

        ++curr_segment;
    }

    // Create empty linkedit segment
    segment_cmds[curr_segment] = create_segment(&header, false);
    if (segment_cmds[curr_segment] == NULL) {
        for (size_t i = 0; i < curr_segment; ++i) {
            free(segment_cmds[i]);
        }
        free(dy_cmds);
        return ENOMEM;
    }
    strcpy(segment_cmds[curr_segment]->segname, SEG_LINKEDIT);

    // Create entry point
    struct entry_point_command ep;
    ep.cmd = LC_MAIN;
    ep.cmdsize = sizeof(ep);
    ep.stacksize = 0;
    header.ncmds++;
    header.sizeofcmds += ep.cmdsize;
    
    // Set correct offset into file
    //segment_cmds[num_segments - 1]->fileoff = sizeof(header) + header.sizeofcmds + image->file_size;
    ep.entryoff = sizeof(header) + header.sizeofcmds + image->ep_file_offset;

    // Write header to file
    fwrite(&header, sizeof(header), 1, fp);

    // Write all segments to file
    curr_segment = 0;
    ibsen_list_for_each(seg_it, &image->segments) {
        struct ibsen_segment* segment = container_of(seg_it, struct ibsen_segment, list);

        if (segment_cmds[curr_segment] != NULL) {
            set_segment(segment_cmds[curr_segment], segment, sizeof(header) + header.sizeofcmds);
            write_load_command(fp, segment_cmds[curr_segment]);
        }

        ++curr_segment;
    }
    write_load_command(fp, segment_cmds[curr_segment]);

    // Write dynamic loader stuff to file
    fwrite(dy_cmds, dy_size, 1, fp);

    // Write entry point to file
    write_load_command(fp, &ep);

    // Write data to file
    ibsen_list_for_each(seg_it, &image->segments) {
        struct ibsen_segment* segment = container_of(seg_it, struct ibsen_segment, list);

        ibsen_list_for_each(sect_it, &segment->sections) {
            struct ibsen_section* section = container_of(sect_it, struct ibsen_section, list);
            const void* ptr = section->data;

            if (ptr == NULL && section->type == SECTION_BC) {
                ptr = bytecode;
            }

            fwrite(ptr, section->file_size, 1, fp);

            for (size_t i = 0; i < section->file_padding; ++i) {
                fputc('\x00', fp);
            }
        }
    }

    free(dy_cmds);
    for (curr_segment = 0; curr_segment < num_segments; ++curr_segment) {
        free(segment_cmds[curr_segment]);
    }

    return 0;
}

