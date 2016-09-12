/*
 * Console configuration suitable for use in public cloud
 * environments, or any environment where direct console access is not
 * available.
 *
 */

/* Log to syslog(s) server
 *
 * The syslog server to be used must be specified via e.g.
 * "set syslog 192.168.0.1".
 */
#define CONSOLE_SYSLOG
#define CONSOLE_SYSLOGS

/* Log to serial port
 *
 * Note that the serial port output from an AWS EC2 virtual machine is
 * generally available (as the "System Log") only after the instance
 * has been stopped.
 */
#define CONSOLE_SERIAL

/* Log to partition on local disk
 *
 * If all other log mechanisms fail then the VM boot disk containing
 * the iPXE image can be detached and attached to another machine in
 * the same cloud, allowing the log to be retrieved from the log
 * partition.
 */
#define CONSOLE_INT13
