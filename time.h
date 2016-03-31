#ifndef time_h
#define time_h

#include <Arduino.h>
#include <WiFi101.h>
#include <WiFiUdp.h>
#include <RTCZero.h>

void setTimeUsingTimeServer(RTCZero rtc);
unsigned long readLinuxEpochUsingNTP();
unsigned long sendNTPpacket(IPAddress & address);

#endif
