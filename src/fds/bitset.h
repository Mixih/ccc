#ifndef BITFIELD_H
#define BITFIELD_H

#include <cstdint>

class Bitset {
private:
    class Reference;
    uint64_t* data;
    std::size_t numElems;
    std::size_t bits;

public:
    Bitset(std::size_t bits);
    ~Bitset();
    Bitset(const Bitset& field);
    Bitset(Bitset&& field) noexcept;
    Bitset& operator=(Bitset&& field) noexcept;

    bool operator==(const Bitset& other) const;
    bool all() const;

    Bitset operator~() const;
    Bitset operator&(const Bitset& b) const;
    Bitset operator|(const Bitset& b) const;
    Bitset operator^(const Bitset& b) const;
    Bitset& operator&=(const Bitset& b);
    Bitset& operator|=(const Bitset& b);
    Bitset& operator^=(const Bitset& b);

    bool operator[](std::size_t bit) const;
    Reference operator[](std::size_t bit);

    Bitset& set(std::size_t bit) noexcept;
    Bitset& clr(std::size_t bit) noexcept;
    Bitset& flip(std::size_t bit) noexcept;

    std::size_t size() const noexcept;
};

class Bitset::Reference {
private:
    Bitset* bitset;
    std::size_t elem;
    unsigned bit;

    Reference(Bitset* bitset, std::size_t elem, unsigned bit);
    friend class Bitset;

public:
    Reference& operator=(const Reference&) = delete;
    Reference& operator=(bool val) noexcept;
    operator bool() const noexcept;
    bool operator~() const noexcept;
    Reference& flip() noexcept;
};

///////////////////////////////////////////
// Implement inline operators for speed
///////////////////////////////////////////
inline bool Bitset::operator[](std::size_t bit) const {
    std::size_t offset = bit / 64;
    unsigned shift = bit % 64;
    return (data[offset] & (1UL << shift)) >> shift;
}

inline Bitset& Bitset::set(std::size_t bit) noexcept {
    std::size_t offset = bit / 64;
    unsigned shift = bit % 64;
    data[offset] |= (1UL << shift);
    return *this;
}

inline Bitset& Bitset::clr(std::size_t bit) noexcept {
    std::size_t offset = bit / 64;
    unsigned shift = bit % 64;
    data[offset] &= ~(1UL << shift);
    return *this;
}

inline Bitset& Bitset::flip(std::size_t bit) noexcept {
    std::size_t offset = bit / 64;
    unsigned shift = bit % 64;
    data[offset] ^= (1UL << shift);
    return *this;
}

inline std::size_t Bitset::size() const noexcept {
    return bits;
}

inline Bitset::Reference Bitset::operator[](std::size_t bit) {
    std::size_t offset = bit / 64;
    unsigned shift = bit % 64;
    return {this, offset, shift};
}

inline Bitset::Reference::Reference(Bitset* bitset, std::size_t elem, unsigned bit)
    : bitset{bitset}, elem{elem}, bit{bit} {
}

inline Bitset::Reference::operator bool() const noexcept {
    return (bitset->data[elem] & (1UL << bit)) >> bit;
}

inline bool Bitset::Reference::operator~() const noexcept {
    return ~((bitset->data[elem] & (1UL << bit)) >> bit);
}

inline Bitset::Reference& Bitset::Reference::flip() noexcept {
    bitset->flip(elem * 64 + bit);
    return *this;
}

inline Bitset::Reference& Bitset::Reference::operator=(bool val) noexcept {
    if (val) {
        bitset->set(elem * 64 + bit);
    } else {
        bitset->clr(elem * 64 + bit);
    }
    return *this;
}

#endif // BITFIELD_H
