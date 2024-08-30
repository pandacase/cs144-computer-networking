#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPConnection::connect() {
  _sender.fill_window();
  send_pending_segments();
}


size_t TCPConnection::write(const string &data) {
  if (data.empty()) return 0;
  // 1. write data into _sender._stream
  size_t n_bytes = _sender.stream_in().write(data);
  // 2. read from _sender._stream, push into _sender._segments_out
  _sender.fill_window();
  // 3. pop from _sender._segments_out, push into this->_segments_out
  send_pending_segments();
  return n_bytes;
}

void TCPConnection::end_input_stream() {
  _sender.stream_in().end_input();
  _sender.fill_window();
  send_pending_segments();
}

void TCPConnection::segment_received(const TCPSegment &seg) { 
  // clear timer
  _time_since_last_segment_received = 0;

  // check if the RST has been set
  if (seg.header().rst) {
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
    _active = false;
    return;
  }

  // pass the new segment to receiver
  _receiver.segment_received(seg);

  // check if need to linger
  if (check_inbound_ended() && !_sender.stream_in().eof()) {
    _linger_after_streams_finish = false;
  }
  
  // check if the ACK has been set
  if (seg.header().ack) {
    _sender.ack_received(seg.header().ackno, seg.header().win);
    send_pending_segments();
  }

  // send ack
  if (seg.length_in_sequence_space() > 0) {
    _sender.fill_window();
    bool is_sent = send_pending_segments();
    // if not sent, send at least a ACK
    if (!is_sent) {
      _sender.send_empty_segment();
      send_pending_segments();
    }
  }
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { 
  // recording time
  _time_since_last_segment_received += ms_since_last_tick;
  // tick the _sender for retransmition
  _sender.tick(ms_since_last_tick);

  // if new retransmit segment generated, send it
  if (!_sender.segments_out().empty()) {
    auto retxSeg = _sender.segments_out().front();
    _sender.segments_out().pop();
    set_ack_and_win(retxSeg);
    // abort the connection
    if (_sender.consecutive_retransmissions() > _cfg.MAX_RETX_ATTEMPTS) {
      _sender.stream_in().set_error();
      _receiver.stream_out().set_error();
      retxSeg.header().rst = true;
      _active = false;
    }
    _segments_out.push(retxSeg);
  }

    // check if done
    if (check_inbound_ended() && check_outbound_ended()) {
      if (!_linger_after_streams_finish) {
        _active = false;
      } else if (_time_since_last_segment_received >= 10 * _cfg.rt_timeout) {
        _active = false;
      }
    }
}

TCPConnection::~TCPConnection() {
  try {
    if (active()) {
      cerr << "Warning: Unclean shutdown of TCPConnection\n";
      // Your code here: need to send a RST segment to the peer
      _sender.stream_in().set_error();
      _receiver.stream_out().set_error();
      send_rst();
      _active = false;

    }
  } catch (const exception &e) {
    std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
  }
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////

bool TCPConnection::send_pending_segments() {
  bool is_sent = false;
  while (!_sender.segments_out().empty()) {
    auto segment = _sender.segments_out().front();
    _sender.segments_out().pop();
    set_ack_and_win(segment);
    _segments_out.push(segment);
    is_sent = true;
  }
  return is_sent;
}

void TCPConnection::set_ack_and_win(TCPSegment &seg) {
  // ask receiver for ack and window size
  auto ackno = _receiver.ackno();
  if (ackno.has_value()) {
    seg.header().ack = true;
    seg.header().ackno = ackno.value();
  }
  size_t win_size = _receiver.window_size();
  seg.header().win = static_cast<uint16_t>(win_size);
}

void TCPConnection::send_rst() {
  // generate a empty segment into _sender's queue
  _sender.send_empty_segment();
  // get the empty segment
  auto rst_seg = _sender.segments_out().front();
  _sender.segments_out().pop();
  // set ack & win & rst
  set_ack_and_win(rst_seg);
  rst_seg.header().rst = true;
  // send it
  _segments_out.push(rst_seg);
}

// prereqs1 : The inbound stream has been fully assembled and has ended.
bool TCPConnection::check_inbound_ended() {
  return _receiver.unassembled_bytes() == 0 && 
         _receiver.stream_out().input_ended();
}
// prereqs2 : The outbound stream has been ended by the local application and fully sent (including
// the fact that it ended, i.e. a segment with fin ) to the remote peer.
// prereqs3 : The outbound stream has been fully acknowledged by the remote peer.
bool TCPConnection::check_outbound_ended() {
  return _sender.stream_in().eof() &&
         _sender.next_seqno_absolute() == _sender.stream_in().bytes_written() + 2 &&
         _sender.bytes_in_flight() == 0;
}

