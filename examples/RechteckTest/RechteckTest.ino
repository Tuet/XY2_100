/*  XY2_100 BasicTest.ino - Basic XY2_100 Output Test
    Copyright (c) 2018 Lutz Lisseck

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.

  Required Connections
  --------------------
    pin 2:  CLOCK+
    pin 14: SYNC+
    pin 7:  CHAN1+
    pin 8:  CHAN2+
    pin 6:  CLOCK- 
    pin 20: SYNC- 
    pin 21: CHAN1- 
    pin 5:  CHAN2- 
    pin 15 & 16 - do not use
    pin 4 - Do not use
    pin 3 - Do not use as PWM.  Normal use is ok.

*/

#include <XY2_100.h>
XY2_100 galvo;

void setup() {
  galvo.begin();
  galvo.setXY(0,0);
}

void loop() {
  static int32_t x = 5000, y = 10000;
  const uint16_t pause = 10;
  galvo.setSignedXY(x, y);
  delay(pause);
  galvo.setSignedXY(-x, y);
  delay(pause);
  galvo.setSignedXY(-x, -y);
  delay(pause);
  galvo.setSignedXY(x, -y);
  delay(pause);
  x+=200;
  y+=300;
  if(x>32000) x = 0;
  if(y>32000) y = 0;
}


