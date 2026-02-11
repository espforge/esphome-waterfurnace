CXX      := g++
CXXFLAGS := -std=c++17 -Wall -I mocks -I ../..
CXXFLAGS += $(shell pkg-config --cflags gtest 2>/dev/null)
LDFLAGS  := $(shell pkg-config --libs gtest gtest_main 2>/dev/null || echo "-lgtest -lgtest_main -lpthread")

TESTS := test_protocol test_sensor test_binary_sensor test_text_sensor test_switch test_climate

.PHONY: test clean

test: $(TESTS)
	@for t in $(TESTS); do echo "=== $$t ===" && ./$$t && echo || exit 1; done

$(TESTS): %: %.cpp hub_stubs.h mocks/esphome_types.h
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

# test_protocol doesn't use hub_stubs.h but listing it as dependency is harmless

clean:
	rm -f $(TESTS)
