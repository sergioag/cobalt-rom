{
 'NAME' => 'cobalt-2M-flat.rom',
 'SIZE' => 0x200000,
 'BOOT_START' => 0x1f0000,
 'FS_SIZE' => 0x1f0000,
 'FS_START' => 0x00000,
 'FS_FILES' => [
		['monitor.bz2', "x86/monitor/monitor.bz2"],
		['vmlinux.bz2', "$ENV{LINUXDIR}/vmlinux.bz2"],
	       ]
};
