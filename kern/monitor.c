// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>

// user extended function
#include <inc/mmu.h>
#include <inc/types.h>
#include <inc/atol.h>

#include <kern/pmap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "backtrace", "Display hierachy of calling stack", mon_backtrace },
// extended in Lab2
	{ "show_mappings", "show_mappings vm_low vm_high\nDisplay physical page mappings and corresponding permission bits that apply to the pages between vm_low and vm_high inclusively.", mon_show_mappings},
	{ "set_vm_perm", "set_vm_perm vm_addr perm_bit\nSet permission on specific vm mapping.", mon_set_vm_perm},
	{ "dump_vm", "dump_vm vm_low vm_high\nDump contents of memory given a range of virtual memory address", mon_dump_vm},
	{ "dump_pm", "dump_pm pm_low pm_high\nDump contents of memory given a range of physical memory address", mon_dump_pm}
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

unsigned read_eip();

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < NCOMMANDS; i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		(end-entry+1023)/1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// Your code here.
        cprintf("Stack backtrace:\n");
        uint32_t *ebp = 0;
        ebp           = (uint32_t *) read_ebp();
        struct Eipdebuginfo info;      // storage for debug information
#define FUNCNAMELEN    256
        char func_name[FUNCNAMELEN];
        while(ebp) {
            uintptr_t eip = (uintptr_t ) *(ebp + 1);
            uint32_t arg1 = *(ebp + 2);
            uint32_t arg2 = *(ebp + 3);
            uint32_t arg3 = *(ebp + 4);
            uint32_t arg4 = *(ebp + 5);
            uint32_t arg5 = *(ebp + 6);

            cprintf("  ebp %08x  eip %08x  args %08x %08x %08x %08x %08x\n",
                    ebp, eip, arg1, arg2, arg3, arg4, arg5);
            debuginfo_eip(eip, &info);
            // ignore sub-string from colon
            memmove(func_name, info.eip_fn_name, info.eip_fn_namelen);
            func_name[info.eip_fn_namelen] = '\0';
            cprintf("         %s:%d: %s+%x\n", info.eip_file, info.eip_line,
                    func_name, eip - info.eip_fn_addr);
            ebp = (uint32_t *) *ebp;
        }
	return 0;
}


// extended monitor in Lab2
int
mon_show_mappings(int argc, char **argv, struct Trapframe *tf)
{
	// wrong command format
	if (argc != 3) {
		cprintf("Usage:\n%s vm_low vm_high\n", argv[0]);
	} else {
		long low_mem  = ROUNDDOWN(atol(argv[1]), PGSIZE);
		long high_mem = atol(argv[2]);
		// make sure high memory address is included
		if (!(high_mem % PGSIZE))
			high_mem = high_mem + 1;
		high_mem = ROUNDUP(high_mem, PGSIZE);

		// print header
		cprintf("Virtual addr\tPhysical addr\tPermission\n");
		while (low_mem < high_mem) {
			pte_t *pte = pgdir_walk(kern_pgdir, (void *) low_mem, 0);
			if (pte)
				cprintf("%08p\t%08p\t%03p\n", low_mem, PTE_ADDR(*pte), PGOFF(*pte));
			else
				cprintf("0x00000000\t0x00000000\t0x00000000\n");

			low_mem += PGSIZE;
		}
	}

	return 0;
}

int
mon_set_vm_perm(int argc, char **argv, struct Trapframe *tf)
{
	// wrong command format
	if (argc != 3) {
		cprintf("Usage:\n%s vm_addr vm_perm\n", argv[0]);
	} else {
		long mem  = atol(argv[1]);
		long alig = ROUNDDOWN(mem, PGSIZE);
		long perm = atol(argv[2]);

		pte_t *pte = pgdir_walk(kern_pgdir, (void *) mem, 0);
		if (pte) {
			if (PTE_ADDR(perm))
				cprintf("WARNING: Only lower 12 bits is set.\n");
			*pte = PTE_ADDR(*pte) | PGOFF(perm);
		} else
			cprintf("No physical memory mapped at %08p\n", mem);
	}

	return 0;
}

#define ACCESSIBLE(mem) ((UPAGES < (mem) && (mem) < ULIM) || \
			 ((KSTACKTOP - KSTKSIZE) < (mem) && (mem) < KSTACKTOP) || \
			 (KERNBASE < (mem)))

// utility function
void
dump_mem_line(int low, int high)
{
	if (ACCESSIBLE(low)) {
		cprintf("%08p\t", low & ~0xf);
		if (low & 0xf) {
			int count = 0;
			while (count++ < 16 - high + low)
				cprintf("   "); 
		}

		while (low < high)
			cprintf("%02x ", *(char *) low++ & 0xff);
		cprintf("\n");
	} else {
		cprintf("WARNING: memory at virtual address %08p cannot be accessed.\n", low);
	}
}

void
dump_mem_range(int low, int high, int low_aligned, int high_aligned)
{
	if (low < low_aligned)
		dump_mem_line(low, low_aligned);
	low = low_aligned;
	while (low_aligned < high_aligned) {
		low_aligned += 16;
		dump_mem_line(low, low_aligned);
		low  += 16;
	}
	if (low < high)
		dump_mem_line(low, high);
}

int
mon_dump_vm(int argc, char **argv, struct Trapframe *tf)
{
	// wrong command format
	if (argc != 3) {
		cprintf("Usage:\n%s vm_low vm_high\n", argv[0]);
	} else {
		long low_mem  = atol(argv[1]);
		long high_mem = atol(argv[2]);
		long laligned = ROUNDUP(low_mem, 16);
		long haligned = ROUNDDOWN(high_mem, 16);

		// print header
		cprintf("Address:offset\t00 01 02 03 04 05 06 07 "
			"08 09 0A 0B 0C 0D 0E 0F \n");
		dump_mem_range(low_mem, high_mem, laligned, haligned);
	}

	return 0;
}

int
mon_dump_pm(int argc, char **argv, struct Trapframe *tf)
{
	// wrong command format
	if (argc != 3) {
		cprintf("Usage:\n%s pm_low pm_high\n", argv[0]);
	} else {
		long low_mem  = atol(argv[1]);
		long high_mem = atol(argv[2]);

		// use the last virtual page to map physical address
		uintptr_t va     = 0xf000e000;

		if (PGNUM(low_mem) > npages) {
			cprintf("Memory range beyond physical limit.\n");
		} else {
			if (PGNUM(high_mem) > npages) {
				cprintf("WARNING: memory out of range, "
					"set upper bound limit.\n");
				high_mem = npages * PGSIZE;
			}
			// print header
			cprintf("Address:offset\t00 01 02 03 04 05 06 07 "
				"08 09 0A 0B 0C 0D 0E 0F \n");

			struct Page *pp = NULL;
			pp = pa2page((physaddr_t) low_mem);
			page_insert(kern_pgdir, pp, (void *) va, 0);

			// first line, need padding
			size_t offset = PGOFF(low_mem);
			int space     = low_mem % 16;
			if (space) {
				cprintf("%08p\t", low_mem & ~0xf);
				while (space--)
					cprintf("   ");
			}
			while (low_mem < high_mem) {
				if (!(low_mem % 16))
					cprintf("%08p\t", low_mem & ~0xf);

				if (!(low_mem % PGSIZE)) {
					pp = pa2page((physaddr_t) low_mem);
					page_insert(kern_pgdir, pp, (void *) va, 0);
				}

				cprintf("%02x ",
				(*(char *) (va | PGOFF(low_mem++))) & 0xff);

				if (!(low_mem % 16))
					cprintf("\n");
			}

			if (high_mem % 16)
				cprintf("\n");
		}
	}

	return 0;
}


/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < NCOMMANDS; i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");


	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}

// return EIP of caller.
// does not work if inlined.
// putting at the end of the file seems to prevent inlining.
unsigned
read_eip()
{
	uint32_t callerpc;
	__asm __volatile("movl 4(%%ebp), %0" : "=r" (callerpc));
	return callerpc;
}
