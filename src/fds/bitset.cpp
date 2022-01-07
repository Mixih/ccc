#include "bitset.h"

#include <cstring>
#include <stdexcept>

#define UDIV_CEIL(a, b) ((a / b) + (a % b != 0))

Bitset::Bitset(std::size_t bits) : bits{bits} {
    numElems = UDIV_CEIL(bits, 64);
    data = new uint64_t[numElems];
    memset(data, 0, sizeof(uint64_t) * numElems);
}

Bitset::~Bitset() {
    delete[] data;
}

Bitset::Bitset(const Bitset& field) : numElems{field.numElems}, bits{field.bits} {
    data = new uint64_t[numElems];
    std::memcpy(data, field.data, field.numElems * sizeof(uint64_t));
}

Bitset::Bitset(Bitset&& field) noexcept : numElems{field.numElems}, bits{field.bits} {
    data = field.data;
    // for safety!
    field.data = nullptr;
    field.numElems = 0;
    field.bits = 0;
}

Bitset& Bitset::operator=(Bitset&& field) noexcept {
    // self-assignment check
    if (this == &field)
        return *this;
    delete data;
    numElems = field.numElems;
    bits = field.bits;
    data = field.data;
    // for safety!
    field.data = nullptr;
    field.numElems = 0;
    field.bits = 0;
    return *this;
}

bool Bitset::operator==(const Bitset& other) const {
    if (numElems != other.numElems || bits != other.bits) {
        return false;
    }
    for (size_t i = 0; i < numElems; ++i) {
        if (data[i] != other.data[i]) {
            return false;
        }
    }
    return true;
}

bool Bitset::all() const {
    uint64_t rem = bits % 64UL;
    uint64_t end = bits == 0 ? numElems : numElems - 1;
    for (size_t i = 0; i < end; ++i) {
        if (!data[i]) {
            return false;
        }
    }
    uint64_t mask = rem ? 1 << (rem - 1) : 0;
    while (mask) {
        if (!(data[end] & mask)) {
            return false;
        }
        mask >>= 1;
    }
    return true;
}

Bitset Bitset::operator~() const {
    Bitset newField(bits);
    for (size_t i = 0; i < numElems; ++i) {
        newField.data[i] = ~data[i];
    }
    return newField;
}

Bitset& Bitset::operator&=(const Bitset& b) {
    if (numElems != b.numElems || bits != b.bits) {
        throw std::length_error("Bitsets must match for binary operator");
    }
    for (size_t i = 0; i < numElems; ++i) {
        data[i] = data[i] & b.data[i];
    }
    return *this;
}
Bitset& Bitset::operator|=(const Bitset& b) {
    if (numElems != b.numElems || bits != b.bits) {
        throw std::length_error("Bitsets must match for binary operator");
    }
    for (size_t i = 0; i < numElems; ++i) {
        data[i] = data[i] | b.data[i];
    }
    return *this;
}
Bitset& Bitset::operator^=(const Bitset& b) {
    if (numElems != b.numElems || bits != b.bits) {
        throw std::length_error("Bitsets must match for binary operator");
    }
    for (size_t i = 0; i < numElems; ++i) {
        data[i] = data[i] ^ b.data[i];
    }
    return *this;
}

Bitset Bitset::operator&(const Bitset& b) const {
    Bitset newField(b);
    newField &= *this;
    return newField;
}
Bitset Bitset::operator|(const Bitset& b) const {
    Bitset newField(b);
    newField |= *this;
    return newField;
}
Bitset Bitset::operator^(const Bitset& b) const {
    Bitset newField(b);
    newField ^= *this;
    return newField;
}