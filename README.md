Advantest R6581(T) DMM TFT display upgrade - STM32 BluePill + LT7680A-R + 4.58" 320x960 TFT

R6581 VFD reverse engineering by MickleT, who developed the original OLED conversion. EEVBlog thread:
https://www.eevblog.com/forum/repair/advantest-r6581-vfd-replacement/

My own input is importing the original project to Visual Studio 2022 / VisualGDB and converting it
to drive a 320x960 TFT LCD.

My webpage:
https://www.ianjohnston.com/index.php/conversions/video-blog-148-r6581totft

**Build:**
- Visual Studio 2022 with the VisualGDB plugin (https://visualgdb.com).
- The .HEX file is in /VisualGDB/Debug. Programming notes are in _Programming.txt.
- Function prototypes in some headers are missing; VS2022/VisualGDB still builds working code.


**Interface with R6581 (VFD data sniffing):**
```
BluePill    Function
PB11        VFD RESTART (EXTI, start of a display refresh cycle)
PB13        VFD SCK
PB15        VFD SDA
```
- One refresh cycle = 47 packets of 5 bytes (U4 shift register), one packet per 5x7 character cell.
- Main line = 18 characters, Aux line = 29 characters.

Link / button options:
- PB0 (LK1): HIGH = Main white / Aux yellow, LOW = Main yellow / Aux white.
- PB1 (LK2): HIGH = annunciators green, LOW = annunciators cyan.
- PA12: front panel DCV button held during power-up = enter TFT timing selection. Timings are stored in
  the last Flash page (EEPROM emulation at 0x0800FC00).
- PA11/PA12: DCV button pressed in normal use toggles the 1000mVdc / 1Vdc display mode.
- PC13: test LED.


**Display hardware:**
- Controller: Levetop LT7680A-R (128Mb built-in SDRAM), driven by STM32 SPI1 (PA4 CS, PA5 SCK, PA6 MISO, PA7 MOSI), reset on PB10.
- TFT: EastRising ER-TFT4.58-1 / AUO 4.58", 320x960 visible, ST7701S driver, 18-bit RGB from the LT7680.
  Same panel as the R6243 TFT project.
- LT7680 canvas is 400x960 (LCD_XSIZE_TFT = 400). The first 80 columns (x = 0-79) are overscan and
  not on the glass, so the visible area is x = 80-399. Text and graphics are rotated 90 degrees.
- ST7701S is initialised once at boot over a bit-banged 9-bit SPI (lcd.c, CS PB3, SCK PB4, SDI PB5).
  Two init sets: AdaFruit_Init() ("AdaF", default) and BuyDisplay_Init() ("BuyD"), selected by the
  stored timing set.
- LT7680 controller adaptor PCB files: /Hardware.


**Fixes - November 2026 (random stuck pixels / edge pixels):**
Same fixes as the 34401A, 3478A and R6243 TFT projects.

1. SDRAM auto-refresh was never enabled (main cause of random pixels) - lt7680.c SDRAM_Init_LT()
   REG[E4h] was set to 0x04 first. Bit 2 (SDR_PARAMEN) switches REG[E0h]-REG[E3h] to the SDRAM
   timing-parameter registers, so the attribute, CAS latency and refresh interval values went to the
   wrong registers and refresh stayed at its default of 0000h (= refresh disabled).
   Fix: do not set bit 2. Write E0h=0x29, E1h=0x03, E2h/E3h=refresh interval, then E4h=0x01.
   Matches Levetop AP-Note V1.2 page 27.
   E0h value depends on the chip: 0x29 for 128Mb (LT7680A-R/B-R, LT7681/3/6), 0x21 for 64Mb (LT7680A/B).

2. Active Window one pixel short in each direction - lt7680.c ConfigureActiveDisplayArea_LT()
   REG[5Ah-5Dh] are the Active Window WIDTH and HEIGHT (datasheet page 158), not end coordinates.
   Writing size-1 left column x=399 and line y=959 outside the window, so they were never cleared.
   Fix: write LCD_XSIZE_TFT (400) and LCD_YSIZE_TFT (960).
   The "right wipe" in the main loop of main.c (8 black DrawLine() calls at y = 952-959, run on every
   display update) is no longer needed and is commented out. Re-enable it if edge pixels return.

3. LT7680 busy / FIFO checks - lt7680.c and lt7680.h
   Status Register bits: bit7 = write FIFO full, bit6 = write FIFO empty, bit3 = core busy.
   New helpers: WaitForLT7680Ready() (bit3 clear), WaitForLT7680WriteFifoNotFull() (bit7 clear),
   WaitForLT7680WriteComplete() (bit6 set, then bit3 clear).
   - DrawText(): waits for idle and FIFO room before each character, and for FIFO empty + idle at the end.
   - SetTextColors() and ConfigureFontAndPosition(): wait for idle first (datasheet requires this before
     changing colour, spacing, rotation or cursor).
   - DrawLine(): waits for idle first and polls REG[67h] bit7 until the line is finished.
   All waits have timeouts so a fault cannot hang the meter.

4. WaitForTextReady() (34401A display.c) polled the wrong register (REG[BAh], SPI Master status).
   Not applicable here - the R6581 code has no WaitForTextReady(); text waits go through DrawText().
   (DrawText_old() in lt7680.c has its REG[BAh] polling commented out and is not used.)


**Notes for alignment (not changed):**
- TFT timings (VBPD, VFPD, VSPW, HBPD, HFPD, HSPW, refresh rate) are user-selectable from 6 sets in
  main.c (LCD_xxx_SETTINGS[]) and saved to Flash. Set 6 uses the BuyDisplay ST7701S init.
- AdaFruit_Init() runs the ST7701S in DE mode (C3h = 0x02). BuyDisplay_Init() runs it in HV mode
  (C3h = 0x81, then HBP = 0x38, VBP = 0x22), so the panel finds the picture from the sync pulses.
- LT7680 horizontal timing registers are in 8-pixel units. REG[17h] adds a 0-15 pixel fine tune to the
  horizontal back porch.
- REG[12h] bit5 (DISP_TEST) shows a colour bar generated from the LT7680 timing only - useful for
  checking that the frame fills the glass.
- Earlier testing noted y = 958/959 hidden and "938 not 960 seems to be far right edge" - check again
  with the test box now the Active Window and SDRAM fixes are in.
- main.c has a commented-out 1-pixel test box around the visible 320x960 area (x = 80-399, y = 0-959)
  in the TEMP TEST block after the boot ClearScreen(). Uncomment it to check alignment.

Datasheets used: LT768x_DS_V42_ENG.pdf, LT768x_AP-Note_V12_ENG.pdf, ST7701S_Ver1.4.pdf.

Ian.
