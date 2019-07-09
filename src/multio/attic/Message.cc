
#include "Message.h"

#include <cstring>

#include "eckit/log/Bytes.h"
#include "eckit/log/Log.h"
#include "eckit/log/ResourceUsage.h"
#include "eckit/maths/Functions.h"

#include "multio/attic/print_buffer.h"

namespace multio {
namespace attic {

Message::Message(size_t size, int peer, int tag) :
    buffer_(size),
    peer_(peer),
    tag_(tag),
    position_(0) {}

void Message::rewind() const {
    position_ = 0;
}

void* Message::data() {
    return buffer_.data();
}

const void* Message::data() const {
    return buffer_.data();
}

size_t Message::size() const {
    return buffer_.size();
}

int Message::tag() const {
    return tag_;
}

int Message::peer() const {
    ASSERT(peer_ >= 0);
    return peer_;
}

void Message::reset(size_t size, int peer, int tag) {
    resize(size);

    peer_ = peer;
    tag_  = tag;

    position_ = 0;
}

void Message::peer(const int new_peer) {
    peer_ = new_peer;
}

void Message::resize(size_t new_cap) {
    buffer_.resize(new_cap);
    buffer_.shrink_to_fit();
}

size_t Message::read(void* buffer, size_t length) const {

    size_t left = buffer_.size() - position_;
    size_t size = std::min(left, length);
    std::memcpy(buffer, buffer_.data() + position_, size);
    position_ += size;
    return size;
}

size_t Message::write(const void* buffer, size_t length) {
    if (position_ + length > buffer_.size()) {
        size_t newsize = position_ + length;
        buffer_.resize(newsize);
        eckit::Log::info() << "Message::write() resizing buffer to " << eckit::Bytes(newsize)
                           << std::endl;
    }

    size_t left = buffer_.size() - position_;
    size_t size = std::min(left, size_t(length));
    std::memcpy(buffer_.data() + position_, buffer, size);

    if (size != size_t(length)) {
        std::ostringstream oss;
        oss << "Attempt to write " << length << " bytes on message, could only write " << size
            << ", buffer is " << buffer_.size();
        throw eckit::SeriousBug(oss.str());
    }

    position_ += size;
    return size;
}

std::string Message::name() const {
    return "Message";
}

void Message::print(std::ostream& out) const {
    out << "Message(tag = " << tag_ << ", peer = " << peer_ << ", position = " << position_
        << ", buffer = ";
        print_buffer(buffer_, out, "");
        out << ")" << std::endl;
}

}  // namespace attic
}  // namespace multio