CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -O2 -march=native -flto -fstack-protector-strong -D_FORTIFY_SOURCE=2
LDFLAGS = -Wl,-z,relro,-z,now -pie
SRCDIR = src
SOURCES = $(SRCDIR)/main.cpp
TARGET = archlog
INSTALL_DIR = /usr/local/bin
DATA_DIR = /usr/local/share/archlog

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^
	strip $@

clean:
	rm -f $(TARGET)

install: $(TARGET)
	sudo mkdir -p $(INSTALL_DIR) $(DATA_DIR)
	sudo cp $(TARGET) $(INSTALL_DIR)/
	sudo chmod 755 $(INSTALL_DIR)/$(TARGET)
	sudo chown root:root $(INSTALL_DIR)/$(TARGET)
	@echo "ArchVault installed successfully to $(INSTALL_DIR)/$(TARGET)"

test: $(TARGET)
	@echo "Testing ArchVault functionality..."
	./$(TARGET) --help
	./$(TARGET) --summary
	./$(TARGET) -m ERROR --tail=5
	./$(TARGET) --journal --tail=3
	@echo "All tests completed successfully"

check-deps:
	@command -v journalctl >/dev/null 2>&1 || { echo "journalctl not found. Install systemd."; exit 1; }
	@echo "Dependencies check passed"

uninstall:
	sudo rm -f $(INSTALL_DIR)/$(TARGET)
	sudo rm -rf $(DATA_DIR)
	@echo "ArchVault uninstalled"