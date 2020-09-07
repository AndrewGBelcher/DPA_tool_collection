# DPA tool collection

A collection of tools, scripts, and examples of Differential Power Analysis and other Power Analysis based work.

## SPI Flash Simulator
A SPI Flash Simulator design for a Xilinx Spartan 7 FPGA for Power Analysis, replaces a 0x10 block of flash data(In practice, this would be Cipher Text).

## Dream Source Labs USB oscilloscope modded for DPA
Modified DSView application to use a DSScope for power trace aquisition. Drives a UART cable to transmit CT input to the target and stores the trace as a .csv with the corresponding CT input as the filename.

## Chipwhisperer Scripts
SimpleSerial.py mod to work with SPI Flash Simulator design, replaced the DSView mod in functionality.
Attack script for ARM7TDMI target power traces, use T-Box AES.

## Misc
