/*
 * Copyright (c) 2015-2016 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file C++ Synchronization demo.  Uses basic C++ functionality.
 */

#include <arch/cpu.h>
#include <cstdio>
#include <logging/log.h>
#include <shell/shell.h>
#include <shell/shell_uart.h>
// #include <stdio.h>
#include <sys/printk.h>
#include <version.h>
#include <zephyr.h>

// Loger module instantiation.
LOG_MODULE_REGISTER (main);

/****************************************************************************/

extern void foo ();

void timer_expired_handler (struct k_timer *timer)
{
        LOG_INF ("Timer expired.");

        /* Call another module to present logging from multiple sources. */
        // foo ();
}

K_TIMER_DEFINE (log_timer, timer_expired_handler, NULL);

static int cmd_log_test_start (const struct shell *shell, size_t argc, char **argv, u32_t period)
{
        ARG_UNUSED (argv);

        k_timer_start (&log_timer, K_MSEC (period), K_MSEC (period));
        shell_print (shell, "Log test started\n");

        return 0;
}

static int cmd_log_test_start_demo (const struct shell *shell, size_t argc, char **argv) { return cmd_log_test_start (shell, argc, argv, 200); }

static int cmd_log_test_start_flood (const struct shell *shell, size_t argc, char **argv) { return cmd_log_test_start (shell, argc, argv, 10); }

static int cmd_log_test_stop (const struct shell *shell, size_t argc, char **argv)
{
        ARG_UNUSED (argc);
        ARG_UNUSED (argv);

        k_timer_stop (&log_timer);
        shell_print (shell, "Log test stopped");

        return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE (
        sub_log_test_start,
        SHELL_CMD_ARG (demo, NULL, "Start log timer which generates log message every 200ms.", cmd_log_test_start_demo, 1, 0),
        SHELL_CMD_ARG (flood, NULL, "Start log timer which generates log message every 10ms.", cmd_log_test_start_flood, 1, 0),
        SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_STATIC_SUBCMD_SET_CREATE (sub_log_test, SHELL_CMD_ARG (start, &sub_log_test_start, "Start log test", NULL, 2, 0),
                                SHELL_CMD_ARG (stop, NULL, "Stop log test.", cmd_log_test_stop, 1, 0),
                                SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER (log_test, &sub_log_test, "Log test", NULL);

static int cmd_demo_ping (const struct shell *shell, size_t argc, char **argv)
{
        ARG_UNUSED (argc);
        ARG_UNUSED (argv);

        shell_print (shell, "pong");

        return 0;
}

static int cmd_demo_params (const struct shell *shell, size_t argc, char **argv)
{
        shell_print (shell, "argc = %d", argc);
        for (size_t cnt = 0; cnt < argc; cnt++) {
                shell_print (shell, "  argv[%d] = %s", cnt, argv[cnt]);
        }

        return 0;
}

static int cmd_version (const struct shell *shell, size_t argc, char **argv)
{
        ARG_UNUSED (argc);
        ARG_UNUSED (argv);

        shell_print (shell, "Zephyr version %s", KERNEL_VERSION_STRING);

        return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE (sub_demo, SHELL_CMD (params, NULL, "Print params command.", cmd_demo_params),
                                SHELL_CMD (ping, NULL, "Ping command.", cmd_demo_ping), SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER (demo, &sub_demo, "Demo commands", NULL);

SHELL_CMD_ARG_REGISTER (version, NULL, "Show kernel version", cmd_version, 1, 0);

/****************************************************************************/

/**
 * @class semaphore the basic pure virtual semaphore class
 */
class semaphore {
public:
        virtual int wait () = 0;
        virtual int wait (int timeout) = 0;
        virtual void give () = 0;
};

/* specify delay between greetings (in ms); compute equivalent in ticks */
constexpr size_t SLEEPTIME = 500;
constexpr size_t STACKSIZE = 2000;

k_thread coop_thread;
K_THREAD_STACK_DEFINE (coop_stack, STACKSIZE);

/*
 * @class cpp_semaphore
 * @brief Semaphore
 *
 * Class derives from the pure virtual semaphore class and
 * implements it's methods for the semaphore
 */
class cpp_semaphore : public semaphore {
private:
        k_sem _sema_internal{};

public:
        cpp_semaphore ();
        virtual ~cpp_semaphore () = default;
        int wait () override;
        int wait (int timeout) override;
        void give () override;
};

/*
 * @brief cpp_semaphore basic constructor
 */
cpp_semaphore::cpp_semaphore ()
{
        printk ("Create semaphore %p\n", this);
        k_sem_init (&_sema_internal, 0, UINT_MAX);
}

/*
 * @brief wait for a semaphore
 *
 * Test a semaphore to see if it has been signaled.  If the signal
 * count is greater than zero, it is decremented.
 *
 * @return 1 when semaphore is available
 */
int cpp_semaphore::wait ()
{
        k_sem_take (&_sema_internal, K_FOREVER);
        return 1;
}

/*
 * @brief wait for a semaphore within a specified timeout
 *
 * Test a semaphore to see if it has been signaled.  If the signal
 * count is greater than zero, it is decremented. The function
 * waits for timeout specified
 *
 * @param timeout the specified timeout in ticks
 *
 * @return 1 if semaphore is available, 0 if timed out
 */
int cpp_semaphore::wait (int timeout) { return k_sem_take (&_sema_internal, K_MSEC (timeout)); }

/**
 *
 * @brief Signal a semaphore
 *
 * This routine signals the specified semaphore.
 *
 * @return N/A
 */
void cpp_semaphore::give () { k_sem_give (&_sema_internal); }

cpp_semaphore sem_main;
cpp_semaphore sem_coop;

void coop_thread_entry ()
{
        k_timer timer{};

        k_timer_init (&timer, nullptr, nullptr);

        while (true) {
                /* wait for main thread to let us have a turn */
                sem_coop.wait ();

                /* say "hello" */
                // printk ("%s: Hello World!\n", __FUNCTION__);
                // LOG_ERR ("%s: Hello World!", __FUNCTION__);

                /* wait a while, then let main thread have a turn */
                k_timer_start (&timer, K_MSEC (SLEEPTIME), K_NO_WAIT);
                k_timer_status_sync (&timer);
                sem_main.give ();
        }
}

/****************************************************************************/

void main ()
{
        k_timer timer{};

        k_thread_create (&coop_thread, coop_stack, STACKSIZE, (k_thread_entry_t)coop_thread_entry, nullptr, nullptr, nullptr, K_PRIO_COOP (7), 0,
                         K_NO_WAIT);

        k_timer_init (&timer, nullptr, nullptr);

        while (true) {
                /* say "hello" */
                // printk ("%s: Hello World!\n", __FUNCTION__);
                // LOG_WRN ("%s: Hello World!", __FUNCTION__);

                /* wait a while, then let coop thread have a turn */
                k_timer_start (&timer, K_MSEC (SLEEPTIME), K_NO_WAIT);
                k_timer_status_sync (&timer);
                sem_coop.give ();

                /* Wait for coop thread to let us have a turn */
                sem_main.wait ();
        }
}
