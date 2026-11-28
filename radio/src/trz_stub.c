/*
 * TRZ pass-1 stub (see radio/util/trz.py): an empty blob, so that the decoder in
 * lcd.cpp links. firmware_pass1.elf is only used to find out which strings
 * survive --gc-sections; it is never flashed.
 */
const char TRZ_BLOB[1] = { 0 };
const unsigned char TRZ_DICT[256] = { 0 };
__asm__("\t.global TRZ_BLOB_END\n\t.set TRZ_BLOB_END, TRZ_BLOB\n");
