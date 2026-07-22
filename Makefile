CXX      := g++
CXXFLAGS := -std=c++20 -O2 -Wall -Wextra -Wpedantic -pthread
SRCDIR   := src

OBJS := $(SRCDIR)/orderbook.o \
        $(SRCDIR)/exchange.o  \
        $(SRCDIR)/generator.o \
        $(SRCDIR)/main.o

TARGET := lob

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(SRCDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(SRCDIR)/orderbook.o: $(SRCDIR)/orderbook.cpp \
    $(SRCDIR)/orderbook.h $(SRCDIR)/order.h $(SRCDIR)/trade.h \
    $(SRCDIR)/symbol_table.h

$(SRCDIR)/exchange.o: $(SRCDIR)/exchange.cpp \
    $(SRCDIR)/exchange.h $(SRCDIR)/orderbook.h $(SRCDIR)/spsc_queue.h \
    $(SRCDIR)/gateway_msg.h $(SRCDIR)/symbol_table.h

$(SRCDIR)/generator.o: $(SRCDIR)/generator.cpp \
    $(SRCDIR)/generator.h $(SRCDIR)/order.h $(SRCDIR)/symbol_table.h

$(SRCDIR)/main.o: $(SRCDIR)/main.cpp \
    $(SRCDIR)/exchange.h $(SRCDIR)/generator.h $(SRCDIR)/symbol_table.h

clean:
	rm -f $(SRCDIR)/*.o $(TARGET)

run: all
	./$(TARGET)

.PHONY: all clean run
