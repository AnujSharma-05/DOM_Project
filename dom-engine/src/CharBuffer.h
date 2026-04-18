#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct Cell {
    char ch{' '};
    std::uint8_t fg{7};
    std::uint8_t bg{0};
    std::uint8_t attrs{0};

    bool operator==(const Cell& other) const {
        return ch == other.ch && fg == other.fg && bg == other.bg && attrs == other.attrs;
    }
};

class CharBuffer {
public:
    CharBuffer(std::size_t width, std::size_t height);

    void resize(std::size_t width, std::size_t height);

    std::size_t width() const;
    std::size_t height() const;

    void clear_back();
    void set_back(std::size_t x, std::size_t y, Cell cell);
    const Cell& back(std::size_t x, std::size_t y) const;

    std::size_t diff_count() const;
    void swap_buffers();
    std::string emit_ansi() const;

private:
    std::size_t index(std::size_t x, std::size_t y) const;

    std::size_t width_{0};
    std::size_t height_{0};
    std::vector<Cell> front_;
    std::vector<Cell> back_;
};
