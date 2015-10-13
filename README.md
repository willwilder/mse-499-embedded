MSE 499 Temperature Controller

This is my UW Materials Science and Engineering (MSE) senior project. The project has two software components, an embedded part for the LPC810, and a python part for various other tasks. The embedded software uses the LPC810_CodeBase: https://github.com/microbuilder/LPC810_CodeBase. I implemented a PID temperature controller and a simple command interpreter for setting the various PID constants and getting status, etc. The Python frontend is based on miniterm.py.


to download program to MCU:
lpc21isp ~/nxpworkspace/LPC810_CodeBase/Release/LPC810_CodeBase.hex /dev/ttyUSB0 115200 12000

set uart speed:
stty -F /dev/ttyUSB0 115200

