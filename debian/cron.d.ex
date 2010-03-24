#
# Regular cron jobs for the sipwise-asterisk package
#
0 4	* * *	root	[ -x /usr/bin/sipwise-asterisk_maintenance ] && /usr/bin/sipwise-asterisk_maintenance
