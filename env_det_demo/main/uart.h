
void uart_init(int);

void test_send_packet(int offset);

#define START_BYTES {0xAB, 0xAB}
#define END_BYTES {0xCD, 0xCD}

