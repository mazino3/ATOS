
#include <beeper.h>
#include <errno.h>
#include <spinlock.h>
#include <interface.h>

/*
* dev/beeper.c - Beeper Interface
*
* Wrapper functions for starting and stopping a beeper device.
*/

struct beeper_device_interface* default_beeper_driver = NULL;
struct spinlock beeper_lock;

int beeper_start(int freq)
{
	spinlock_acquire(&beeper_lock);

	if (default_beeper_driver != NULL) {
		default_beeper_driver->start(freq);
		spinlock_release(&beeper_lock);
		return 0;

	} else {
		spinlock_release(&beeper_lock);
		return ENODEV;
	}
}

int beeper_stop(void)
{
	spinlock_acquire(&beeper_lock);

	if (default_beeper_driver != NULL) {
		default_beeper_driver->stop();
		spinlock_release(&beeper_lock);
		return 0;

	} else {
		spinlock_release(&beeper_lock);
		return ENODEV;
	}
}

int beeper_register_driver(struct beeper_device_interface* driver)
{
	if (driver == NULL) {
		return ENODEV;
	}
	
	spinlock_acquire(&beeper_lock);

	if (default_beeper_driver != NULL) {
		spinlock_release(&beeper_lock);
		return EALREADY;
	
	} else {
		default_beeper_driver = driver;
		spinlock_release(&beeper_lock);
		return 0;
	}
}
