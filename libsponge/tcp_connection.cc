#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPConnection::connect() {

}

size_t TCPConnection::write(const string &data) {
  DUMMY_CODE(data);
  return {};
}

void TCPConnection::end_input_stream() {
_sender.stream_in().end_input();
}

void TCPConnection::segment_received(const TCPSegment &seg) { 
  if (_receiver.segment_received(seg)) {
    if (_receiver.ackno().has_value()) {
      // ?
    }
  }
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { 
  _time_since_last_segment_received += ms_since_last_tick;
  _sender.tick(ms_since_last_tick);
}


TCPConnection::~TCPConnection() {
  try {
    if (active()) {
      cerr << "Warning: Unclean shutdown of TCPConnection\n";

      // Your code here: need to send a RST segment to the peer
    }
  } catch (const exception &e) {
    std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
  }
}
