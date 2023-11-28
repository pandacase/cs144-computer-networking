#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>
#include <algorithm>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

// It will be your TCPSender’s responsibility to:
// • Keep track of the receiver’s window (processing incoming acknos and window sizes)
// • Fill the window when possible, by reading from the ByteStream, creating new TCP
// segments (including SYN and FIN flags if needed), and sending them
// • Keep track of which segments have been sent but not yet acknowledged by the receiver—
// we call these “outstanding” segments
// • Re-send outstanding segments if enough time passes since they were sent, and they
// haven’t been acknowledged yet

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout(retx_timeout)
    , _stream(capacity)
    , _current_retransmission_timeout(retx_timeout) {}

uint64_t TCPSender::bytes_in_flight() const { return {_bytes_in_flight}; }

void TCPSender::fill_window() {
    // while `first time come in` or `win > 0`:
    while (!_syn_sent || _receiver_window_size > 0) {
        TCPSegment seg = TCPSegment();

        // Construct the segment
        seg.header().seqno = wrap(_next_seqno, _isn);
        // - set the syn
        if (_next_seqno == 0) {
            seg.header().syn = true;
            _syn_sent = true;
        }
        size_t payload_length_to_send = min(TCPConfig::MAX_PAYLOAD_SIZE, size_t(_receiver_window_size));
        // - set the payload
        payload_length_to_send = min(payload_length_to_send, _stream.buffer_size());
        string str_to_send = _stream.read(payload_length_to_send);
        seg.payload() = move(str_to_send);
        size_t length_to_send = seg.length_in_sequence_space();
        // - set the fin
        if (_stream.eof() && length_to_send < _receiver_window_size) { 
            seg.header().fin = true;    // why `<` but not `<=` ?
            _fin_sent = true;           // `fin` is about to be set.
        }
        
        
        if (length_to_send != 0) {
            // sent the segment
            _segments_out.push(seg);
            // tag that this segment is in flight
            _segments_in_flight[_next_seqno] = seg;
            _bytes_in_flight += length_to_send;
            // update the next byte to be sent and the window size
            _next_seqno += length_to_send;
            _receiver_window_size -= length_to_send;
            // trun on the timer
            _timer_on = true;
        }
        
        // if no more 
        if (_stream.buffer_size() == 0) {
            break;
        }
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
//! \returns `false` if the ackno appears invalid (acknowledges something the TCPSender hasn't sent yet)
bool TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t absolute_ackno = unwrap(ackno, _isn, _acked);
    if (absolute_ackno > _next_seqno) {
        return {false};
    } else if (!_syn_sent) {
        return {false};
    } else if (_fin_received) {
        return {true};
    }
    
    if (absolute_ackno >= _acked) {
        _acked = absolute_ackno;
        _receiver_window_size = window_size;
        // update the segments in flight
        auto it = _segments_in_flight.begin();
        while(it != _segments_in_flight.end()) {
            if (it->first < absolute_ackno) {
                // removed from the segments in flight
                _bytes_in_flight -= it->second.length_in_sequence_space();
                it = _segments_in_flight.erase(it);
            } else {
                break;
            }
        }

        // set the RTO back to initial value
        _current_retransmission_timeout = _initial_retransmission_timeout;
        // restart the timer
        _timer = 0;
        if (!_segments_in_flight.empty()) {
            _timer_on = true;
        } else {
            _timer_on = false;
            if (_fin_sent) {
                _fin_received = true;
            }
        }
        // reset the consecutive retransmission
        _consecutive_retransmission = 0;
    }

    fill_window();
    return {true};
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
    if (_timer_on) {
        // increase the timer
        _timer += ms_since_last_tick;
        
        // check if time out
        if (_timer >= _current_retransmission_timeout) { 
            // retransmission
            if (!_segments_in_flight.empty()) {
                auto first_segment = _segments_in_flight.begin();
                _segments_out.push(first_segment->second);
            }

            if (_receiver_window_size != 0) {
                // keep track of the number of consecutive retransmissions:
                _consecutive_retransmission += 1;
                // Double the RTO:
                _current_retransmission_timeout *= 2;
                // start the retransmission timer:
                _timer = 0;
            }
        }
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return {_consecutive_retransmission}; }

void TCPSender::send_empty_segment() {
    TCPSegment seg = TCPSegment();
    _segments_out.push(seg);
}
