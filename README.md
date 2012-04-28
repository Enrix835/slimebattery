#What is it?

slimebattery is one of the most lighweight battery systray icon
you can find. It depends only on GTK+ and you must have 'acpi'
installed. No other extra libs.
It has sever handy command line options, and it supports your
favorite icon theme (slimebattery supports faenza icon-theme,
elementary and others).

#Examples
slimebattery can show your battery percentage in different ways.

###slimebattery showing battery icon that updates data every minute
	slimebattery --interval 60
###slimebattery in text mode showing extra information
	slimebattery --text-mode --verbose --interval 10

#Requirements
* gcc
* gtk+-2.4
* acpi

#Install
	$ make
	# make install

#Uninstall
	# make uninstall

#Credits

Enrico "Enrix835" Trotta <enrico{DOT}trt{AT}gmail{DOT}com>

Obviously: sed -e 's/{DOT}/./g' -e 's/{AT}/@/' README
  
