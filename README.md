# zx-zxcode2tap

This program takes a binary or obj ZX Spectrum machine code program and creates a tap file from it.

It adds a ZX Basic loader as well. I.e. the user just needs to type
~~~
LOAD ""
~~~

This will load the ZX Basic loader, which contains a program that consists basically of
~~~
LOAD "" CODE 16384
LOAD "" CODE 
RANDOMIZE USR EXEC_ADDR
~~~

I.e. the machine code object file is loaded and afterwards executed.


# Usage

~~~
zxcode2tap prg_name -code code_file_name -start addr1 -exec addr2 [-screen screen_file_name]
~~~

- prg_name: The name of the program. Used for the tap filename
   and for the name presented while loading.
- "-code code_file_name": The file containing the machine code binary.
-  "-start addr1": The load code start address.
- "-exec addr2": The machine code execution start address.
- "-screen screen_file_name": The file name of the screen data.


# Example

~~~
./zxcode2tap name -code main.obj -start 25000 -exec 32702
~~~

will take the machine code file 'main.obj' and create a tap file that starts the program called 'name' automatically at address 32702.
The 'main.obj' code is loaded at address 25000.
The created tap file is called 'name.tap'.


# Building

zxcode2tap can be build simply by invoking
~~~
./make
~~~
from the project's root directory.
This will create the zxcode2tap binary in the 'bin' folder.

Building was tested on OSX (10.13.6).
It should work on Linux as well (maybe some header file needs to be added).



