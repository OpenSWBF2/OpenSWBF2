#ifndef GF_UTF8_H
#define GF_UTF8_H

#include <exception>
#include <iterator>

namespace utf8
{
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
// SYNTHETIC: SWBF2 0x005d4a20
// utf8::exception::`scalar deleting destructor'
// SYNTHETIC: SWBF2 0x005d8610 SYMBOL
// ??0exception@utf8@@QAE@ABV01@@Z
// utf8::exception::exception(class utf8::exception const &)

// VTABLE: SWBF2 0x007aba68
class exception : public std::exception
{
};

// SYNTHETIC: SWBF2 0x005d8a10
// utf8::not_enough_room::`scalar deleting destructor'
// SYNTHETIC: SWBF2 0x005d85f0 SYMBOL
// ??0not_enough_room@utf8@@QAE@ABV01@@Z
// utf8::not_enough_room::not_enough_room(class utf8::not_enough_room const &)

// VTABLE: SWBF2 0x007ab9a0
class not_enough_room : public exception
{
public:
    // FUNCTION: SWBF2 0x005d4aa0
    virtual const char* what() const throw() { return "Not enough space"; }
};

// SYNTHETIC: SWBF2 0x005d4a60
// utf8::invalid_utf8::`scalar deleting destructor'
// SYNTHETIC: SWBF2 0x005d8630 SYMBOL
// ??0invalid_utf8@utf8@@QAE@ABV01@@Z
// utf8::invalid_utf8::invalid_utf8(class utf8::invalid_utf8 const &)

// VTABLE: SWBF2 0x007ab994
class invalid_utf8 : public exception
{
    uint8_t u8;

public:
    invalid_utf8(uint8_t u) : u8(u) {}
    // FUNCTION: SWBF2 0x005d4a50
    virtual const char* what() const throw() { return "Invalid UTF-8"; }
    uint8_t utf8_octet() const { return u8; }
};

// SYNTHETIC: SWBF2 0x005d49e0
// utf8::invalid_code_point::`scalar deleting destructor'
// SYNTHETIC: SWBF2 0x005d8660 SYMBOL
// ??0invalid_code_point@utf8@@QAE@ABV01@@Z
// utf8::invalid_code_point::invalid_code_point(class utf8::invalid_code_point const &)

// VTABLE: SWBF2 0x007ab970
class invalid_code_point : public exception
{
    uint32_t cp;

public:
    invalid_code_point(uint32_t cp) : cp(cp) {}
    // FUNCTION: SWBF2 0x005d49d0
    virtual const char* what() const throw() { return "Invalid code point"; }
    uint32_t code_point() const { return cp; }
};

namespace internal
{
enum utf_error
{
    UTF8_OK,
    NOT_ENOUGH_ROOM,
    INVALID_LEAD,
    INCOMPLETE_SEQUENCE,
    OVERLONG_SEQUENCE,
    INVALID_CODE_POINT
};

template <typename octet_type>
inline uint8_t mask8(octet_type oc)
{
    return static_cast<uint8_t>(0xff & oc);
}

template <typename octet_type>
inline bool is_trail(octet_type oc)
{
    return (mask8(oc) >> 6) == 0x2;
}

template <typename u32>
inline bool is_surrogate(u32 cp)
{
    return cp >= 0xd800u && cp <= 0xdfffu;
}

template <typename u32>
inline bool is_code_point_valid(u32 cp)
{
    return cp <= 0x0010ffffu && !is_surrogate(cp);
}

template <typename octet_iterator>
inline typename std::iterator_traits<octet_iterator>::difference_type
sequence_length(octet_iterator lead_it)
{
    uint8_t lead = mask8(*lead_it);
    if (lead < 0x80)
    {
        return 1;
    }
    else if ((lead >> 5) == 0x6)
    {
        return 2;
    }
    else if ((lead >> 4) == 0xe)
    {
        return 3;
    }
    else if ((lead >> 3) == 0x1e)
    {
        return 4;
    }
    else
    {
        return 0;
    }
}

template <typename octet_difference_type>
inline bool is_overlong_sequence(uint32_t cp, octet_difference_type length)
{
    if (cp < 0x80)
    {
        if (length != 1)
        {
            return true;
        }
    }
    else if (cp < 0x800)
    {
        if (length != 2)
        {
            return true;
        }
    }
    else if (cp < 0x10000)
    {
        if (length != 3)
        {
            return true;
        }
    }
    return false;
}

template <typename octet_iterator>
utf_error increase_safely(octet_iterator& it, octet_iterator end)
{
    if (++it == end)
    {
        return NOT_ENOUGH_ROOM;
    }
    if (!is_trail(*it))
    {
        return INCOMPLETE_SEQUENCE;
    }
    return UTF8_OK;
}

#define UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR(IT, END) \
    { utf_error ret = increase_safely(IT, END); if (ret != UTF8_OK) return ret; }

template <typename octet_iterator>
utf_error get_sequence_1(octet_iterator& it, octet_iterator end, uint32_t& code_point)
{
    if (it == end)
    {
        return NOT_ENOUGH_ROOM;
    }
    code_point = mask8(*it);
    return UTF8_OK;
}

template <typename octet_iterator>
utf_error get_sequence_2(octet_iterator& it, octet_iterator end, uint32_t& code_point)
{
    if (it == end)
    {
        return NOT_ENOUGH_ROOM;
    }
    code_point = mask8(*it);
    UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR(it, end)
    code_point = ((code_point << 6) & 0x7ff) + ((*it) & 0x3f);
    return UTF8_OK;
}

template <typename octet_iterator>
utf_error get_sequence_3(octet_iterator& it, octet_iterator end, uint32_t& code_point)
{
    if (it == end)
    {
        return NOT_ENOUGH_ROOM;
    }
    code_point = mask8(*it);
    UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR(it, end)
    code_point = ((code_point << 12) & 0xffff) + ((mask8(*it) << 6) & 0xfff);
    UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR(it, end)
    code_point += (*it) & 0x3f;
    return UTF8_OK;
}

template <typename octet_iterator>
utf_error get_sequence_4(octet_iterator& it, octet_iterator end, uint32_t& code_point)
{
    if (it == end)
    {
        return NOT_ENOUGH_ROOM;
    }
    code_point = mask8(*it);
    UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR(it, end)
    code_point = ((code_point << 18) & 0x1fffff) + ((mask8(*it) << 12) & 0x3ffff);
    UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR(it, end)
    code_point += (mask8(*it) << 6) & 0xfff;
    UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR(it, end)
    code_point += (*it) & 0x3f;
    return UTF8_OK;
}

#undef UTF8_CPP_INCREASE_AND_RETURN_ON_ERROR

template <typename octet_iterator>
utf_error validate_next(octet_iterator& it, octet_iterator end, uint32_t& code_point)
{
    octet_iterator original_it = it;
    uint32_t cp = 0;
    typedef typename std::iterator_traits<octet_iterator>::difference_type octet_difference_type;
    const octet_difference_type length = sequence_length(it);

    utf_error err = UTF8_OK;
    switch (length)
    {
    case 0:
        return INVALID_LEAD;
    case 1:
        err = get_sequence_1(it, end, cp);
        break;
    case 2:
        err = get_sequence_2(it, end, cp);
        break;
    case 3:
        err = get_sequence_3(it, end, cp);
        break;
    case 4:
        err = get_sequence_4(it, end, cp);
        break;
    }

    if (err == UTF8_OK)
    {
        if (is_code_point_valid(cp))
        {
            if (!is_overlong_sequence(cp, length))
            {
                code_point = cp;
                ++it;
                return UTF8_OK;
            }
            else
            {
                err = OVERLONG_SEQUENCE;
            }
        }
        else
        {
            err = INVALID_CODE_POINT;
        }
    }

    it = original_it;
    return err;
}

template <typename octet_iterator>
inline utf_error validate_next(octet_iterator& it, octet_iterator end)
{
    uint32_t ignored;
    return validate_next(it, end, ignored);
}
}

template <typename octet_iterator>
octet_iterator append(uint32_t cp, octet_iterator result)
{
    if (!internal::is_code_point_valid(cp))
    {
        throw invalid_code_point(cp);
    }

    if (cp < 0x80)
    {
        *(result++) = static_cast<uint8_t>(cp);
    }
    else if (cp < 0x800)
    {
        *(result++) = static_cast<uint8_t>((cp >> 6) | 0xc0);
        *(result++) = static_cast<uint8_t>((cp & 0x3f) | 0x80);
    }
    else if (cp < 0x10000)
    {
        *(result++) = static_cast<uint8_t>((cp >> 12) | 0xe0);
        *(result++) = static_cast<uint8_t>(((cp >> 6) & 0x3f) | 0x80);
        *(result++) = static_cast<uint8_t>((cp & 0x3f) | 0x80);
    }
    else
    {
        *(result++) = static_cast<uint8_t>((cp >> 18) | 0xf0);
        *(result++) = static_cast<uint8_t>(((cp >> 12) & 0x3f) | 0x80);
        *(result++) = static_cast<uint8_t>(((cp >> 6) & 0x3f) | 0x80);
        *(result++) = static_cast<uint8_t>((cp & 0x3f) | 0x80);
    }
    return result;
}

template <typename octet_iterator, typename output_iterator>
output_iterator replace_invalid(octet_iterator start, octet_iterator end,
                                output_iterator out, uint32_t replacement)
{
    while (start != end)
    {
        octet_iterator sequence_start = start;
        internal::utf_error err_code = internal::validate_next(start, end);
        switch (err_code)
        {
        case internal::UTF8_OK:
            for (octet_iterator it = sequence_start; it != start; ++it)
            {
                *out++ = *it;
            }
            break;
        case internal::NOT_ENOUGH_ROOM:
            throw not_enough_room();
        case internal::INVALID_LEAD:
            out = append(replacement, out);
            ++start;
            break;
        case internal::INCOMPLETE_SEQUENCE:
        case internal::OVERLONG_SEQUENCE:
        case internal::INVALID_CODE_POINT:
            out = append(replacement, out);
            ++start;

            while (start != end && internal::is_trail(*start))
            {
                ++start;
            }
            break;
        }
    }
    return out;
}

template <typename octet_iterator>
uint32_t next(octet_iterator& it, octet_iterator end)
{
    uint32_t cp = 0;
    internal::utf_error err_code = internal::validate_next(it, end, cp);
    switch (err_code)
    {
    case internal::UTF8_OK:
        break;
    case internal::NOT_ENOUGH_ROOM:
        throw not_enough_room();
    case internal::INVALID_LEAD:
    case internal::INCOMPLETE_SEQUENCE:
    case internal::OVERLONG_SEQUENCE:
    {
        throw invalid_utf8(*it);
    }
    case internal::INVALID_CODE_POINT:
    {
        throw invalid_code_point(cp);
    }
    }
    return cp;
}
}

#endif
