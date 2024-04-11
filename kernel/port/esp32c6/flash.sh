
PORT="$1"
OUTPUT="$2"

esptool.py -b 921600 --port "$PORT" \
    write_flash --flash_mode dio --flash_freq 80m --flash_size 2MB \
    0x0 port/esp32c6/bootloader.bin \
    0x10000 "$OUTPUT/badger-os.bin" \
    0x8000 port/esp32c6/partition-table.bin
