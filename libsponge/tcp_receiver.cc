#include "tcp_receiver.hh"
#include <algorithm>

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

TCPReceiver::TCPReceiver(const size_t capacity) : 
    _reassembler(capacity), 
    _capacity(capacity), 
    _syn_flag(false),
    _fin_flag(false),
    _isn(0) {}

//! @brief handle an inbound segment
//! @return true if any part of seg is handled
//! 
//! @details :
//! 1. If the ISN hasn’t been set yet, a segment is acceptable if (and only if) it has the SYN bit set.
//! 2. If the window has size zero, then its size should be treated as one byte for the 
//!    purpose of determining the first and last byte of the window.
//! 3. If the segment’s length in sequence space is zero (e.g. just a bare acknowledgment
//!    with no payload and no SYN or FIN flag), then its size should be treated as one
//!    byte for the purpose of determining the first and last sequence number it occupies.
bool TCPReceiver::segment_received(const TCPSegment &seg) {
    // In tcp, syn arrive with no payload for establish connection.
    // So parse first SYN alone here:
    if (!_syn_flag) {
        if (!seg.header().syn) {
            return false;
        } else {
            _syn_flag = true;
            _isn = seg.header().seqno;
            if (seg.header().fin) {
                _reassembler.stream_out().end_input();
            }
            return true;
        }
    } else { // After started, reject second SYN
        if (seg.header().syn) {
            return false;
        }
    }

    // parse progress info
    bool first_fin = false;
    if (!_fin_flag && seg.header().fin) {
        _fin_flag = true;
        first_fin = true;
    }
    uint64_t checkpoint = _reassembler.next_unassembled();
    uint64_t index = unwrap(seg.header().seqno, _isn, checkpoint) - 1;
    uint64_t real_index = max(checkpoint, index);
    int length = seg.length_in_sequence_space() - (seg.header().syn ? 1 : 0) - (seg.header().fin ? 1 : 0);
    int real_length = min(index + length, checkpoint + window_size()) - real_index;
    
    // push substring
    if (real_length > 0) {
        string data = seg.payload().copy().substr(real_index - index, real_length);
        _reassembler.push_substring(data, real_index, _fin_flag);
        return true;
    } else if (real_length == 0 && first_fin) {
        _reassembler.push_substring(string(""), checkpoint, true);
        return true;
    } else {
        return false;   
    }
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if (!_syn_flag) {
        return {nullopt};
    } else {
        return wrap(_reassembler.next_unassembled() + (_syn_flag ? 1 : 0) + (_reassembler.stream_out().input_ended() ? 1 : 0), _isn);
    }
}

size_t TCPReceiver::window_size() const { return {stream_out().remaining_capacity()}; }
