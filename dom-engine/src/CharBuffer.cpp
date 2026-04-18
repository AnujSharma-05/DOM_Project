#include "CharBuffer.h"

#include <algorithm>
#include <cassert>
#include <sstream>

namespace {

std::vector<Cell> make_cells(std::size_t width, std::size_t height) {
    return std::vector<Cell>(width * height);
}

}  // namespace

CharBuffer::CharBuffer(std::size_t width, std::size_t height)
    : width_(width), height_(height), front_(make_cells(width, height)), back_(make_cells(width, height)) {}

void CharBuffer::resize(std::size_t width, std::size_t height) {
    width_ = width;
    height_ = height;
    front_ = make_cells(width, height);
    back_ = make_cells(width, height);
}

std::size_t CharBuffer::width() const {
    return width_;
}

std::size_t CharBuffer::height() const {
    return height_;
}

void CharBuffer::clear_back() {
    std::fill(back_.begin(), back_.end(), Cell{});
}

void CharBuffer::set_back(std::size_t x, std::size_t y, Cell cell) {
    back_[index(x, y)] = cell;
}

const Cell& CharBuffer::back(std::size_t x, std::size_t y) const {
    return back_[index(x, y)];
}

std::size_t CharBuffer::diff_count() const {
    std::size_t changed = 0;
    for (std::size_t i = 0; i < back_.size(); ++i) {
        if (!(back_[i] == front_[i])) {
            ++changed;
        }
    }
    return changed;
}

void CharBuffer::swap_buffers() {
    front_.swap(back_);
}

std::string CharBuffer::emit_ansi() const {
    std::string output;
    int last_fg = -1;

    for (std::size_t y = 0; y < height_; ++y) {
        for (std::size_t x = 0; x < width_; ++x) {
            const std::size_t idx = index(x, y);
            const Cell& previous = front_[idx];
            const Cell& current = back_[idx];
            if (previous == current) {
                continue;
            }

            output += "\033[";
            output += std::to_string(y + 1);
            output += ";";
            output += std::to_string(x + 1);
            output += "H";

            if (static_cast<int>(current.fg) != last_fg) {
                output += "\033[38;5;";
                output += std::to_string(current.fg);
                output += "m";
                last_fg = static_cast<int>(current.fg);
            }

            output += current.ch;
        }
    }

    return output;
}

std::size_t CharBuffer::index(std::size_t x, std::size_t y) const {
    assert(x < width_ && y < height_ && "CharBuffer coordinate is out of range");
    return y * width_ + x;
}
