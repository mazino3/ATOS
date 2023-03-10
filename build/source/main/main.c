
#include <arch.h>
#include <physical.h>
#include <virtual.h>
#include <heap.h>
#include <sys/stat.h>
#include <string.h>
#include <assert.h>
#include <kprintf.h>
#include <process.h>
#include <uio.h>
#include <cpu.h>
#include <vfs.h>
#include <fcntl.h>
#include <loader.h>
#include <device.h>
#include <console.h>
#include <process.h>
#include <syscall.h>
#include <test.h>
#include <thread.h>
#include <fs/demofs/demofs.h>

void fault_tester(void* arg) {
    (void) arg;

    kprintf("result is %d\n", 1 / ((size_t) arg));

    while (true) {
        kprintf("... ");
    }
}

void terminate_test(void* arg) {
    (void) arg;

    thread_terminate();
}

void fake_shell(void* arg) {
	(void) arg;

	kprintf("\n ATOS Kernel\n     Copyright Alex Boxall 2022\n\n");

	/*
	* Just pretend to launch into a command line shell.
	*/
	while (1) {
        kprintf(">");

		char buffer[256];
        console_gets(buffer, sizeof(buffer));

		/*
		* Must check for an empty string to guard the following strlen()
		* in case some clown decides to type the ^@ escape code.
		*/
		if (buffer[0] == '\n' || buffer[0] == 0) {
			continue;
		}

		/*
		* Strip the final newline if it exists.
		*/
		if (buffer[strlen(buffer) - 1] == '\n') {
			buffer[strlen(buffer) - 1] = '\0';
		}

		if (!strncmp(buffer, "test", 4)) {
#ifdef NDEBUG
			kprintf("Tests are not included in release builds.\n");
#else
			if (strlen(buffer) > 5) {
				test_run(buffer + 5);
			} else {
				kprintf("Missing test name (e.g. 'test sleep')\n");
			}
#endif
			kprintf("\n");
			continue;

		} else if (!strncmp(buffer, "type", 4)) {
			if (strlen(buffer) > 5) {
				char* filename = buffer + 5;
				struct vnode* node;
				int ret = vfs_open(filename, O_RDONLY, 0, &node);
				if (ret != 0) {
					kprintf("Cannot open: %s\n", strerror(ret));
				} else {
					struct stat st;
					ret = vnode_op_stat(node, &st);
					if (ret == 0) {
						uint8_t* buffer = malloc(st.st_size + 1);
						memset(buffer, 0, st.st_size + 1);

						struct uio uio = uio_construct_kernel_read(buffer, st.st_size, 0);
						int ret = vfs_read(node, &uio);
						if (ret != 0) {
							kprintf("Cannot read: %s\n", strerror(ret));
						} else {
							kprintf("%s", buffer);
						}

						free(buffer);

					} else {
						kprintf("Cannot stat: %s\n", strerror(ret));
					}
					
					vfs_close(node);
				}
			} else {
				kprintf("Missing file name\n");
			}
			kprintf("\n");
			continue;

        } else if (!strcmp(buffer, "fork")) {
            int res = thread_fork();

            while (true) {
                kprintf("%d\n", res);
                thread_sleep(1);
            }

        } else if (!strcmp(buffer, "fault")) {
            thread_create(fault_tester, NULL, vas_get_current_vas());
			continue;

        } else if (!strcmp(buffer, "eat")) {
            size_t p = phys_allocate_page();
            (void) p;
			continue;

        } else if (!strcmp(buffer, "eat mid")) {
            for (int i = 0; i < 16; ++i) {
                size_t p = phys_allocate_page();
                (void) p;
            }
            
			continue;

        } else if (!strcmp(buffer, "eat lots")) {
            for (int i = 0; i < 64; ++i) {
                size_t p = phys_allocate_page();
                (void) p;
            }
            
			continue;

        } else if (!strcmp(buffer, "user")) {
            struct process* p = process_create();
            process_create_thread(p, thread_execute_in_usermode, NULL);
            continue;
            
        } else if (!strcmp(buffer, "terminate")) {
            thread_create(terminate_test, NULL, vas_get_current_vas());
            continue;

		} else if (!strcmp(buffer, "restart")) {
			arch_reboot();
			continue;
		}
		
		kprintf("Unsupported command '%s'\n\n", buffer);
	}
}

/*
* Our main kernel function - the first C function to be called when the
* kernel starts running. It responsible for initialising all of the other 
* subsystems and threads.
*/
void kernel_main()
{
	/*
	* The order in which subsystems are initialised in is critical. Systems should
	* be initialised in order of least dependent on other systems to most dependent.
	*/

    kprintf("\nkernel_main\n");
	phys_init();
	virt_init();
    
	heap_init();
	cpu_init();
    heap_reinit();
    thread_init();  
    process_init();
    vfs_init();
	interface_init();

    arch_initialise_devices_no_fs();

	vfs_mount_filesystem("hd0", demofs_root_creator);
    vfs_add_virtual_mount_point("sys", "hd0:/System");
    swapfile_init();
    syscall_init();

    arch_initialise_devices_with_fs();

#ifndef NDEBUG
	test_kernel();
#endif

    process_create_thread(process_create(), fake_shell, NULL);

	/*
	* We ought not run anything else in this bootstrap thread,
	* as it (may) have a different stack to the rest (e.g. on x86, it is
	* a 16KB stack with no canary protection). We won't terminate it though,
    * again due to its (possibly) unique stack.
	*/
	while (1) {
		spinlock_acquire(&scheduler_lock);
		thread_block(THREAD_STATE_UNINTERRUPTIBLE);
		spinlock_release(&scheduler_lock);
	}
}