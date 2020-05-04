


void oneNibble(char*& store, unsigned char val) {
  val &= 0xF;
  *store++ = (val < 10 ? '0' : 'A' - 10) + val;
}

void oneByte(char*& store, unsigned char val) {
  oneNibble(store, val >> 4);
  oneNibble(store, val);
}

void oneULong(char* store, unsigned long val) {
  oneByte(store, val >> 24);
  oneByte(store, val >> 16);
  oneByte(store, val >> 8);
  oneByte(store, val);
  *store = 0;
}