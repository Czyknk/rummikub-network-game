# Kompilator
CXX = g++

# Flagi kompilacji:
# -std=c++14
# -Wall -Wextra
# -g
# -O2
CXXFLAGS = -std=c++14 -Wall -Wextra -pedantic -g -O2

# Katalogi
SRC_DIR = src
BUILD_DIR = build

# Nazwa pliku wynikowego
TARGET = rummikub_server_app

# Automatyczne znajdowanie wszystkich plików .cpp w katalogu src/
SRCS = $(wildcard $(SRC_DIR)/*.cpp)

# Zamiana listy plików .cpp na .o w katalogu build/
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))

# Domyślny cel (uruchamiany po wpisaniu 'make')
all: $(TARGET)

# Linkowanie (tworzenie pliku wykonywalnego)
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET)
	@echo "--------------------------------------------------"
	@echo "Kompilacja zakonczona sukcesem!"
	@echo "Uruchom serwer komenda: ./$(TARGET) <port>"
	@echo "--------------------------------------------------"

# Kompilacja plików obiektowych
# $@ - plik wynikowy (.o), $< to plik wejściowy (.cpp)
# mkdir -p tworzy katalog build, jeśli nie istnieje
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Sprzątanie projektu (usuwa pliki tymczasowe i binarkę)
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# Oznaczenie celów, które nie są plikami
.PHONY: all clean