#include "CharBuffer.h"

#include <algorithm>
#include <cassert>

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

std::size_t CharBuffer::index(std::size_t x, std::size_t y) const {
    assert(x < width_ && y < height_ && "CharBuffer coordinate is out of range");
    return y * width_ + x;
}
