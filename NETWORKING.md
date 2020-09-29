# Networking 
Networking is implemented for the ```stm32f746g_disco``.
- (Top) → Device Drivers → Entropy Drivers → STM32 RNG driver : OFF
- (Top) → Random subsystem → Non-random number generator : ON

Without the latter option I got: ```undefined reference to `sys_rand32_get'```

or ```error: 'DT_N_INST_0_st_stm32_rng_P_label' undeclared here (not in a function); did you mean 'DT_N_INST_0_st_stm32_rcc'?```

if depending on the former setting. As stated [here](https://github.com/zephyrproject-rtos/zephyr/issues/15565), one can also put these into ```prj.conf```:

```ini
CONFIG_ENTROPY_GENERATOR=y
CONFIG_TEST_RANDOM_GENERATOR=y
```

**PROBLEM** I cannot send big amounts of data at once. I'm trying to send 480B in a loop.  Board disconnects after sending 1 big buffer of data. Shrinking the buffer helps.