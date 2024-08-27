#ifndef SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
#define SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

#include "byte_stream.hh"

#include <cstdint>
#include <string>
#include <map>

//! \brief A class that assembles a series of excerpts from a byte stream (possibly out 
//! of order, possibly overlapping) into an in-order byte stream.
class StreamReassembler {
  private:
    // Your code here -- add private members as necessary.
    std::map<size_t, std::string> _segments;  //!< The set for unassembled segments
    size_t _unassembled_bytes;  //!< Cnt for unassembled bytes
    size_t _eof_end;     //!< The index of eof
    ByteStream _output;  //!< The reassembled in-order byte stream
    size_t _next_unassembled;  //!< The next unassembled byte index
    size_t _capacity;    //!< The maximum number of bytes

  public:
    //! \brief Construct a `StreamReassembler` that will store up to `capacity` bytes.
    //! \note This capacity limits both the bytes that have been reassembled,
    //! and those that have not yet been reassembled.
    StreamReassembler(const size_t capacity);

    //! \brief Receives a substring and writes any newly contiguous bytes into the stream.
    //!
    //! If accepting all the data would overflow the `capacity` of this
    //! `StreamReassembler`, then only the part of the data that fits will be
    //! accepted. If the substring is only partially accepted, then the `eof`
    //! will be disregarded.
    //!
    //! \param data the string being added
    //! \param index the index of the first byte in `data`
    //! \param eof whether or not this segment ends with the end of the stream
    void push_substring(const std::string &data, const uint64_t index, const bool eof);

    //! \name Access the reassembled byte stream
    //!@{
    const ByteStream &stream_out() const { return _output; }
    ByteStream &stream_out() { return _output; }
    //!@}

    //! The number of bytes in the substrings stored but not yet reassembled
    //!
    //! \note If the byte at a particular index has been submitted twice, it
    //! should only be counted once for the purpose of this function.
    size_t unassembled_bytes() const { return _unassembled_bytes; }

    //! \brief Is the internal state empty (other than the output stream)?
    //! \returns `true` if no substrings are waiting to be assembled
    bool empty() const { return _segments.empty(); }

    //! @brief The next index the stream is waiting for
    size_t next_unassembled() const { return _next_unassembled; }

  private:
      //! @brief An aux func of [StreamReassembler::push_substring] : 
      //! write a sub string into _output stream
      //! 
      //! @param _output The output byte stream
      //! @param _next_unassembled The next unassembled byte index
      //! @param data The new input string
      //! @param index The index of first byte of data
      //! @return true if successfully write
      //! @return false (else)
      bool write_substring(
        ByteStream &output, 
        size_t &next_unassembled, 
        const std::string &data, 
        const size_t index
      );

      //! @brief An aux func of [StreamReassembler::push_substring] : Add a string into 
      //! _segments set
      //! 
      //! @param _segments The set of all string that are unable to write yet (unassembled)
      //! @param _unassembled_bytes The number of unassembled bytes
      //! @param data The new input string
      //! @param index The index of first byte of data
      void add_and_merge_unassembled_bytes(
        std::map<size_t, std::string> &segments, 
        size_t &unassembled_bytes, 
        const std::string &data, 
        const size_t index
      );
};

#endif  // SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
