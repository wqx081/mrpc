#ifndef BASE_PICKLE_H_
#define BASE_PICKLE_H_

#include <stddef.h>
#include <stdint.h>

#include <string>

#include "base/macros.h"
#include <glog/logging.h>
#include "base/ref_counted.h"
#include "base/string_piece.h"

namespace mrpc {

class Pickle;

// PickleIterator reads data from a Pickle. The Pickle object must remain valid
// while the PickleIterator object is in use.
class  PickleIterator {
 public:
  PickleIterator() : payload_(NULL), read_index_(0), end_index_(0) {}
  explicit PickleIterator(const Pickle& pickle);

  // Methods for reading the payload of the Pickle. To read from the start of
  // the Pickle, create a PickleIterator from a Pickle. If successful, these
  // methods return true. Otherwise, false is returned to indicate that the
  // result could not be extracted. It is not possible to read from the iterator
  // after that.
  bool ReadBool(bool* result) ;
  bool ReadInt(int* result) ;
  bool ReadLong(long* result) ;
  bool ReadUInt16(uint16_t* result) ;
  bool ReadUInt32(uint32_t* result) ;
  bool ReadInt64(int64_t* result) ;
  bool ReadUInt64(uint64_t* result) ;
  bool ReadFloat(float* result) ;
  bool ReadDouble(double* result) ;
  bool ReadString(std::string* result) ;
  // The StringPiece data will only be valid for the lifetime of the message.
  bool ReadStringPiece(StringPiece* result) ;

  // A pointer to the data will be placed in |*data|, and the length will be
  // placed in |*length|. The pointer placed into |*data| points into the
  // message's buffer so it will be scoped to the lifetime of the message (or
  // until the message data is mutated). Do not keep the pointer around!
  bool ReadData(const char** data, int* length) ;

  // A pointer to the data will be placed in |*data|. The caller specifies the
  // number of bytes to read, and ReadBytes will validate this length. The
  // pointer placed into |*data| points into the message's buffer so it will be
  // scoped to the lifetime of the message (or until the message data is
  // mutated). Do not keep the pointer around!
  bool ReadBytes(const char** data, int length) ;

  // A safer version of ReadInt() that checks for the result not being negative.
  // Use it for reading the object sizes.
  bool ReadLength(int* result)  {
    return ReadInt(result) && *result >= 0;
  }

  // Skips bytes in the read buffer and returns true if there are at least
  // num_bytes available. Otherwise, does nothing and returns false.
  bool SkipBytes(int num_bytes)  {
    return !!GetReadPointerAndAdvance(num_bytes);
  }

 private:
  // Read Type from Pickle.
  template <typename Type>
  bool ReadBuiltinType(Type* result);

  // Advance read_index_ but do not allow it to exceed end_index_.
  // Keeps read_index_ aligned.
  void Advance(size_t size);

  // Get read pointer for Type and advance read pointer.
  template<typename Type>
  const char* GetReadPointerAndAdvance();

  // Get read pointer for |num_bytes| and advance read pointer. This method
  // checks num_bytes for negativity and wrapping.
  const char* GetReadPointerAndAdvance(int num_bytes);

  // Get read pointer for (num_elements * size_element) bytes and advance read
  // pointer. This method checks for int overflow, negativity and wrapping.
  const char* GetReadPointerAndAdvance(int num_elements,
                                       size_t size_element);

  const char* payload_;  // Start of our pickle's payload.
  size_t read_index_;  // Offset of the next readable byte in payload.
  size_t end_index_;  // Payload size.

  //FRIEND_TEST_ALL_PREFIXES(PickleTest, GetReadPointerAndAdvance);
};

// This class provides an interface analogous to Pickle's WriteFoo()
// methods and can be used to accurately compute the size of a hypothetical
// Pickle's payload without having to reference the Pickle implementation.
class  PickleSizer {
 public:
  PickleSizer();
  ~PickleSizer();

  // Returns the computed size of the payload.
  size_t payload_size() const { return payload_size_; }

  void AddBool() { return AddInt(); }
  void AddInt() { AddPOD<int>(); }
  void AddLong() { AddPOD<uint64_t>(); }
  void AddUInt16() { return AddPOD<uint16_t>(); }
  void AddUInt32() { return AddPOD<uint32_t>(); }
  void AddInt64() { return AddPOD<int64_t>(); }
  void AddUInt64() { return AddPOD<uint64_t>(); }
  void AddFloat() { return AddPOD<float>(); }
  void AddDouble() { return AddPOD<double>(); }
  void AddString(const StringPiece& value);
  void AddData(int length);
  void AddBytes(int length);

 private:
  // Just like AddBytes() but with a compile-time size for performance.
  template<size_t length> void  AddBytesStatic();

  template <typename T>
  void AddPOD() { AddBytesStatic<sizeof(T)>(); }

  size_t payload_size_ = 0;
};

class  Pickle {
 public:
  class  Attachment : public RefCountedThreadSafe<Attachment> {
   public:
    Attachment();

   protected:
    friend class RefCountedThreadSafe<Attachment>;
    virtual ~Attachment();

    DISALLOW_COPY_AND_ASSIGN(Attachment);
  };

  Pickle();
  explicit Pickle(int header_size);
  Pickle(const char* data, int data_len);
  Pickle(const Pickle& other);
  virtual ~Pickle();
  Pickle& operator=(const Pickle& other);
  size_t size() const { return header_size_ + header_->payload_size; }
  const void* data() const { return header_; }
  size_t GetTotalAllocatedSize() const;
  bool WriteBool(bool value) {
    return WriteInt(value ? 1 : 0);
  }
  bool WriteInt(int value) {
    return WritePOD(value);
  }
  bool WriteLong(long value) {
    // Always write long as a 64-bit value to ensure compatibility between
    // 32-bit and 64-bit processes.
    return WritePOD(static_cast<int64_t>(value));
  }
  bool WriteUInt16(uint16_t value) { return WritePOD(value); }
  bool WriteUInt32(uint32_t value) { return WritePOD(value); }
  bool WriteInt64(int64_t value) { return WritePOD(value); }
  bool WriteUInt64(uint64_t value) { return WritePOD(value); }
  bool WriteFloat(float value) {
    return WritePOD(value);
  }
  bool WriteDouble(double value) {
    return WritePOD(value);
  }
  bool WriteString(const StringPiece& value);
  bool WriteData(const char* data, int length);
  bool WriteBytes(const void* data, int length);

  // WriteAttachment appends |attachment| to the pickle. It returns
  // false iff the set is full or if the Pickle implementation does not support
  // attachments.
  virtual bool WriteAttachment(scoped_refptr<Attachment> attachment);

  // ReadAttachment parses an attachment given the parsing state |iter| and
  // writes it to |*attachment|. It returns true on success.
  virtual bool ReadAttachment(PickleIterator* iter,
                              scoped_refptr<Attachment>* attachment) const;

  // Indicates whether the pickle has any attachments.
  virtual bool HasAttachments() const;

  // Reserves space for upcoming writes when multiple writes will be made and
  // their sizes are computed in advance. It can be significantly faster to call
  // Reserve() before calling WriteFoo() multiple times.
  void Reserve(size_t additional_capacity);

  // Payload follows after allocation of Header (header size is customizable).
  struct Header {
    uint32_t payload_size;  // Specifies the size of the payload.
  };

  // Returns the header, cast to a user-specified type T.  The type T must be a
  // subclass of Header and its size must correspond to the header_size passed
  // to the Pickle constructor.
  template <class T>
  T* headerT() {
    DCHECK_EQ(header_size_, sizeof(T));
    return static_cast<T*>(header_);
  }
  template <class T>
  const T* headerT() const {
    DCHECK_EQ(header_size_, sizeof(T));
    return static_cast<const T*>(header_);
  }

  // The payload is the pickle data immediately following the header.
  size_t payload_size() const {
    return header_ ? header_->payload_size : 0;
  }

  const char* payload() const {
    return reinterpret_cast<const char*>(header_) + header_size_;
  }

  // Returns the address of the byte immediately following the currently valid
  // header + payload.
  const char* end_of_payload() const {
    // This object may be invalid.
    return header_ ? payload() + payload_size() : NULL;
  }

 protected:
  char* mutable_payload() {
    return reinterpret_cast<char*>(header_) + header_size_;
  }

  size_t capacity_after_header() const {
    return capacity_after_header_;
  }

  // Resize the capacity, note that the input value should not include the size
  // of the header.
  void Resize(size_t new_capacity);

  // Claims |num_bytes| bytes of payload. This is similar to Reserve() in that
  // it may grow the capacity, but it also advances the write offset of the
  // pickle by |num_bytes|. Claimed memory, including padding, is zeroed.
  //
  // Returns the address of the first byte claimed.
  void* ClaimBytes(size_t num_bytes);

  // Find the end of the pickled data that starts at range_start.  Returns NULL
  // if the entire Pickle is not found in the given data range.
  static const char* FindNext(size_t header_size,
                              const char* range_start,
                              const char* range_end);

  // Parse pickle header and return total size of the pickle. Data range
  // doesn't need to contain entire pickle.
  // Returns true if pickle header was found and parsed. Callers must check
  // returned |pickle_size| for sanity (against maximum message size, etc).
  // NOTE: when function successfully parses a header, but encounters an
  // overflow during pickle size calculation, it sets |pickle_size| to the
  // maximum size_t value and returns true.
  static bool PeekNext(size_t header_size,
                       const char* range_start,
                       const char* range_end,
                       size_t* pickle_size);

  // The allocation granularity of the payload.
  static const int kPayloadUnit;

 private:
  friend class PickleIterator;

  Header* header_;
  size_t header_size_;  // Supports extra data between header and payload.
  // Allocation size of payload (or -1 if allocation is const). Note: this
  // doesn't count the header.
  size_t capacity_after_header_;
  // The offset at which we will write the next field. Note: this doesn't count
  // the header.
  size_t write_offset_;

  // Just like WriteBytes, but with a compile-time size, for performance.
  template<size_t length> void  WriteBytesStatic(const void* data);

  // Writes a POD by copying its bytes.
  template <typename T> bool WritePOD(const T& data) {
    WriteBytesStatic<sizeof(data)>(&data);
    return true;
  }

  inline void* ClaimUninitializedBytesInternal(size_t num_bytes);
  inline void WriteBytesCommon(const void* data, size_t length);
};

}  // namespace base

#endif  // BASE_PICKLE_H_
