# JoStomp HX
JoStomp HX

This project is a mini midi controller for HX Stomp. It allows easy access to the 3 snapshots of a preset, while retaining the possibility of remaining in Footswitch mode on the HX. This offers many more possibilities than a double TRS footswitch.
This pedal is uassi configured to scroll through the presets and provides access to a mini RC20 type looper.

Jo Stomp HX

The JoStomp HX can

    select the three snapshots
    scroll up/down through presets 
    act as a two button LOOPER controller
    
    ... and can do more if we want !

MIDI Muppet Main Modes


scroll preset: red, snapshot: green, looper: purple
Inooper :	play: green, record: flash red, overdub: yellow

To select a mode (snapshot or scroll presets, press and hold  button 2


Overview
Using the Modes:

SCROLL Mode:     buttons 1/3 switches prog patches down/up
                 2 long : mode SNAPSHOT
                 buttons 1+3 both long: LOOPER mode
                 
 
  SNAPSHOT Mode: buttons 1/2/3 select snapshot 1/2/3
                 button 2 long : mode SCROLL
                 buttons 1+3 both long: LOOPER mode
   
  LOOPER Mode:   button 1  record/play/overdub
                 button 3  play/stop undo(long)
                 button 2  select mode HX (presets, snapshots, footswitch)
                 button 3 long  toggles undo/redo
                 button 2 long : mode SNAPSHOT
                 buttons 1+3 both long: mode SNAPSHOT

Setting the MIDI Channel

MIDI channel can be set to any value between 1 and 16. HX Stomp listens on channel 1 per default.

To change the MIDI Channel:

oStomp    Press and hold button 1 while powering up the J. After a second the device will indicate the currently set MIDI channel by slowly flashing the green LED (1 flash = channel 1, ..., 16 flashes = channel 16).
    Press up to increase the MIDI channel or press dn to decrease the channel.
    Press and hold up and dn to exit MIDI channel configuration.

MIDI Channel configuration will be stored in EEPROM and will be loaded on restart.
Building MIDI Muppet HX

Parts are around 20€:

    Stomp case: e.g. Hammond 1590A
    3 momentary foot switches
    Arduino Pro Mini with programming headers populated (5V)
    MIDI/DIN Socket
    2,1 mm power Socket
    LED RGB
    5 x 220R resistors (5V version of Arduino)
    FTDI serial adaptor (for programming)
    diode IN4147



building MIDI Muppet

drilled
Wiring

    Button 1/2/3 on pins D6, D7,D8
    LED RGB green red blue D3, D4, D5 via 220R resistor common cathond to ground
    Arduino TX pin: via 220R resistor to MIDI pin 5 (data line)
    Arduino 5V/VCC pin: via 220R resistor to MIDI pin 4 (voltage reference line)
    Arduino RAW pin: 9V from power socket via diode (protection)
    Arduino GNC pin: GND from power socket

schematic

Wiring

I put a little bit of capton tape on backside of a foot switch and on the the inside of the case for isolation and fixated the Arduino PCB with a little bit of hot snot.

Hot Snot
The Code

The code requires the OneButton and the JC_Button library to be installed. The Arduino library manager will be your friend.
Programming

Disconnect external power supply first! The FTDI adaptor will provide power.

Hook up the FTDI adaptor to the Arduino board, select "Arduino Pro or Pro Mini" in your Arduino IDE, load the code, compile and upload.

The LED will flash rapidly on boot. Congratulations, you have just created a powerful controller for your HX Stomp. Have fun.

