Assume that we have a custom board that differs significantly from dev boards supported by zephyr (listed on their website). My main concerns aka tasks to try out are:

1. How to configure an existing driver so it accesses correct GPIOs. Let's start gently and blink a LED. There is an [example](https://docs.zephyrproject.org/latest/samples/basic/blinky/README.html) provided already, so let's analyze it and adapt it to my "custom" board so it would blink a custom LED.
2. How to configure some slightly more advanced drivers like UART or SPI.
3. How to write a device driver for a peripheral which is not yet supported by a board, provided that the API exists for this type of peripherals in the Zephyr source code. Other drivers of such type can be of great help and can be taken as a reference.
4. What is the difference between Zephyr interfacing a peripheral that is built into the µC and a peripheral which is connected to the system by some other means like SPI or I²C.
5. How to write a driver or support in some way or another a peripheral (not necessarily built into the µC) for which there is no API provided in the Zephyr.
6. How to support entirely new µC (of the family that is supported)?

Other tasks regarding a mobile camera project.

7. How to use external memory available in STM32F746G-DISCO (8MB) SDRAM. It seems that the only boards that use sdram are imxrt-s. So this is my only reference.
8. Use the camera module. Again, the only board that supports a video sensor is i.MX rt 1064.
9. Use a wifi module. The best would be a module mounted on the Arduino shield because DISCO board provides a header.
10. COAP.

# Links and tips

* https://docs.zephyrproject.org/latest/guides/build/index.html
* https://docs.zephyrproject.org/latest/guides/dts/intro.html
* https://docs.zephyrproject.org/latest/application/index.html#custom-board-definition
* https://docs.zephyrproject.org/latest/samples/application_development/out_of_tree_board/README.html

- ```*.dts``` are for the boards
- ```*.dtsi``` are included by *.dts and can be found in boarts and dts/arch
- ```*.overlay``` are in samples, tests and boards/shields

# Device tree notes
* [It is used to describe the hardware](https://docs.zephyrproject.org/latest/guides/dts/intro.html) - helps to build the same codebase on a variety of different hardware targets.
* The final output is stored in `devicetree.h` (can be included anywhere).
* This header file contains a bunch of macros starting with `DT_`. Generated macros have lower case letters as well whereas non-generated are all uppercase.

# Ad 1. configuring a GPIO
Add `CONFIG_GPIO=y` to the `prj.conf`

For the board, I am currently using (STM32F746G-DISCO) there is `led0` alias defined already. To check which `dts`, `dtsi` and `overlay` files get assembled into the final `zephyr.dts` refer to a file `zephyr-test/build/zephyr/stm32f746g_disco.dts.pre.d` which seems to contain everything that final `zephyr.dts` comprises of. Inside we can see a `led_1` which has a label `green_led_1`:

```dts
green_led_1: led_1 {
  gpios = <&gpioi 1 GPIO_ACTIVE_HIGH>;
  label = "User LD1";
};
```

Further we can see an alias `led0` like so:

```dts
aliases {
  led0 = &green_led_1;
};
```

Then I [followed this guide](https://docs.zephyrproject.org/latest/application/index.html#custom-board-definition) which basically **only** meant copying the original configuration from `zephyrproject/zephyr/boards/arm/stm32f746g_disco` to my application directory : `zephyr-test/boards/arm/stm32f746g_disco`. This allows me to modify the board configuration (Kconfig and device tree files) outside the main tree. As simple as that. The only difference is that now I have to point the cmake to the right path to look for the board config:

```sh
cd /home/iwasz/workspace/zephyr-test
ninja -C build pristine
cmake -B build -G Ninja -b -DBOARD=stm32f746g_disco -DBOARD_ROOT=. .
ninja -C build
```

After that it was a matter of adding another led to the dts...

```dts
red_led_1: led_2 {
  gpios = <&gpioa 0 GPIO_ACTIVE_HIGH>;
  label = "User LD2";
};
```

... and accesing the node from C using `DT_NODELABEL` rather than `DT_ALIAS`:

```c
#define RED_LED_1_NODE DT_NODELABEL (red_led_1)
```

# Ad 7. SDRAM memory
There's a PR pending, so I won't delve into this subject at least for now. It seems to be complicated judging from the conversation guys working on that had.

Questions:
* [ ] Is that true, that SDRAM can be used in the exactly same way as SRAM does from the programmer (user) perspective? Can I for example declare an array worth 1MB of elements on the stack?
* [x] What with LD script then? Should the SDRAM be reflected somehow in the LD script? Yes, for sure. iMX RT-s have sdram options (turned on conditionally) in the ld script.

# Ad 8. Camera module
yyy