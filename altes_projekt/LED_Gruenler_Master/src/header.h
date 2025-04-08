void Transmission(int inhalt1, int inhalt2, uint8_t streifen_von, uint8_t streifen_bis);

void SPI_code();

void print_code();

void vergleich(uint8_t byte6, uint8_t byte7, uint8_t byte8);

void find_transmission(uint8_t byte, int value1, int value2);

void modi5_transmission(uint8_t byte); // Modi9 (Snake) jeweiligen Strip ansteuern
