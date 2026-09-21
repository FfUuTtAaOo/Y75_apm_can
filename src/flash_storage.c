#include "flash_storage.h"
#include "sensor_types.h"
#include "crc.h"
#include "at24c04.h"
#include "main.h"
#include <string.h>

/* ================================================================
 *  CRC-16 for stored config integrity check
 * ================================================================ */

// static uint16_t calc_crc(const uint8_t *data, uint16_t len)
// {
//     return crc16_modbus(data, len);
//     // return 0;
// }

/* ================================================================
 *  Load from flash
 * ================================================================ */

int flash_load_all(void)
{
    int i = 0, j = 0, offset = 0, tmp = 0;
    uint8_t at24c04_read_buffer[32] = { 0 };
    floatuint_t matrix_data_tmp[6] = { 0 };
    for (i = 0; i < MATRIX_ROWS; i++) {
        /* Mirrors the layout used by flash_save_matrix(). */
        uint16_t addr = (i != 5) ? (uint16_t)(i * 0x18) : 0x80U;
        BSP_EEPROM_ReadBytes(addr, at24c04_read_buffer, MATRIX_SIZE);

        for (j = 0; j < MATRIX_SIZE; j++) {
            offset = j / 4;
            tmp = j % 4;
            matrix_data_tmp[offset].u32 = U8_TO_U32_SAFE(at24c04_read_buffer[j], matrix_data_tmp[offset].u32, 3 - tmp);
        }
        for (j = 0; j < 6; j++) {
            g_matrix.m[i][j] = matrix_data_tmp[j].f32;
        }
    }

    return 1;
}

/* ================================================================
 *  EEPROM byte-write with page-boundary awareness
 *
 *  AT24C04 page write is limited to 16 bytes and DOES NOT cross a
 *  page boundary: once the end of the current page is reached the
 *  write wraps around to the START of that same page and overwrites
 *  earlier data.  So a burst must be split into chunks that each fit
 *  within the remaining space of the current 16-byte page.
 * ================================================================ */
static void flash_eeprom_write(uint16_t addr, const uint8_t *pData, uint16_t len)
{
    while (len > 0) {
        uint16_t page_rem = AT24C04_PAGE_SIZE - (addr % AT24C04_PAGE_SIZE);
        uint16_t chunk = (len < page_rem) ? len : page_rem;
        BSP_EEPROM_WritePage(addr, pData, chunk);
        addr += chunk;
        pData += chunk;
        len -= chunk;
    }
}

/* ================================================================
 *  Save to flash  (erase-then-write-all pattern)
 * ================================================================ */
int flash_save_matrix(uint8_t index)
{
    uint16_t addr;

    if (index >= MATRIX_ROWS) {
        return 0;
    }

    /* EEPROM layout: matrices 1~5 at 0x00/0x18/0x30/0x48/0x60,
     * matrix 6 at 0x80.  Each matrix is MATRIX_SIZE (24) bytes.
     * flash_eeprom_write() splits the 24 bytes into page-aligned
     * chunks, so non-page-aligned bases (0x18, 0x48) are safe. */
    addr = (index != 5) ? (uint16_t)(index * 0x18) : 0x80U;

    flash_eeprom_write(addr, (uint8_t *)&g_matrix.m[index][0], MATRIX_SIZE);

    return 1;
}

int flash_save_zero(void)
{
    // at24c04_write(FLASH_STORAGE_ADDR + FLASH_OFF_ZERO, (uint8_t *)g_sensor.force_zero, 24);
    return 1;
}
