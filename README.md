## LoRaSoundkit
** Open source (hardware) sound level meter for the internet of things.**

* [General](#General)
* [Electronic components assembly](#electronic-components-assembly)
* [Board configuration](#Board-configuration)
* [Libraries](#Libraries)
* [LoRa TTN keys(#LoRa-TTN-keys)
* [Payload Interface](#Payload-Interface)
* [Example graphical output Sound Kit](#Example-graphical-output-Sound-Kit)

## General

This Soundkit sensor measures continuously audible sound by analyzing the data using FFT. The results are send each minute to the LoRa network. The sensor measures  audible spectrum from 31.5 Hz to 8 kHz divided in 9 octaves. Also each minute the average and peak levels are calculated for the 3 weighting curves dB(A), dB(C) and db(Z).

![alt Apeldoorn Sounds Kit](images/soundkit.jpg "Sounds Kit")

## Electronic components assembly
* Sparkfun Lora Gateway 1-channel (ESP32), used as LoRa Sensor
* I2S MEMS microphone SPH046 or NMP441
* antenna ¼ lambda, e.g a wire of 8.4 cm length

```
+---------+     +-------------+  
| SPH0645 |     | Sparkfun    |  \|/  
+---------+     +-------------+   |  
| 3V      | <-> | 3V      ant | --+
| GND     | <-> | GND         |
| BCLK    | <-> | 18          |
| DOUT    | <-> | 19          |
| LRCL    | <-> | 23          |
| SEL     | nc  |             |
+---------+     |             |
                +-------------+
```

**N.B.**
For sound measurements lower then 30 dB, the supply to the MEMS microphone must be very clean. The 3V supplied by the Sparkfun ESP gives in my situation some rumble in low frequencies. It can be uncoupled by extra 100nf and 100 uF or a separate 3.3V supply.

## Board configuration

* Install ESP32 Arduino Core
Add the line in Arduino→preferences→Additional Boardsmanagers URL:

```
https://dl.espressif.com/dl/package_esp32_index.json
```
Restart Arduino environment.
* In the Arduino menu Tools→Boards, choose Sparkfun Lora gateway board.
If not vissible check the presence of the Sparkfun variant file, see instructions at https://learn.sparkfun.com/tutorials/sparkfun-lora-gateway-1-channel-hookup-guide/programming-the-esp32-with-arduino  

## Libraries
LMIC
Install LMIC library from Matthys Kooijman (I did not use the advised LMIC from MCCI-Catena, because it uses US settings).
Download https://github.com/matthijskooijman/arduino-lmic 
and put it in your <arduino-path>\libraries\

Arduino FFT
I used the https://www.arduinolibraries.info/libraries/arduino-fft library.
Copy the two files “arduinoFFT.h” and arduinoFFT.ccp” to your .ino main directory

Make sure that the pin to I2S interface are set correctly (in file nois9lora.ino)
```
	// define IO pins Sparkfun for I2S for MEMS microphone
	#define BCKL 18
	#define LRCL 23
	#define NOT_USED -1
	#define DOUT 19
```
Make sure for that the PIN configuration for the LoRa LMIC are set correctly (lora.h)
These pins are wired internaly on the Sparkfun board.
```
	// define RFM 95 pins
	#define NSS 16
	#define RXTX LMIC_UNUSED_PIN
	#define RST 5
	#define DIO1 26
	#define DIO2 33
	#define DIO3 32
```
## LoRa TTN keys
The device address and keys have to be set to the ones generated in the TTN console. Login in the TTN console and add your device.
Choose activation mode OTAA and get the APPEUI, DEVEUI and APPKEY keys.
Specify the keys in the file lora.h
```
	// specify here TTN keys 
	#define APPEUI "70BAA57ED002DA53"
	#define DEVEUI "0000000000000001"
	#define APPKEY "049CCC7976E5CC3C802CBCF28F1082AE"
```
## Payload Interface
A Lora message is max 50 bytes. In total 33 values are sent in one message. Therfore each value is compressed in an integer of 12 bits.  

One upload message contains 3 times (for dBA, dBC and dbZ) the peak value, average value and the spectrum. The spectrum contains the dB values for the following frequency bands:
 31.5Hz, 63Hz, 125Hz, 250Hz, 500Hz, 1kHz, 2kHz, 4kHz and 8kHz

The TTN payload decoder produces the following JSON message:
```
{
  "la": {
    "avg": 42.6,
    "peak": 54.9,
    "spectrum": [
      -13,
      5.8,
      26.9,
      35.8,
      35.9,
      34.7,
      33.8,
      32.5,
      31
    ]
  },
  "lc": {
    "avg": 48.5,
    "peak": 64.3,
    "spectrum": [
      23.3,
      31.2,
      42.8,
      44.4,
      39.1,
      34.7,
      32.8,
      31.8,
      29.1
    ]
  },
  "lz": {
    "avg": 48.7,
    "peak": 64.4,
    "spectrum": [
      26.3,
      32,
      43,
      44.4,
      39.1,
      34.7,
      32.6,
      31.5,
      32.1
    ]
  }
}
```
## Example graphical output Sound Kit
Below a graph of a sound measurement in my living room in dB(Z).
In this graph some remarkable items are vissible:
* blue line shows the peaks of the belling comtoise clock each half hour
* visible noise of the dishing machine from 1:00 to 2:00
* low noise of of the fridge, see the  63 Hz and 125 Hz line

![alt Example output](images/grafana.png "Example output")

The green blocks shows the average spectrum levels.
This graph is made with Nodered, InfluxDb and Grafana.







