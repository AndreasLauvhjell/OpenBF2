// Path: [INFERRED] shared helper in the persistence cluster.
#ifndef GAME_PERSISTENCE_SIMPLETOKENIZER_H
#define GAME_PERSISTENCE_SIMPLETOKENIZER_H
#include <string>
#include <cstdint>
namespace dice { namespace hfe {
// [INFERRED] co-location of this helper until its original source path is attested.

// init deliberately accumulates delimiter bits when called more than once.
class SimpleTokenizer
{
public:
    // Creates an empty tokenizer with no delimiters and position zero; call init before use.
    SimpleTokenizer()
        : m_delimiters{}, m_position(0)
    {
    }
    SimpleTokenizer(const std::string& text, const std::string& delimiters)
        : m_delimiters{}, m_position(0)
    {
        init(text, delimiters);
    }
    void init(const std::string& text, const std::string& delimiters)
    {
        m_position = 0;
        m_text = text;
        for (unsigned int i = 0; i < delimiters.size(); ++i)
        {
            const unsigned char c = static_cast<unsigned char>(delimiters[i]);
            m_delimiters[c >> 5] |= std::uint32_t(1) << (c & 31);
        }
    }
    bool next(std::string& token)
    {
        token.clear();
        while (m_position < m_text.size() && isDelimiter(m_text[m_position]))
            ++m_position;
        const unsigned int start = m_position;
        if (start >= m_text.size())
            return false;
        while (m_position < m_text.size() && !isDelimiter(m_text[m_position]))
            ++m_position;
        token = m_text.substr(start, m_position - start);
        return true;
    }
private:
    bool isDelimiter(unsigned char c) const
    {
        return (m_delimiters[c >> 5] & (std::uint32_t(1) << (c & 31))) != 0;
    }
    std::string m_text;
    std::uint32_t m_delimiters[8];
    unsigned int m_position;
};
} }
#endif

