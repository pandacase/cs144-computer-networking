#include "byte_stream.hh"

#include <algorithm>
#include <iterator>
#include <stdexcept>

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity)
  : _stream(),
    _capacity(capacity),
    _input_ended(false),
    _written(0),
    _readed(0) { }

size_t ByteStream::write(const string &data) {
    size_t accepted = 0;
    for (size_t i = 0; i < data.length(); ++i) {
        if (remaining_capacity() > 0) {
            _stream.push_back(data[i]);
            accepted += 1;
            _written += 1;
        } else {
            break;
        }
    }
    return accepted;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    string ret;
    size_t realLen = min(len, _stream.size());
    for (size_t i = 0; i < realLen; ++i) {
        ret.push_back(_stream[i]);
    }
    return ret;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    size_t realLen = min(len, _stream.size());
    while (realLen--) {
        _stream.pop_front();
        _readed += 1;
    }
}

void ByteStream::end_input() {
    _input_ended = true;
}

bool ByteStream::input_ended() const { return _input_ended; }

size_t ByteStream::buffer_size() const { return _stream.size(); }

bool ByteStream::buffer_empty() const { return _stream.empty(); }

bool ByteStream::eof() const { return _stream.empty() && input_ended(); }

size_t ByteStream::bytes_written() const { return _written; }

size_t ByteStream::bytes_read() const { return _readed; }

size_t ByteStream::remaining_capacity() const { return (_capacity - _stream.size()); }
