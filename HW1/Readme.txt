Run the OS
	1 - Go to inside project in ubuntu terminal.
	2 - Select the kernel.cpp according to test case
		a - Copy the kernel1.cpp to kernel.cpp for firstStrategy for PartA
		b - Copy the kernel2.cpp to kernel.cpp for firstStrategy for PartB
		c - Copy the kernel3.cpp to kernel.cpp for secondStrategy for PartB
	3 - Run make clean and make mykernel.iso commands
	4 - Open the virtual machine and select mykernel.iso file as iso file of own operating system.

Warning:
	In my makefile, the run part is like this: 

	run: mykernel.iso
		(killall VirtualBoxVM && sleep 1) || true
		"/mnt/c/Program Files/Oracle/VirtualBox/VBoxManage.exe" startvm "My Operating System"

	I specified the path to VBoxManage.exe because I use wsl in windows. If you are going to run it from ubuntu, you should add it to the makefile file run section.


	run: mykernel.iso
		(killall VirtualBox && sleep 1) || true
		VirtualBox --startvm 'My Operating System' &