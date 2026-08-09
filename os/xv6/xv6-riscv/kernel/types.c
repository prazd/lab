unsigned to_little_endian32(unsigned val) {
  unsigned res = 0;
  unsigned char* r_ptr = (unsigned char*)&res;
  unsigned char* val_ptr = (unsigned char*)&val;
  r_ptr[0] = val_ptr[3];
  r_ptr[1] = val_ptr[2];
  r_ptr[2] = val_ptr[1];
  r_ptr[3] = val_ptr[0];
  return res;
}

unsigned long to_little_endian64(unsigned long val) {
  unsigned long res = 0;
  unsigned char* r_ptr = (unsigned char*)&res;
  unsigned char* val_ptr = (unsigned char*)&val;
  r_ptr[0] = val_ptr[7];
  r_ptr[1] = val_ptr[6];
  r_ptr[2] = val_ptr[5];
  r_ptr[3] = val_ptr[4];
  r_ptr[4] = val_ptr[3];
  r_ptr[5] = val_ptr[2];
  r_ptr[6] = val_ptr[1];
  r_ptr[7] = val_ptr[0];
  return res;
}