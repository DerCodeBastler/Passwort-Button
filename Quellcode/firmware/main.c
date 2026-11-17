/* KeePass-Taster Pico 2, 1.1.0. Keine Passwortausgabe ohne BOOTSEL-Klick. */
#ifdef KEEPASS_HOST_TEST
#include "host_hal.h"
#else
#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"
#include "hardware/regs/sio.h"
#include "tusb.h"
#endif
#include <string.h>
#include <stdio.h>

#define MAX_KEYS 256u
#define STORE_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)
#define MAGIC_V1 0x4b503201u
#define MAGIC 0x4b503202u
#define DEFAULT_DELAY 3u
#define MIN_DELAY 3u
#define MAX_DELAY 50u
#define DEBOUNCE_MS 30u
#define SETUP_HOLD_MS 2000u
#define SETUP_WINDOW_MS 60000u
#define HID_TIMEOUT_MS 250u
#define LINE_TIMEOUT_MS 1000u

typedef struct {
    uint32_t magic, count, checksum, delay_ms, append_enter;
    uint8_t pairs[MAX_KEYS * 2];
} record_t;
typedef struct {
    uint32_t magic, count, checksum;
    uint8_t pairs[MAX_KEYS * 2];
} legacy_record_t;
_Static_assert(sizeof(record_t) <= FLASH_SECTOR_SIZE, "Speicherbereich zu klein");

static record_t record;
static uint8_t caps;
static bool typing, key_down, report_pending, release_pending;
static uint32_t due, last_progress, led_until;
static bool led_feedback, setup_enabled;
static uint32_t setup_until;
static unsigned pos;
static struct {
    bool raw, stable, armed, accepted, held_action;
    uint32_t changed, pressed_at, last_poll;
} button;
static char line[MAX_KEYS * 4 + 20];
static unsigned line_len;
static bool line_invalid;
static uint32_t line_started;

/* Volatile-Schreiben verhindert, dass der Compiler das Leeren entfernt. */
static void wipe(void *memory, size_t length) {
    volatile uint8_t *p = memory;
    while (length--) *p++ = 0;
}
static uint32_t now_ms(void) { return to_ms_since_boot(get_absolute_time()); }
static uint32_t pair_hash(const uint8_t *pairs, unsigned count) {
    uint32_t h = 2166136261u;
    for (unsigned i = 0; i < count * 2; ++i) h = (h ^ pairs[i]) * 16777619u;
    return h ^ count;
}
static uint32_t hash(const record_t *r) {
    uint32_t h = pair_hash(r->pairs, r->count);
    h = (h ^ r->delay_ms) * 16777619u;
    return (h ^ r->append_enter) * 16777619u;
}
static bool valid_pair(uint8_t key, uint8_t mod) {
    bool printable = (key >= 4 && key <= 0x27) ||
                     (key >= 0x2c && key <= 0x38) || key == 0x64;
    return printable && (mod == 0 || mod == 2 || mod == 0x40);
}
static bool valid_record(const record_t *r) {
    if (r->magic != MAGIC || r->count > MAX_KEYS ||
        r->delay_ms < MIN_DELAY || r->delay_ms > MAX_DELAY ||
        r->append_enter > 1 || r->checksum != hash(r)) return false;
    for (unsigned i = 0; i < r->count; ++i)
        if (!valid_pair(r->pairs[2*i], r->pairs[2*i+1])) return false;
    return true;
}
static void load_record(void) {
    memcpy(&record, (const void *)(XIP_BASE + STORE_OFFSET), sizeof record);
    if (valid_record(&record)) return;
    legacy_record_t old;
    memcpy(&old, (const void *)(XIP_BASE + STORE_OFFSET), sizeof old);
    wipe(&record, sizeof record);
    record.magic = MAGIC;
    record.delay_ms = DEFAULT_DELAY;
    if (old.magic == MAGIC_V1 && old.count <= MAX_KEYS &&
        old.checksum == pair_hash(old.pairs, old.count)) {
        unsigned count = old.count;
        bool enter = count && old.pairs[(count-1)*2] == 0x28 &&
                     old.pairs[(count-1)*2+1] == 0;
        if (enter) --count;
        bool ok = true;
        for (unsigned i = 0; i < count; ++i)
            if (!valid_pair(old.pairs[2*i], old.pairs[2*i+1])) ok = false;
        if (ok) {
            record.count = count;
            record.append_enter = enter;
            memcpy(record.pairs, old.pairs, count * 2);
        }
    }
    record.checksum = hash(&record);
    wipe(&old, sizeof old);
}

#ifndef KEEPASS_HOST_TEST
/* Unveränderte BOOTSEL-Abfrage aus dem funktionierenden Original:
 * komplett im RAM, Interrupts gesperrt, Core 1 wird nicht gestartet. */
static bool __no_inline_not_in_flash_func(bootsel_pressed)(void) {
    uint32_t irq = save_and_disable_interrupts();
    uint32_t old = ioqspi_hw->io[1].ctrl;
    hw_write_masked(&ioqspi_hw->io[1].ctrl,
        GPIO_OVERRIDE_LOW << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
        IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);
    for (volatile unsigned i = 0; i < 1000; ++i) __asm volatile("nop");
    bool pressed = !(sio_hw->gpio_hi_in & SIO_GPIO_HI_IN_QSPI_CSN_BITS);
    ioqspi_hw->io[1].ctrl = old;
    restore_interrupts(irq);
    return pressed;
}
#endif

static bool save_record(const record_t *next) {
    uint8_t page[((sizeof(record_t) + FLASH_PAGE_SIZE - 1) / FLASH_PAGE_SIZE) * FLASH_PAGE_SIZE];
    memset(page, 0xff, sizeof page);
    memcpy(page, next, sizeof *next);
    uint32_t irq = save_and_disable_interrupts();
    flash_range_erase(STORE_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(STORE_OFFSET, page, sizeof page);
    restore_interrupts(irq);
    wipe(page, sizeof page);
    bool ok = !memcmp((const void *)(XIP_BASE + STORE_OFFSET), next, sizeof *next);
    if (ok) record = *next;
    return ok;
}
static bool setup_open(uint32_t now) {
    if (setup_enabled && (int32_t)(now - setup_until) >= 0) setup_enabled = false;
    return setup_enabled;
}
static void reset_line(void) {
    wipe(line, sizeof line);
    line_len = 0;
    line_invalid = false;
    line_started = 0;
}
static void reply(const char *s) { tud_cdc_write_str(s); tud_cdc_write_flush(); }
static int unhex(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
/* Strikte Grammatik: 3..50, Leerzeichen, 0|1. Keine Vorzeichen/Überläufe. */
static bool parse_options(const char **text, record_t *next) {
    const char *p = *text;
    if (*p < '1' || *p > '9') return false;
    unsigned delay = (unsigned)(*p++ - '0');
    if (*p >= '0' && *p <= '9') delay = delay * 10 + (unsigned)(*p++ - '0');
    if (delay < MIN_DELAY || delay > MAX_DELAY || *p++ != ' ') return false;
    if (*p != '0' && *p != '1') return false;
    next->delay_ms = delay;
    next->append_enter = (unsigned)(*p++ - '0');
    *text = p;
    return true;
}
static void command(void) {
    uint32_t now = now_ms();
    /* Ausschließlich nicht geheime Metadaten; niemals Länge/Tastencodes. */
    if (!strcmp(line, "STATUS")) {
        char result[80];
        snprintf(result, sizeof result, "%s DE V2 DELAY=%lu ENTER=%lu %s\n",
            record.count ? "READY" : "EMPTY", (unsigned long)record.delay_ms,
            (unsigned long)record.append_enter, setup_open(now) ? "OPEN" : "LOCKED");
        reply(result);
        return;
    }
    bool set = !strncmp(line, "SET ", 4);
    bool config = !strncmp(line, "CONFIG ", 7);
    bool clear = !strcmp(line, "CLEAR");
    if (!set && !config && !clear) { reply("ERR COMMAND\n"); return; }
    if (typing || release_pending || report_pending) { reply("ERR BUSY\n"); return; }
    if (!setup_open(now)) { reply("ERR LOCKED\n"); return; }
    record_t next = record;
    const char *error = NULL;
    if (clear) {
        wipe(next.pairs, sizeof next.pairs);
        next.count = 0;
        next.append_enter = 0;
    } else {
        const char *p = line + (set ? 4 : 7);
        if (!parse_options(&p, &next)) error = "ERR OPTIONS\n";
        else if (config && *p) error = "ERR OPTIONS\n";
        else if (set) {
            if (*p != ' ') error = "ERR LENGTH\n";
            else {
                ++p;
                size_t n = strlen(p);
                if (!n || n % 4 || n > MAX_KEYS * 4) error = "ERR LENGTH\n";
                else {
                    wipe(next.pairs, sizeof next.pairs);
                    next.count = n / 4;
                    for (unsigned i = 0; i < n / 2; ++i) {
                        int a = unhex(p[2*i]), b = unhex(p[2*i+1]);
                        if (a < 0 || b < 0) { error = "ERR HEX\n"; break; }
                        next.pairs[i] = (uint8_t)((a << 4) | b);
                    }
                    for (unsigned i = 0; !error && i < next.count; ++i)
                        if (!valid_pair(next.pairs[2*i], next.pairs[2*i+1])) error = "ERR KEY\n";
                }
            }
        }
    }
    if (error) reply(error);
    else {
        next.magic = MAGIC;
        next.checksum = hash(&next);
        setup_enabled = false; /* Eine gültige Schreibaktion pro Tastengeste. */
        if (!save_record(&next)) reply("ERR FLASH\n");
        else {
            reply(clear ? "OK CLEARED\n" : config ? "OK CONFIG\n" : "OK SAVED\n");
            led_until = now_ms() + 800;
            led_feedback = true;
        }
    }
    wipe(&next, sizeof next);
}
static void poll_serial(void) {
    uint32_t now = now_ms();
    if ((line_len || line_invalid) && now - line_started >= LINE_TIMEOUT_MS) {
        reset_line();
        line_invalid = true; /* Rest der alten Zeile bis LF verwerfen. */
        line_started = now;
    }
    /* Begrenzte Arbeit pro Schleife: USB-Flut darf BOOTSEL/HID nicht blockieren. */
    for (unsigned budget = 0; budget < 64 && tud_cdc_available(); ++budget) {
        char c = (char)tud_cdc_read_char();
        if (c == '\r') continue;
        if (c == '\n') {
            line[line_len] = 0;
            if (line_invalid) reply("ERR FRAME\n");
            else command();
            reset_line();
        } else {
            if (!line_len && !line_invalid) line_started = now;
            if ((unsigned char)c < 32 || (unsigned char)c > 126 || line_len >= sizeof line - 1)
                line_invalid = true;
            else if (!line_invalid) line[line_len++] = c;
        }
    }
}
static void abort_typing(void) {
    typing = false;
    key_down = false;
    report_pending = false;
    release_pending = true;
    pos = 0;
}
static void reset_session(void) {
    abort_typing();
    setup_enabled = false;
    caps = 0;
    memset(&button, 0, sizeof button);
    button.changed = now_ms();
    reset_line();
}
void tud_mount_cb(void) { reset_session(); }
void tud_umount_cb(void) { reset_session(); }
void tud_suspend_cb(bool remote_wakeup_en) { (void)remote_wakeup_en; reset_session(); }
void tud_resume_cb(void) { reset_session(); }
void tud_cdc_line_state_cb(uint8_t instance, bool dtr, bool rts) {
    (void)instance; (void)rts;
    if (!dtr) { reset_line(); setup_enabled = false; }
}
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t id, hid_report_type_t type,
                              uint8_t *buf, uint16_t len) {
    (void)instance; (void)id; (void)type; (void)buf; (void)len;
    return 0; /* Kein Kontrolltransfer liefert Tastencodes oder Passwort. */
}
void tud_hid_set_report_cb(uint8_t instance, uint8_t id, hid_report_type_t type,
                           uint8_t const *buf, uint16_t len) {
    if (instance == 0 && id == 0 && type == HID_REPORT_TYPE_OUTPUT && len == 1)
        caps = (buf[0] & KEYBOARD_LED_CAPSLOCK) ? 1 : 0;
}
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const *report, uint16_t len) {
    (void)instance; (void)report; (void)len;
    if (!typing || !report_pending) return;
    report_pending = false;
    last_progress = now_ms();
    due = last_progress + record.delay_ms;
    if (!key_down) {
        ++pos;
        if (pos >= record.count + record.append_enter) typing = false;
    }
}
void tud_hid_report_failed_cb(uint8_t instance, hid_report_type_t type,
                              uint8_t const *report, uint16_t len) {
    (void)instance; (void)type; (void)report; (void)len;
    abort_typing();
}
static void poll_button(uint32_t now) {
    if (now - button.last_poll < 5) return;
    button.last_poll = now;
    bool pressed = bootsel_pressed();
    if (pressed != button.raw) { button.raw = pressed; button.changed = now; }
    if (now - button.changed < DEBOUNCE_MS) return;
    if (button.raw != button.stable) {
        button.stable = button.raw;
        if (button.stable) {
            button.accepted = button.armed && !typing && !release_pending &&
                              tud_mounted() && !tud_suspended() && tud_hid_ready();
            button.armed = false;
            button.held_action = false;
            button.pressed_at = now;
        } else {
            if (button.accepted && !button.held_action && !typing &&
                tud_mounted() && !tud_suspended() && tud_hid_ready()) {
                if (setup_open(now)) setup_enabled = false;
                else if (record.count) {
                    /* Einzige Stelle, die eine Passwortausgabe startet. */
                    typing = true;
                    key_down = false;
                    report_pending = false;
                    pos = 0;
                    due = now + 150;
                    last_progress = now;
                }
            }
            button.accepted = false;
            button.armed = tud_mounted() && !tud_suspended();
        }
    }
    if (!button.stable) button.armed = tud_mounted() && !tud_suspended();
    if (button.stable && button.raw && button.accepted && !button.held_action &&
        now - button.pressed_at >= SETUP_HOLD_MS) {
        button.held_action = true;
        setup_enabled = true;
        setup_until = now + SETUP_WINDOW_MS;
        reset_line();
        tud_cdc_read_flush();
    }
}
static void poll_hid(uint32_t now) {
    if (!tud_mounted() || tud_suspended()) {
        if (typing) abort_typing();
        return;
    }
    uint8_t keys[6] = {0};
    if (release_pending) {
        if (tud_hid_ready() && tud_hid_keyboard_report(0, 0, keys)) release_pending = false;
        return;
    }
    if (!typing) return;
    if (now - last_progress >= HID_TIMEOUT_MS) { abort_typing(); return; }
    if (report_pending || (int32_t)(now - due) < 0 || !tud_hid_ready()) return;
    uint8_t modifier = 0;
    if (!key_down) {
        uint8_t key = pos < record.count ? record.pairs[pos*2] : 0x28;
        if (pos < record.count) modifier = record.pairs[pos*2+1];
        if (caps && ((key >= 4 && key <= 29) || key == 0x2f || key == 0x33 || key == 0x34)
            && modifier != 0x40) modifier ^= 2;
        keys[0] = key;
    }
    if (tud_hid_keyboard_report(0, modifier, keys)) {
        key_down = !key_down;
        report_pending = true;
    }
    wipe(keys, sizeof keys);
}
static void app_tick(void) {
    tud_task();
    poll_serial();
    uint32_t now = now_ms();
    poll_button(now);
    poll_hid(now);
    if (led_feedback && (int32_t)(now - led_until) >= 0) led_feedback = false;
    bool lit = typing || led_feedback;
    if (!lit && setup_open(now)) lit = now % 1000 < 100 || (now % 1000 >= 200 && now % 1000 < 300);
    else if (!lit && !record.count) lit = now % 2000 < 100;
    gpio_put(PICO_DEFAULT_LED_PIN, lit);
}
#ifndef KEEPASS_HOST_TEST
int main(void) {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    load_record();
    tusb_init();
    while (1) { app_tick(); tight_loop_contents(); }
}
#endif
