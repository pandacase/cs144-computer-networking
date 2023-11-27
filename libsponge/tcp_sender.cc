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
    TCPSegment seg = TCPSegment();
    
    // sent the SYN
    if (_next_seqno == 0) {
        seg.header().syn = true;
        _timer_on = true;
        _segments_out.push(seg);
        _segments_in_flight[_next_seqno] = seg;
        _bytes_in_flight += 1;
        _next_seqno += 1;
        return;
    }

    // construct the segment to be sent
    seg.header().seqno = wrap(_next_seqno, _isn);
    size_t payload_length_to_send = min(TCPConfig::MAX_PAYLOAD_SIZE, size_t(_receiver_window_size));
    if (_stream.input_ended() && payload_length_to_send >= _stream.buffer_size()) {
        seg.header().fin = true;
    }
    payload_length_to_send = min(payload_length_to_send, _stream.buffer_size());
    string str_to_send = _stream.read(payload_length_to_send);
    seg.payload() = move(str_to_send);
    // sent the segment
    _segments_out.push(seg);
    // tag that this segment is in flight
    _segments_in_flight[_next_seqno] = seg;
    _bytes_in_flight += payload_length_to_send + seg.header().fin ? 1 : 0;
    // update the next byte to be sent
    _next_seqno += payload_length_to_send + seg.header().fin ? 1 : 0;
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
//! \returns `false` if the ackno appears invalid (acknowledges something the TCPSender hasn't sent yet)
bool TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t absolute_ackno = unwrap(ackno, _isn, _acked);
    if (absolute_ackno >= _next_seqno) {
        return {false};
    }
    
    if (absolute_ackno >= _acked) {
        _acked = absolute_ackno;
        _receiver_window_size = window_size;


        // set the RTO back to initial value
        _current_retransmission_timeout = _initial_retransmission_timeout;
        // restart the timer
        if (!_segments_in_flight.empty()) {
            _timer = 0;
        } 
        // reset the consecutive retransmission
        _consecutive_retransmission = 0;
    }

    return {true};
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
    if (_timer_on) {
        // increase the timer
        _timer += ms_since_last_tick;
        
        // check if time out
        if (_timer > _current_retransmission_timeout) { 
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
    
}
