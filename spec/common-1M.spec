{
 'NAME' => 'cobalt-1M.rom',
 'SIZE' => 0x100000,
 'BOOT_START' => 0x00000,
 'FS_SIZE' => 0xf0000,
 'FS_START' => 0x10000,
 'FS_FILES' => [
		['monitor.bz2', "x86/monitor/monitor.bz2"],
		['vmlinux.bz2', "$ENV{LINUXDIR}/vmlinux.bz2"],
	       ]
};
