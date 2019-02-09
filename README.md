# XY2_100
XY2_100 library for Teensy 3.2 / LC (Arduino) 

This library outputs a XY2-100 compatible data-stream to control laser galvanometers. A Teensy 3.2 or Teensy LC is needed. This library uses DMA transfer to generate the differential signals nearly with full XY2-100 speed. See the examples how to use this library.

## Connections
As the Teensy outputs 3.3V signals in a differential fashion, for short cables it can be directly connected to the galvanometer. For reliable purpose the usage of RS485 drivers with galvanic isolation is strongly recommended.

```
  Required Connections
  --------------------
    pin  2: CLOCK+
    pin 14: SYNC+
    pin  7: CHAN1+
    pin  8: CHAN2+
    pin  6: CLOCK- 
    pin 20: SYNC- 
    pin 21: CHAN1- 
    pin  5: CHAN2- 
```  
    

License
----

MIT

