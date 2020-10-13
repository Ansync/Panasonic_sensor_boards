set auto-load safe-path /
target extended-remote :2331
monitor flash breakpoints 1
monitor semihosting enable
monitor semihosting IOClient 3
monitor reset
load
