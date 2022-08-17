# LatencyMeterRefreshed

[![GitHub top language](https://img.shields.io/github/languages/top/AndyFilter/Latency-Meter-CSharp.svg)](https://en.wikipedia.org/wiki/C_Sharp_(programming_language))  [![Windows](https://img.shields.io/badge/platform-Windows-0078d7.svg)](https://en.wikipedia.org/wiki/Microsoft_Windows) ![GitHub all releases](https://img.shields.io/github/downloads/AndyFilter/Latency-Meter-CSharp/total.svg) [![GitHub](https://img.shields.io/github/license/AndyFilter/Latency-Meter-CSharp.svg)](https://github.com/AndyFilter/Latency-Meter-CSharp/blob/main/LICENSE) 

This program lets you **measure latency of your system** given that you have Arduino. ***Now in C++!***

# Measure performance and latency of your PC

Are you Interested in measuring the latency of your system, but can't afford **750\$ monitor**? Well in that case you are not alone! This program let's you even measure the latency of single component like **mouse latency, system latency,** or just overall latency. For not more than **10$**!

# What will you need

- **Arduino** (I used UNO, but I think it will work with others too, you will just need to change the second line in the loop function to utilize *digitalRead* function instead of reading directly from the registers.)
- Some jumper wires.
- A photoresistor (I don't think the type really matters, but mine seems to be working from 300Ω to 5MΩ).
- Like a 500Ω resistor (Not required. On Arduino Uno you can use built-in pullup resistor).
- A button.
- Windows 10 / 11(not tested) and basic knowledge of using it.
  
  # How to install
1. Make sure you are in possession of **Arduino** or other microcontroller that allows for a serial communication with a PC.
2. Connect the Arduino to the other elements as follows:

> **Light_Sensor - A0,**  
> **Button - D2**

I would recommend putting a **500Ω resistor in series with the light sensor**. The button is configured to use arduino-UNO's built-in **"pullup resistor" (button, INPUT_PULLUP)**.

**Reminder!** button is reversed, I mean the code detects press of a button normally, but the state is different (if (buttonState == LOW)). All it means is that you plug it to digital pin 2 and to ground.

**For the ease of use** I would suggest connecting **photoresistor not on the same board as the button**, even better just don't use a board for it! Because you will need to place it in the correct place of your monitor.

![Arduino UNO Schema](https://media.discordapp.net/attachments/687599007643729964/1009519797748375683/LightsensorLatencyMeterTrans.png)

3. Connect the Arduino (or the microcontroller of choice) to the PC via USB.
4. Upload the [***Code***](https://github.com/AndyFilter/LatencyMeterRefreshed/blob/main/Arduino/SystemLatencyMeter.ino) onto the board.
5. Download the [***latest release***](github.com/AndyFilter/LatencyMeterRefreshed/releases/latest) of the program.

# How to use

1. Make sure you have everything connected as shown in the schematic above
2. Connect the Arduino the the PC via USB.
3. Open the program and set the correct **serial port of the Arduino**.
4. Move the photoresistor so that it *looks* at the black rectangle at the bottom of the program.
5. Press *Connect*. If everything goes right program should not close and you should see no errors.
6. Press the **Button** connected to the **PIN 2** of the Arduino.
7. The measurements should appear on the screen!
8. You can now **Save** them, and if that's not your first time using the program you can even **Open** the saves you exported earlier!

# Modifications

If you want to change for example the baudrate, you will not only need to change it in the program's [code](https://github.com/AndyFilter/LatencyMeterRefreshed/blob/4fecf90172a97df74cab3bb14bb9c1e6ab2867e5/serial.cpp#L8), but also the microcontrollers's code. It is done that way to save on *cpu* cycles on the Arduino part. I could make it so that you can change the baud rate from the program itself, but the current value of 19200 allows for **sub milliseconds delays** and also is reliable. All this makes it kind of pointless to change it to 9600 or maybe even 152000. With that said, I think the program could benefit from things like custom delay between measurements etc. I might look into that and check if it adds any significant latency, if not; expect it in some future update.

# Questions and Issues

If you have any kind of question or issue to report. DM me through Discord: **IsPossible#8212**, or create an issue on **GitHub**

# Edits & Forks

Feel free to fork the project. Tweak, fix and add to the code. I tried to add as many **useful comments** to the code as it's possible, so code should be **easy to read**.
I'll be fixing and adding to the code myself, when I'll see a need to.


