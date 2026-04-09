#include "kraken.h"
#include <stdint.h>

#define USB_DESC_DEVICE             0x01u
#define USB_DESC_CONFIGURATION      0x02u
#define USB_DESC_STRING             0x03u
#define USB_DESC_INTERFACE          0x04u
#define USB_DESC_ENDPOINT           0x05u
#define USB_DESC_IAD                0x0Bu
#define USB_CLASS_MISC              0xEFu
#define USB_SUBCLASS_COMMON         0x02u
#define USB_PROTOCOL_IAD            0x01u
#define USB_CLASS_CDC               0x02u
#define USB_CLASS_CDC_DATA          0x0Au

static const uint8_t s_desc_device[] = {
    18, USB_DESC_DEVICE,
    0x10, 0x02,
    USB_CLASS_MISC, USB_SUBCLASS_COMMON, USB_PROTOCOL_IAD, 64,
    0xB1, 0x30,
    0x03, 0x10,
    0x00, 0x01,
    0x01, 0x02, 0x03,
    0x01
};

static const uint8_t s_desc_config[] = {
    9, USB_DESC_CONFIGURATION, 67, 0, 2, 1, 0, 0x80, 50,
    8, USB_DESC_IAD, 0, 2, USB_CLASS_CDC, USB_SUBCLASS_COMMON, USB_PROTOCOL_IAD, 0,
    9, USB_DESC_INTERFACE, 0, 0, 1, USB_CLASS_CDC, 0x02, 0x01, 0,
    5, 0x24, 0x00, 0x10, 0x01,
    4, 0x24, 0x02, 0x02,
    5, 0x24, 0x06, 0x00, 0x01,
    5, 0x24, 0x01, 0x03, 0x01,
    7, USB_DESC_ENDPOINT, 0x81, 0x03, 8, 0, 16,
    9, USB_DESC_INTERFACE, 1, 0, 2, USB_CLASS_CDC_DATA, 0x00, 0x00, 0,
    7, USB_DESC_ENDPOINT, 0x02, 0x02, 64, 0, 0,
    7, USB_DESC_ENDPOINT, 0x82, 0x02, 64, 0, 0
};

static uint16_t s_desc_string[32];

static uint16_t *ascii_to_utf16(const char *s) {
    uint16_t len = 0;
    while (s[len] != '\0' && len < 31) {
        s_desc_string[1 + len] = (uint8_t)s[len];
        ++len;
    }
    s_desc_string[0] = (uint16_t)((USB_DESC_STRING << 8) | (2 + len * 2));
    return s_desc_string;
}

const uint8_t *tud_descriptor_device_cb(void) {
    return s_desc_device;
}

const uint8_t *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return s_desc_config;
}

const uint16_t *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;
    switch (index) {
        case 0:
            s_desc_string[0] = (uint16_t)((USB_DESC_STRING << 8) | 4);
            s_desc_string[1] = 0x0409;
            return s_desc_string;
        case 1: return ascii_to_utf16("Night Traders Dev");
        case 2: return ascii_to_utf16("Kraken USB Console");
        case 3: return ascii_to_utf16("SG2002-CDC-0001");
        default: return ascii_to_utf16("Kraken");
    }
}
