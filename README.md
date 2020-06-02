## LoRaSoundkit
**Open source (hardware) sound level meter for the internet of things.**

* [General](#General)
* [Electronic components assembly](#electronic-components-assembly)
* [Board configuration](#Board-configuration)
* [Libraries](#Libraries)
* [LoRa TTN keys](#LoRa-TTN-keys)
* [Payload Interface](#Payload-Interface)
* [Example graphical output Sound Kit](#Example-graphical-output-Sound-Kit)

## General

This Soundkit sensor measures continuously audible sound by analyzing the data using FFT. The results are send each minute to the LoRa network. The sensor measures  audible spectrum from 31.5 Hz to 8 kHz divided in 9 octaves. Also each minute the average, minimum and maximum levels are calculated for the 3 weighting curves dB(A), dB(C) and db(Z).

![alt Apeldoorn Sounds Kit](images/soundkit.jpg "Sounds Kit")

## Electronic components assembly
The software is based on ESP32 processor wtih Lora RFM95 module. Two boards has been tested viz. Sparkfun LoRa board and TTGO LoRa board.<br>
Components:
* Sparkfun LoRa Gateway 1-channel ESP32 (used as Sensor), or LilyGO TTGO LoRa32 868MHz ESP32
* I2S MEMS microphone SPH046 or I2S MEMS microphone NMP441
* antenna ¼ lambda, e.g a wire of 8.4 cm length
* power adapter 5V, 0,5A

Below the wiring from MEMS microphone (SPH0645 or NMP443) to processor board (Sparkfun or TTGO):
| SPH0645 | NMP442 |  |Sparkfun| TTGO |
| ------- | ------ |--|--------|-------|
| 3V | 3V | <--> | 3V | 3V |
| GND | GND | <--> | GND | GND|
| BCLK |  | <--> | 18 |  |
| DOUT |  | <--> | 19 |  |
| LRCL |  | <--> | 23 |  |
| SEL |  | nc |   |  |

**N.B.**
For sound measurements lower then 30 dB, the supply to the MEMS microphone must be very clean. The 3V supplied by the Sparkfun ESP gives in my situation some rumble in low frequencies. It can be uncoupled by extra 100nf and 100 uF or a separate 3.3V stabilzer.

## Board configuration
Install ESP32 Arduino Core
Add the line in Arduino→preferences→Additional Boardsmanagers URL:
```
	https://dl.espressif.com/dl/package_esp32_index.json
```
Restart Arduino environment.

In the Arduino menu Tools→Boards, choose Sparkfun Lora gateway board.
If not vissible check the presence of the Sparkfun variant file, see instructions at https://learn.sparkfun.com/tutorials/sparkfun-lora-gateway-1-channel-hookup-guide/programming-the-esp32-with-arduino  

## Libraries
**LMIC**
Install LMIC library from Matthys Kooijman (I did not use the advised LMIC from MCCI-Catena, because it uses US settings).
Download https://github.com/matthijskooijman/arduino-lmic 
and put it in your <arduino-path>\libraries\

**Arduino FFT**
I used the https://www.arduinolibraries.info/libraries/arduino-fft library.
Copy the two files “arduinoFFT.h” and arduinoFFT.ccp” to your .ino main directory

## Config file
In the config,h file the processor board is defined either Sparkfun or TTGO
```
//#define _SPARKFUN         // uncomment if SParkfun board
#define _TTGO               // uncomment if TTGO board
```
The cycle count, how often a measurement is sent to the thingsnetwork in msec.:
```
#define CYCLECOUNT   60000 
```
**LoRa TTN keys**
The device address and keys have to be set to the ones generated in the TTN console. Login in the TTN console and add your device.
Choose activation mode OTAA and copy the APPEUI, DEVEUI and APPKEY keys into this config file:
```
#define APPEUI "0000000000000000"
#define DEVEUI "0000000000000000"
#define APPKEY "00000000000000000000000000000000"
```
## Payload Interface
A LoRa payload message length is max 50 bytes, therefore the message is compressed into values of 12 bits. The payload decoder converts the payload to a JSON message.

One upload message contains the minimum, maximum and average level for a measurment and the 9 octaves spectrum values. This is done for the weigting curves dB(A), dB(C) and db(Z).
The spectrum contains the dB values for the octave bands 31.5Hz, 63Hz, 125Hz, 250Hz, 500Hz, 1kHz, 2kHz, 4kHz and 8kHz

The TTN payload decoder produces the following JSON message:
```
  "la": {
    "avg": 44.2,
    "max": 50.4,
    "min": 39.8,
    "spectrum": [
      22.2,
      30.4,
      37.1,
      36,
      35,
      35.3,
      34.3,
      37.4,
      30.5
    ]
  },
  "lc": {
    "avg": 61.3,
    "max": 72.5,
    "min": 49.5,
    "spectrum": [
      58.6,
      55.8,
      53,
      44.6,
      38.2,
      35.3,
      33.3,
      36.6,
      28.6
    ]
  },
  "lz": {
    "avg": 63.4,
    "max": 75.1,
    "min": 50.3,
    "spectrum": [
      61.6,
      56.6,
      53.2,
      44.6,
      38.2,
      35.3,
      33.1,
      36.4,
      31.6
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







