// An address ABOVE 0xC0000000 is shared between ALL processes and is used by the kernel.
// An address BELOW 0xC0000000 is only used by the CURRENT process, and may include NON-USER pages, such as the KERNEL STACK

#include <memory>
#include "elf.h"
#include "drawing.h"
#include "error.h"
#include "fs.h"
#include "memory.h"
#include "process.h"
#include "video.h"

using namespace kernel;

// Manages a process' memory.
class ProcessMemoryManager
{
public:
    template <typename T>
    class SharedMemory;

    ProcessMemoryManager(void* pgdir_p)
        : pgdir(pgdir_p, PF_WRITE)
    {
        // Wipe lower page directory
        for (auto i = 0; i < 768; i++) pgdir[i] = 0;

        // Map the kernel into process
        for (auto i = 768; i < 1023; i++) pgdir[i] = system_pdt[i];

        // Map in the PDT
        pgdir[1023] = ((unsigned)pgdir_p)+(PF_PRESENT|PF_WRITE);
    }
    // Given the logical address of the page directory,
    // and the desired logical address, this function ensures
    // that the page exists in memory, and returns its physical address
    void* getpage(void* vaddr, int writeable, int user)
    {
        // Offset into the page directory
        unsigned pgdir_off = ((unsigned)vaddr)>>22;
        // Offset into the page table
        unsigned pgtbl_off = (((unsigned)vaddr)>>12)&0x3FF;
        // Get or allocate physical address of page table
        void* pgtbl_p;
        if (pgdir[pgdir_off]&1)
        {
            pgtbl_p = (void*)(pgdir[pgdir_off]&~0xFFF);
        }
        else
        {
            pgtbl_p = page_alloc(1);
            pgdir[pgdir_off] = (unsigned)pgtbl_p+(PF_PRESENT|PF_WRITE|PF_USER);
        }
        // Temporarily map in page table in kernel space
        MappedMemory<unsigned> pgtbl(pgtbl_p, PF_WRITE);
        // Allocate the page and map it in user space
        void* pg_p;
        if (pgtbl[pgtbl_off] & 1)
        {
            pg_p = (void*)(pgtbl[pgtbl_off]&0xFFFFF000);
        }
        else
        {
            pg_p = page_alloc(1);
            pgtbl[pgtbl_off] = (((unsigned)pg_p)&~0xFFF)+(PF_PRESENT|(writeable ? PF_WRITE : 0)|(user ? PF_USER : 0));
        }
        // return the page
        return pg_p;
    }

    template <typename T = char>
    SharedMemory<T> map(T* vaddr, unsigned bytes, int writeable, int user)
    {
        return SharedMemory<T>(this, vaddr, bytes, writeable, user);
    }

    // Creates and maps memory of the given size at the given address
    // and maps it into this process.
    template <typename T = char>
    class SharedMemory
    {
    public:
        SharedMemory()
            : ptr(nullptr), bytes(0) {}
        SharedMemory(SharedMemory&) = delete;
        SharedMemory(SharedMemory&& other) noexcept
            : SharedMemory()
        {
            swap(*this, other);
        }
        SharedMemory& operator=(SharedMemory&& other) // move assign
        {
            swap(*this, other);
            return *this;
        }
        friend void swap(SharedMemory& a, SharedMemory& b)
        {
            using std::swap;
            swap(a.ptr, b.ptr);
            swap(a.bytes, b.bytes);
        }
        ~SharedMemory()
        {
            if (ptr == nullptr || bytes <= 0) return;
            for (unsigned t = 0; t < bytes; t += 4096)
            {
                page_unmap((char*)(void*)ptr+t);
            }
        }
        const T *get() const { return ptr; }
        T *get() { return const_cast<T*>(static_cast<const SharedMemory&>(*this).get()); }
        T& operator[](int index)
        {
            return const_cast<T&>(static_cast<const SharedMemory&>(*this)[index]);
        }
        const T& operator[](int index) const
        {
            if (index < 0 || index * sizeof(T) >= bytes) [[unlikely]]
            {
                throw OutOfBoundsError();
            }
            return ptr[index];
        }
    private:
        friend class ProcessMemoryManager;
        SharedMemory(ProcessMemoryManager* mgr, T* vaddr, unsigned bytes, int writeable, int user)
            : ptr((T*)kfindrange(bytes)), bytes(bytes)
        {
            for (unsigned t = 0; t < bytes; t += 4096)
            {
                page_map((char*)(void*)ptr+t, mgr->getpage((char*)(void*)vaddr+t, writeable, user), PF_WRITE);
            }
        }
        T *ptr;
        unsigned bytes;
    }; // class SharedMemory
private:
    MappedMemory<unsigned> pgdir;
}; // ProcessMemoryManager

extern "C" {

static void* driver_end = reinterpret_cast<void*>(0xE0000000); // FIXME: FIX FOR MULTITHREADING

void* vm8086_gpfault;

} // extern "C"

int process_start(std::shared_ptr<kernel::FileSystem> fs, const char* filename)
{
    void *pgdir_p;
    {   auto elf = fs->open(filename);
        Elf32_Ehdr ehdr;
        try
        {
            elf->read(ehdr);
        }
        catch(const kernel::EndOfFileError&)
        {
            throw ElfError("Invalid ELF header.");
        }
        if ((ehdr.e_ident[EI_MAG0] != ELFMAG0) || 
            (ehdr.e_ident[EI_MAG1] != ELFMAG1) || 
            (ehdr.e_ident[EI_MAG2] != ELFMAG2) || 
            (ehdr.e_ident[EI_MAG3] != ELFMAG3))
        {
            throw ElfError("Invalid ELF header.");
        }
        if (ehdr.e_version != EV_CURRENT || ehdr.e_ident[EI_VERSION] != EV_CURRENT)
        {
            throw ElfError("Unsupported ELF version.");
        }
        if (ehdr.e_type != ET_EXEC)
        {
            throw ElfError("ELF is not an executable.");
        }
        if (ehdr.e_machine != K_ELF_MACHINE)
        {
            throw ElfError("Unsupported ELF machine.");
        }
        if (ehdr.e_ident[EI_CLASS] != K_ELF_CLASS)
        {
            throw ElfError("Unsupported ELF class.");
        }
        if (ehdr.e_ident[EI_DATA] != K_ELF_DATA)
        {
            throw ElfError("Unsupported ELF data encoding.");
        }
        {   // Allocate a page directory for the process
            pgdir_p = page_alloc(1);
            ProcessMemoryManager manager(pgdir_p);

            // DPL0 stack is 0xBFFFF000-0xC0000000
            // DPL3 stack is 0xBFFFE000-0xBFFFF000

            // Create a DPL3 stack
            manager.getpage(reinterpret_cast<void*>(0xBFFFE000), 1, 1);

            // Set up the process's DPL0 stack
            {
                auto _kstack = manager.map<unsigned>(reinterpret_cast<unsigned*>(0xBFFFF000), 0x1000, 1, 0);
                auto kstack = _kstack.get()+1024;
                *--kstack = 0x43;                           // SS
                *--kstack = 0xBFFFF000;                     // ESP initial value
                *--kstack = 0x0202;                         // EFLAGS
                *--kstack = 0x3B;                           // CS; 0x38 is DPL3 code. (0x3B: RPL=3)
                *--kstack = reinterpret_cast<unsigned>
                                (ehdr.e_entry);             // EIP; set to entry point for initial value
                *--kstack = 0;                              // These values are POPAD'd
                *--kstack = 0;
                *--kstack = 0;
                *--kstack = 0;
                *--kstack = 0;
                *--kstack = 0;
                *--kstack = 0;
                *--kstack = 0;
                *--kstack = 0x43;                   // Segment selectors for DS, ES, FS, & GS. 0x40 is DPL3 data. (0x43: RPL=3)
                *--kstack = 0x43;
                *--kstack = 0x43;
                *--kstack = 0x43;
            }
            
            // load program segments
            for (auto i = 0; i < ehdr.e_phnum; i++)
            {
                Elf32_Phdr phdr;
                elf->seek(ehdr.e_phoff+ehdr.e_phentsize*i);
                elf->read(phdr);
                if (phdr.p_type == PT_LOAD)
                {
                    elf->seek(phdr.p_offset);
                    auto pmap = manager.map<char>((char*)phdr.p_vaddr, phdr.p_memsz, phdr.p_flags&PF_W, 1);
                    elf->read(pmap.get(), phdr.p_filesz);
                    for (auto i = phdr.p_filesz; i < phdr.p_memsz; i++)
                        pmap[i] = 0;
                }
            }
        }  // ProcessMemoryManager manager
    }  // std::unique_ptr<kernel::File> elf
    // Create the process and schedule it for execution
    Process* p = process_create(filename);
    p->cr3 = (unsigned)pgdir_p;
    p->esp = 0xBFFFFFBC;
    p->priority = 3;
    p->vm8086 = 0;
    p->blocked = 0;
    p->fpu_saved = 0;
    p->stdio[0] = fs->register_descriptor(std::make_unique<PipeFile>()); // stdin
    p->stdio[1] = fs->register_descriptor(std::make_unique<PipeFile>()); // stdout
    p->stdio[2] = fs->register_descriptor(std::make_unique<PipeFile>()); // stderr
    process_enqueue(p);
    return p->pid;
}

int load_driver(std::shared_ptr<kernel::FileSystem> fs, std::shared_ptr<kernel::SymbolManager> symbols, const char* filename)
{
    int ret = 0;
    auto elf = fs->open(filename);;
    Elf32_Ehdr ehdr;
    int i;
    try
    {
        elf->read(ehdr);
    }
    catch(const kernel::EndOfFileError&)
    {
        throw ElfError("Invalid ELF header.");
    }
    if ((ehdr.e_ident[EI_MAG0] != ELFMAG0) || 
        (ehdr.e_ident[EI_MAG1] != ELFMAG1) || 
        (ehdr.e_ident[EI_MAG2] != ELFMAG2) || 
        (ehdr.e_ident[EI_MAG3] != ELFMAG3))
    {
        throw ElfError("Invalid ELF header.");
    }
    if (ehdr.e_version != EV_CURRENT || ehdr.e_ident[EI_VERSION] != EV_CURRENT)
    {
        throw ElfError("Unsupported ELF version.");
    }
    if (ehdr.e_type != ET_REL)
    {
        throw ElfError("ELF is not a valid driver.");
    }
    if (ehdr.e_machine != K_ELF_MACHINE)
    {
        throw ElfError("Unsupported ELF machine.");
    }
    if (ehdr.e_ident[EI_CLASS] != K_ELF_CLASS)
    {
        throw ElfError("Unsupported ELF class.");
    }
    if (ehdr.e_ident[EI_DATA] != K_ELF_DATA)
    {
        throw ElfError("Unsupported ELF data encoding.");
    }

    auto shdrs = std::make_unique<Elf32_Shdr[]>(ehdr.e_shnum);
    elf->seek(ehdr.e_shoff);
    std::unique_ptr<Elf32_Sym[]> symtab(nullptr);

    // read section headers
    for (i = 0; i < ehdr.e_shnum; i++)
    {
        elf->seek(ehdr.e_shoff+ehdr.e_shentsize*i);
        elf->read(shdrs[i]);
    }

    // Read the section header string table
    auto shstrtab = std::make_unique<char[]>(shdrs[ehdr.e_shstrndx].sh_size);
    elf->seek(shdrs[ehdr.e_shstrndx].sh_offset);
    elf->read(shstrtab.get(), shdrs[ehdr.e_shstrndx].sh_size);

    int j, cnt;
    Elf32_Shdr *s;
    std::unique_ptr<char[]> strtab(nullptr);
    // Load sections and symbol table
    for (i = 1; i < ehdr.e_shnum; i++)
    {
        s = &shdrs[i];
        switch (s->sh_type)
        {
        case SHT_PROGBITS:
            if (s->sh_flags & SHF_ALLOC)
            {
                s->sh_addr = driver_end;
                driver_end = (void*)(((unsigned)driver_end + s->sh_size + 0xFFF) & 0xFFFFF000);
                for (j = 0; j < s->sh_size; j += 4096)
                    page_map((char*)s->sh_addr + j, page_alloc(1), s->sh_flags & SHF_WRITE ? PF_WRITE : PF_NONE);
                elf->seek(s->sh_offset);
                elf->read((char*)s->sh_addr, s->sh_size);
            }
            break;
        case SHT_NOBITS:
            if (s->sh_flags & SHF_ALLOC)
            {
                s->sh_addr = driver_end;
                driver_end = (void*)(((unsigned)driver_end + s->sh_size + 0xFFF) & 0xFFFFF000);
                for (j = 0; j < s->sh_size; j += 4096)
                    page_map((char*)s->sh_addr + j, page_alloc(1), s->sh_flags & SHF_WRITE ? PF_WRITE : PF_NONE);
                kzeromem(s->sh_addr, s->sh_size);
            }
            break;
        case SHT_SYMTAB:
            if (kstrcmp(".symtab", shstrtab.get()+s->sh_name) != 0) break;
            cnt = s->sh_size/s->sh_entsize;
            symtab = std::make_unique<Elf32_Sym[]>(cnt);
            for (j = 0; j < cnt; j++)
            {
                elf->seek(s->sh_offset+s->sh_entsize*j);
                elf->read(symtab[j]);
            }
    
            // Read the linked string table
            strtab = std::make_unique<char[]>(shdrs[s->sh_link].sh_size);
            elf->seek(shdrs[s->sh_link].sh_offset);
            elf->read(strtab.get(), shdrs[s->sh_link].sh_size);
            break;
        }
    }

    void* drivermain = 0;
    static char t[1024];
    // Update symbols
    for (i = 1; i < cnt; i++)
    {
        if (symtab[i].st_shndx == SHN_UNDEF)
        {
            Elf32_Sym *ksym = symbols->find_symbol((const char*)strtab.get() + symtab[i].st_name);
            if (ksym)
            {
                symtab[i].st_shndx = SHN_ABS;
                symtab[i].st_value = ksym->st_value;
            }
            else if (ELF32_ST_BIND(ksym->st_info) != STB_WEAK)
            {
                ksprintf(t, "Undefined symbol '%s'", strtab.get()+symtab[i].st_name);
                throw ElfError(t);
            }
        }
        else if (symtab[i].st_shndx < SHN_LORESERVE)
        {
            symtab[i].st_value = (char*) symtab[i].st_value + (unsigned)shdrs[symtab[i].st_shndx].sh_addr;
            if (!kstrcmp("driver_main", (const char*)strtab.get()+symtab[i].st_name)) drivermain = symtab[i].st_value;
        }
    }

    if (!drivermain)
    {
        throw ElfError("Could not find entry driver_main.");
    }

    // Perform relocations
    for (i = 1; i < ehdr.e_shnum; i++)
    {
        s = &shdrs[i];
        if (s->sh_type == SHT_REL)
        {
            // Avoid relocating debug symbols if we didn't load that section
            bool loaded = false;
            switch (shdrs[s->sh_info].sh_type)
            {
                case SHT_PROGBITS:
                case SHT_NOBITS:
                    if (shdrs[s->sh_info].sh_flags & SHF_ALLOC)
                    {
                        loaded = true;
                    }
                    break;
            }
            if (loaded)
            {
                int count = s->sh_size/s->sh_entsize;
                for (j = 0; j < count; j++)
                {
                    Elf32_Rel rel;
                    elf->seek(s->sh_offset+j*s->sh_entsize);
                    elf->read(rel);
                    unsigned soff = (unsigned)shdrs[s->sh_info].sh_addr;
                    unsigned* ptr = (unsigned*)(void*)((char*)rel.r_offset + soff);
                    Elf32_Sym sym = symtab[ELF32_R_SYM(rel.r_info)];
                    unsigned symval = (unsigned)sym.st_value;
                    switch (ELF32_R_TYPE(rel.r_info))
                    {
                    case R_386_32:
                        *ptr += symval;
                        break;
                    case R_386_PC32:
                        *ptr += symval - (unsigned)ptr;
                        break;
                    }
                }
            }
        }
    }

    int retval;
    asm volatile("call *%1":"=a"(retval):"g"(drivermain));
    return retval;
}
