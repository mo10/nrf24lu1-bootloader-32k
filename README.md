# nrf24lu1-bootloader-32k
 Added reset function for Nordic Semiconductor `bootloader 32k`
## Usage
```
usage: bootlu1p [options] <hex-file>
  options:
    -r Reset after programming
    -f 16 Flash size is 16K Bytes
    -f 32 Flash size is 32K Bytes
```
## Build
### bootloader_32k
 Use Keil C51 to build 
### host_application
 In Windows, run `win32make.bat`
 In Linux ,run `make`
