/*

				Gurich
	**	Ricoh SP110 series driver **

	Copyright (C) 2016, 2017 Gustaf Haglund <kontakt@ghaglund.se>

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <gurich.h>

/*
 *
 * Expects graceful error handling due to g->initalized.
 *
 */
#ifndef _NO_USB

/*
 * Some models (e.g. the SP112SU, which combines a printer with a scanner)
 * expose the printer function on an interface other than 0 -- interface 0
 * there is the scanner. Instead of hardcoding an interface number, walk the
 * active configuration and claim whichever interface actually advertises
 * itself as a USB printer-class device (bInterfaceClass 0x07). Falls back
 * to interface 0 if that can't be determined, which preserves the original
 * behaviour for the plain SP110.
 */
static unsigned int gurich_find_printer_interface(libusb_device * device)
{
	struct libusb_config_descriptor * config;
	unsigned int iface = 0;

	if (libusb_get_active_config_descriptor(device, &config) != 0) {
		return iface;
	}

	for (uint8_t i = 0; i < config->bNumInterfaces; ++i) {
		const struct libusb_interface * intf = &config->interface[i];

		for (int a = 0; a < intf->num_altsetting; ++a) {
			if (intf->altsetting[a].bInterfaceClass == LIBUSB_CLASS_PRINTER) {
				iface = intf->altsetting[a].bInterfaceNumber;
				libusb_free_config_descriptor(config);
				return iface;
			}
		}
	}

	libusb_free_config_descriptor(config);
	return iface;
}

static void printer_usb(struct gurich_usb * g, struct libusb_device_descriptor devdesc)
{
	char libusb_strerror[128];
	int libusb_error;

	libusb_set_debug(g->ctx, PRINTER_LIBUSB_DEBUG);

	g->idProduct = devdesc.idProduct;
	g->iSerialNumber = devdesc.iSerialNumber;
	g->device_handle = libusb_open_device_with_vid_pid(g->ctx, PRINTER_VENDOR_ID, devdesc.idProduct);

	if (g->device_handle == NULL) {
		goto libusb_fail;
	}

	g->device = libusb_get_device(g->device_handle);
	g->interface = gurich_find_printer_interface(g->device);

	libusb_error = libusb_kernel_driver_active(g->device_handle, g->interface);
	if (libusb_error == 1) {
		if ((libusb_error = libusb_detach_kernel_driver(g->device_handle, g->interface)) < 0) {
			goto libusb_fail;
		}
	}

	libusb_error = libusb_claim_interface(g->device_handle, g->interface);
	if (libusb_error != 0) {
		goto libusb_fail;
	}

	g->initialized = true;
	return;

	libusb_fail:
		snprintf(libusb_strerror, 128, "ERROR: %s", libusb_error_name(libusb_error));

		/*
		libusb_release_interface(g->device_handle, GURICH_USB_INTERFACE);
		libusb_attach_kernel_driver(g->device_handle, GURICH_USB_INTERFACE);
		libusb_close(g->device_handle);
		*/

		libusb_close(g->device_handle);
		libusb_exit(g->ctx);

		fprintf(stderr, "Something went wrong with libusb (%s). Quitting.\n\n", libusb_strerror);
}
#endif

/*
 *
 * Limited to only supporting one connected Ricoh printer,
 * when using the CLI. The CUPS backend can handle more printers.
 */
void check_printer_usb(struct gurich_usb * g)
{
	#ifndef _NO_USB
	libusb_device **list;
	struct libusb_device_descriptor devdesc;
	ssize_t numusbdevs, i, err;

	err = libusb_init(&g->ctx);
	if (err) {
		return;
	}
	numusbdevs = libusb_get_device_list(g->ctx, &list);

	for (i = 0; i < numusbdevs; ++i)
	{
		g->device = list[i];

		if (libusb_get_device_descriptor(g->device, &devdesc) < 0)
			continue;

		if (!devdesc.bNumConfigurations || !devdesc.idVendor
			|| !devdesc.idProduct)
			continue;

		if (devdesc.idVendor == PRINTER_VENDOR_ID) {
			/* Found a Ricoh printer. */
			printer_usb(g, devdesc);
			break;
		}
	}

	if (numusbdevs >= 0) {
		libusb_free_device_list(list, 1);
	}
	#endif
}

void cleanup_usb(struct gurich_usb * g)
{
	if (g->initialized)
	{
		libusb_release_interface(g->device_handle, g->interface);
		libusb_attach_kernel_driver(g->device_handle, g->interface);
		libusb_close(g->device_handle);

		libusb_exit(g->ctx);
	}
}
