/**
 * @file AT24C16.cpp
 * @author Figo Arzaki Maulana (figoarzaki123@gmail.com)
 * @brief Simple driver to interface to AT24C16 EEPROM
 * AT24C16 is internally organized with 128 pages of 16 bytes each, it's total size is 16K Bits(2048 Bytes)
 * Note the following from the memory map.
    Each page requires 8-bits to to access the 256 locations/address
    Additional 3 bits are required to address 8 pages
    Hence 13 bits are required to address the entire 16K Memory.
  Page write can only be done 16-bytes at a time.
 * @version 0.1
 * @date 2022-07-03
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "AT24C16.h"

/**
 * @brief Initialize AT24C16 EEPROM
 *
 * @param _i2c I2C instance used
 * @param sda SDA pin
 * @param scl SCL pin
 * @param speed Speed of the I2C bus, 100000 = 100kHz
 * @return true If init success
 * @return false
 */
bool AT24C16::init(i2c_inst_t *_i2c, uint sda, uint scl, uint32_t speed)
{
    static uint8_t rxdata = 0;
    i2c_inst = _i2c;
    i2c_init(i2c_inst, speed);
    gpio_set_function(sda, GPIO_FUNC_I2C);
    gpio_set_function(scl, GPIO_FUNC_I2C);
    gpio_pull_up(sda);
    gpio_pull_up(scl);
    return i2c_read_blocking(i2c_inst, AT24C16_i2cAddress, &rxdata, 1, false) > 0;
}

/**
 * @brief Write memory with specific length to destination address on EEPROM, whole 2048 bytes write takes around ~900ms
 *
 * @param destAddress
 * @param src source memory pointer to write to EEPROM
 * @param len length to be written
 */
void AT24C16::memWrite(uint16_t destAddress, const void *src, size_t len)
{
    const uint8_t *p_data = (const uint8_t *)src;
    // Write first page if not aligned.
    uint8_t notAlignedLength = 0;
    uint8_t pageOffset = destAddress % AT24C16_PageSize;
    if (pageOffset > 0)
    {
        notAlignedLength = AT24C16_PageSize - pageOffset;
        pageWrite(destAddress, p_data, notAlignedLength);
        len -= notAlignedLength;
    }
    if (len > 0)
    {
        destAddress += notAlignedLength;
        p_data += notAlignedLength;

        // Write complete and aligned pages.
        uint8_t pageCount = len / AT24C16_PageSize;
        for (uint8_t i = 0; i < pageCount; i++)
        {
            pageWrite(destAddress, p_data, AT24C16_PageSize);
            destAddress += AT24C16_PageSize;
            p_data += AT24C16_PageSize;
            len -= AT24C16_PageSize;
        }

        if (len > 0)
        {
            // Write remaining uncomplete bytes.
            pageWrite(destAddress, p_data, AT24C16_PageSize);
        }
    }
}

/**
 * @brief Read memory from EEPROM and save it to passed pointer, whole 2048 bytes read takes around ~52ms
 *
 * @param srcAddress EEPROM Address to read from
 * @param dest Destination memory pointer to be written from EEPROM
 * @param len Length of memory to be read
 */
void AT24C16::memRead(uint16_t srcAddress, void *dest, size_t len)
{
    uint8_t wordAddress = srcAddress & 0xFF;
    i2c_write_blocking(i2c_inst, AT24C16_i2cAddress | ((srcAddress >> 8) & 0x07), &wordAddress, 1, false);
    i2c_read_blocking(i2c_inst, AT24C16_i2cAddress | ((srcAddress >> 8) & 0x07), (uint8_t *)dest, len, false);
}

/**
 * @brief Perform byte write operation on to specific address
 *
 * @param destAddress
 * @param byte
 */
void AT24C16::byteWrite(uint16_t destAddress, uint8_t byte)
{
    uint8_t writeBuffer[] = {destAddress & 0xFF, byte};
    i2c_write_blocking(i2c_inst, AT24C16_i2cAddress | ((destAddress >> 8) & 0x07), writeBuffer, sizeof(writeBuffer),
                       false);
}

/**
 * @brief Perform byte read operation from specific address
 *
 * @param srcAddress
 * @return uint8_t Memory read from address on EEPROM
 */
uint8_t AT24C16::byteRead(uint16_t srcAddress)
{
    uint8_t rx = 0;
    uint8_t address_lnibble = srcAddress & 0xFF;
    i2c_write_blocking(i2c_inst, AT24C16_i2cAddress | ((srcAddress >> 8) & 0x07), &address_lnibble, 1, false);
    i2c_read_blocking(i2c_inst, AT24C16_i2cAddress | ((srcAddress >> 8) & 0x07), &rx, 1, false);
    return rx;
}

/**
 * @brief Function to write chunk of 16-bytes of data to EEPROM or Page Write operation on EEPROM
 *
 * @param address
 * @param src
 * @param len
 */
void AT24C16::pageWrite(uint16_t address, const uint8_t *src, uint8_t len)
{
    if (len == 0 || len > 16)
        return;
    const uint8_t *src_b = src;
    uint8_t wordAddress = address & 0xFF;
    i2c_write_blocking(i2c_inst, AT24C16_i2cAddress | ((address >> 8) & 0x07), &wordAddress, 1, true);
    for (size_t i = 0; i < len; ++i)
    {
        while (!i2c_get_write_available(i2c_inst))
            tight_loop_contents();
        i2c_get_hw(i2c_inst)->data_cmd = (i == len - 1) ? *src_b++ | (1 << 9) : *src_b++;
    }
    // From Datasheet
    // tWR max 5ms
    // Note: 1. The write cycle time tWR is the time from a valid stop condition of a write sequence to the end of the
    // internal clear/write cycle
    sleep_ms(7);
}