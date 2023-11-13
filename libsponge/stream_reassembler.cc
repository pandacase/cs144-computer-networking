#include "stream_reassembler.hh"
#include <iostream>
#include <algorithm>

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : 
    _segments(),
    _unassembled_bytes(0),
    _eof_end(0),
    _output(capacity), 
    _next_unassembled(0),
    _capacity(capacity) {}


bool StreamReassembler::write_substring(ByteStream &output, size_t &next_unassembled, const string &data, const size_t index) {
    size_t sub_length = index + data.size() - next_unassembled;
    sub_length = min(sub_length, output.remaining_capacity());
    string sub_data(data, (next_unassembled - index), sub_length);
    // write into output stream
    output.write(sub_data);
    next_unassembled += sub_length;
    return true;
}

void StreamReassembler::add_and_merge_unassembled_bytes(map<size_t, string> &segments, size_t &unassembled_bytes, const string &data, const size_t index) {
    string mg_data = data;
    size_t mg_index = index;
    auto it = segments.begin();
    while (it != segments.end()) {
        string it_data = it->second;
        size_t it_index = it->first;
        if (it_index <= mg_index) {
            if (it_index + it_data.size() >= mg_index + mg_data.size()) {
                /* mg within the it */
                mg_data = it_data;
                mg_index = it_index;
                unassembled_bytes -= it_data.size();
                it = segments.erase(it);
            } else if (it_index + it_data.size() >= mg_index) {
                /* it begins, overlap with mg */
                size_t sub_length = (mg_index + mg_data.size()) - (it_index + it_data.size());
                string sub_data(mg_data, (it_index + it_data.size() - mg_index), sub_length);
                mg_data = it_data + sub_data;
                mg_index = it_index;
                unassembled_bytes -= it_data.size();
                it = segments.erase(it);
            } else {
                /* no overlap, do nothing */
                it++;
            }
        } else if (mg_index < it_index) {
            if (mg_index + mg_data.size() >= it_index + it_data.size()) {
                /* it within the mg */
                unassembled_bytes -= it_data.size();
                it = segments.erase(it);
            } else if (mg_index + mg_data.size() >= it_index) {
                /* mg begins, overlap with it */
                size_t sub_length = (it_index + it_data.size()) - (mg_index + mg_data.size());
                string sub_data(it_data, (mg_index + mg_data.size() - it_index), sub_length);
                mg_data = mg_data + sub_data;
                unassembled_bytes -= it_data.size();
                it = segments.erase(it);
            } else {
                /* no overlap, do nothing */
                it++;
            }
        }
    }

    segments[mg_index] = mg_data;
    unassembled_bytes += mg_data.size();
}


//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // corner case
    if (eof) {
        if (index <= _next_unassembled) {
            write_substring(_output, _next_unassembled, data, index);
            _output.end_input();
            return;
        }
        _eof_end = index + data.size();
    }
    
    // push the sub-string directly first
    if (index > _next_unassembled) {
        if (_unassembled_bytes + data.size() <= _capacity) {
            add_and_merge_unassembled_bytes(_segments, _unassembled_bytes, data, index);
        }
    } else if (index + data.size() > _next_unassembled) {
        write_substring(_output, _next_unassembled, data, index);
    } else {
        /* do nothing */ 
    }

    // try to push from _segments
    auto it = _segments.begin();
    while (it != _segments.end()) {
        if (it->first <= _next_unassembled && it->first + it->second.size() > _next_unassembled) {
            write_substring(_output, _next_unassembled, it->second, it->first);
            _unassembled_bytes -= it->second.size();
            it = _segments.erase(it);
        } else if (it->first + it->second.size() <= _next_unassembled) {
            _unassembled_bytes -= it->second.size();
            it = _segments.erase(it);
        } else {
            it ++;
        }
    }

    // finally check if end up to eof
    if (_eof_end != 0 && _next_unassembled == _eof_end) {
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes; }

bool StreamReassembler::empty() const { return _segments.empty(); }
