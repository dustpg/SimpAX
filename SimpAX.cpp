#include "SimpAX.h"
#include <cassert>
#include <cstddef>
#include <memory>
#include <array>

//#define SAX_PROFILER
#ifdef SAX_PROFILER
#include <chrono>
#endif

#if !(defined(NDEBUG) && defined(SAX_NO_ERROR_CHECK))
#define SAX_DO_ERROR_CHECK
#endif


// �������Զ�������

/// <summary>
/// Frees the specified .
/// </summary>
/// <param name="">The .</param>
/// <returns></returns>
inline void SimpAX::CAXStream::free(void* ptr) noexcept {
    return std::free(ptr);
}

/// <summary>
/// Mallocs the specified .
/// </summary>
/// <param name="">The .</param>
/// <returns></returns>
inline void*SimpAX::CAXStream::malloc(size_t len) noexcept {
    return std::malloc(len);
}

/// <summary>
/// Reallocs the specified PTR.
/// </summary>
/// <param name="ptr">The PTR.</param>
/// <param name="">The .</param>
/// <returns></returns>
inline void*SimpAX::CAXStream::realloc(void* ptr, size_t len) noexcept {
    return std::realloc(ptr, len);
}

#ifdef _MSC_VER
_declspec(noinline)
#endif

/// <summary>
/// Operator==s the specified a.
/// </summary>
/// <param name="a">a.</param>
/// <param name="b">The b.</param>
/// <returns></returns>
bool SimpAX::IsSame(const StrPair& a, const StrPair& b) noexcept {
    // ������ͬ��˵
    if (a.b - a.a == b.b - b.a) {
        // Ϊ���ų�null
        if (a.a == b.a) return true;
        if (!a.a | !b.a) return false;
        // ����
        const auto* __restrict itr1 = a.a, * __restrict itr2 = b.a;
        while (itr1 < a.b) { if (*itr1 != *itr2) return false; ++itr1; ++itr2; }
        // ȫ��һ��
        return true;
    }
    return false;
}

// simpax::impl namepsace
namespace SimpAX { namespace impl {
    template<unsigned... Is> struct seq {};
    template<unsigned N, unsigned... Is>
    struct gen_seq : gen_seq<N - 1, N - 1, Is...> {};
    template<unsigned... Is>
    struct gen_seq<0, Is...> : seq<Is...> {};

    using Table = std::array<const char, 128>;
    // hex-number
    static inline constexpr char hex(char t) {
        return (t >= '0' && t <= '9') ? (t - '0')
            : ((t >= 'A' && t <= 'F') ? (t - 'A' + 10)
                : ((t >= 'a' && t <= 'f') ? (t - 'a' + 10) : -1));
    }
    template<unsigned... Is>
    constexpr Table hex_gen(seq<Is...>) { return{ { hex(Is)... } }; }
    constexpr Table hex_gen() { return hex_gen(gen_seq<128>{}); }
    // hex table
    static constexpr Table HEX_TABLE = hex_gen();
    // get hex number
    static inline auto get_hex(Char ch) noexcept -> Char {
        Char num = HEX_TABLE[(ch & 127)]; assert(num != Char(-1)); return num;
    }
    // is space?
    static inline bool is_space(Char ch) noexcept {
        return (ch == Char(' ')) | (ch == Char('\t'));
    }
    // is tag start
    static inline bool is_begin_tag_open(Char ch) noexcept {
        return ch == Char('<');
    }
    // is tag end
    static inline bool is_begin_tag_close(Char ch) noexcept {
        return ch == Char('>');
    }
    // is number start
    static inline bool is_number_start(Char ch) noexcept {
        return ch == Char('#');
    }
    // is hex number start
    static inline bool is_hex_number_start(Char ch) noexcept {
        return ch == Char('x');
    }
    // is hex number start
    static inline bool is_namespace(Char ch) noexcept {
        return ch == Char(':');
    }
    // is tag close
    static inline bool is_end_tag(Char ch) noexcept {
        return ch == Char('/');
    }
    // is pi start
    static inline bool is_begin_pi(Char ch) noexcept {
        return ch == Char('?');
    }
    // is pi end
    static inline bool is_end_pi(Char ch) noexcept {
        return ch == Char('?');
    }
    // is comment start
    static inline bool is_begin_comment(Char ch) noexcept {
        return ch == Char('!');
    }
    // is comment start2
    static inline bool is_begin_comment2(const Char& ch) noexcept {
        return (ch == Char('-')) && (1[&ch] == Char('-'));
    }
    // is comment end
    static inline bool is_end_comment(Char ch) noexcept {
        return (ch == Char('-'));
    }
    // is comment end2
    static inline bool is_end_comment2(const Char& ch) noexcept {
        return (ch == Char('-')) && (1[&ch] == Char('>'));
    }
    // is equal in attr?
    static inline bool is_attr_eq(Char ch) noexcept {
        return (ch == Char('='));
    }
    // is quot attr?
    static inline bool is_quot(Char ch) noexcept {
        return (ch == Char('"')) | (ch == Char('\''));
    }
    // is escape?
    static inline bool is_escape(Char ch) noexcept {
        return ch == Char('&');
    }
    // is escape end
    static inline bool is_end_escape(Char ch) noexcept {
        return ch == Char(';');
    }
    // is sting head equal
    static auto is_str_head_equal(
        const Char* __restrict a, 
        const Char* __restrict b
    ) noexcept ->const Char* {
        while (*b) { if (*a != *b) return nullptr; ++a; ++b; }
        return a;
    }
}}

/// <summary>
/// Initializes a new instance of the <see cref="CAXStream"/> class.
/// </summary>
SimpAX::CAXStream::CAXStream() noexcept {

}

/// <summary>
/// Finalizes an instance of the <see cref="CAXStream"/> class.
/// </summary>
/// <returns></returns>
SimpAX::CAXStream::~CAXStream() noexcept {
    this->free(m_pEscapeBuffer);
    if (m_pStackBase != m_aStackBuffer) this->free(m_pStackBase);
}

/*
    ���ƿ��Ժ���ĸ�������Լ��������ַ�
    ���Ʋ��������ֻ��߱����ſ�ʼ
    ���Ʋ������ַ� ��xml�������� XML��Xml����ʼ
    ���Ʋ��ܰ����ո�
*/

/// <summary>
/// Loads the specified string.
/// </summary>
/// <param name="str">The string.</param>
/// <returns></returns>
auto SimpAX::CAXStream::Load(const Char* str) noexcept -> Result {
    // ״̬
    m_pStackTop = m_pStackBase;
    // ״̬��
    enum sm {
        // ׼������Ԫ��, �ȴ���ǩ
        sm_standby,
        // ��������ע��, �ȴ�-->
        sm_comment_end,
        // ��������ָ��, �ȴ�ָ��
        sm_pi_begin,
        // ��������ָ��, �ȴ�?
        sm_pi_end,
        // ����Ԫ������, �ȴ��հ�/��ǩ��������
        sm_elename_end,
        // ��ǩ��������
        sm_ele_end,
        // ��ʼ���Լ���, �ȴ��ַ�
        sm_key_begin,
        // �������Լ���, �ȴ��հ�
        sm_key_end,
        // �������ԵȺ�, �ȴ��Ⱥ�
        sm_attr_eq,
        // ��ʼ������ֵ, �ȴ�'/"����
        sm_value_begin,
        // ����������ֵ, �ȴ��ض�����
        sm_value_end,
        // ����CDATA ֵ, �ȴ�]]>
        sm_cdata_end,
        // ����
        sm_count,
        // ״̬
    } state = sm_standby;
    // ��ǰԪ��
    StrPair this_element;
    StrPair this_text;
    // ����Ԫ��
    union { StrPair this_comment; ATTRs this_attr; PIs this_pi; };
    Char    this_quot;
    bool    value_escape = false, text_escape = false;
    // �����¼
    const auto doc = str;
    auto make_error = [&, doc](Result::Code code) noexcept ->Result {
#if defined(SAX_NO_ERROR_CHECK)
        assert(code == Result::Code_OOM && "bad xml");
#endif
        return{ code, uint32_t(str - doc) };
    };
#ifdef SAX_PROFILER
    using hclock = std::chrono::high_resolution_clock;
    using tp = std::chrono::time_point<hclock>;
    tp tps[sm_count];
#endif
#ifndef DEBUG
    this_quot = 0;
#endif
    this_text.a = str;
#ifdef SAX_PROFILER
    auto last = hclock::now();
#endif
    // ����
    while (auto ch = *str) {
#ifdef SAX_PROFILER
        auto now = hclock::now();
        auto count = now - last;
        last = now;
        tps[state] += count;
#endif
        switch (state)
        {
        case sm_standby:
            // ��¼ת�����
            text_escape |= impl::is_escape(ch);
            // �ȴ���ǩ��ʼ
            if (impl::is_begin_tag_open(ch)) {
                this_text.b = str; ++str; ch = *str;
                // �ύTEXT
                if (this_text.a != this_text.b) {
                    // ת���ַ�?
                    if (text_escape) {
                        text_escape = false;
                        this->interpret_escape(this_text);
                        if (!m_pEscapeBuffer) return make_error(Result::Code_OOM);
                    }
                    this->add_text(this_text);
                }
                // 0. ������ǩ
                if (impl::is_end_tag(ch)) { this_element.a = str + 1; state = sm_ele_end; }
                // 1. ?ָ��
                else if (impl::is_begin_pi(ch)) { state = sm_pi_begin; this_pi.target.a = str+1; }
                // 2. !ע��/CDATA
                else if (impl::is_begin_comment(ch)) {
                    // !--, �ȴ�����-->
                    if (impl::is_begin_comment2(str[1])) { 
                        state = sm_comment_end; str += 3; this_comment.a = str; continue;
                    }
                    // ![CDATA[, �ȴ�����]]>
                    else if (auto cdata = impl::is_str_head_equal(str + 1, "[CDATA[")) {
                        state = sm_cdata_end; this_text.a = str = cdata; continue;
                    }
#ifdef SAX_DO_ERROR_CHECK
                    // �������
                    else return make_error(Result::Code_BadComment);
#endif
                }
                // 3. ��ǩ, TODO: ����ǩ������Ч��
                else {
#ifdef SAX_DO_ERROR_CHECK
                    if (impl::is_space(ch)) return make_error(Result::Code_BadElement);
#endif
                    this_element.a = str;
                    state = sm_elename_end;
                }
            }
            break;
        case sm_comment_end:
            // �ȴ�ע�ͽ�����
            if (impl::is_end_comment(ch)) {
                if (impl::is_end_comment2(str[1])) {
                    state = sm_standby;
                    this_comment.b = str;
                    this_text.a = str += 3;
                    this->add_comment(this_comment);
                    continue;
                }
            }
            break;
        case sm_pi_begin:
            //�ر�
            if (impl::is_end_pi(ch)) {
                this_pi.target.b = str;
                this_pi.instructions.a = str;
                [[fallthrough]];
            }
            // �հ�
            else {
                if (impl::is_space(ch)) {
                    this_pi.target.b = str;
                    this_pi.instructions.a = str + 1;
                    state = sm_pi_end;
                }
                break;
            }
        case sm_pi_end:
            // �ȴ�?>
            if (impl::is_end_pi(ch)) {
                this_pi.instructions.b = str; ++str;
                this_text.a = str + 1;
                if (impl::is_begin_tag_close(*str)) state = sm_standby;
#ifdef SAX_DO_ERROR_CHECK
                else return make_error(Result::Code_BadPI);
#endif
                this->add_processing(this_pi);
            }
            break;
        case sm_elename_end:
            // �հ� ���� �ر�, ��ʾ���
            if (impl::is_space(ch) | impl::is_begin_tag_close(ch) | impl::is_end_tag(ch)) {
                this_element.b = str;
                this->begin_element(this_element);
                this->push(this_element);
                if (!this->is_stack_ok()) return make_error(Result::Code_OOM);
                state = sm_standby;
                // �հ׷���, �ȴ���
                if (impl::is_space(ch)) state = sm_key_begin;
                // ����/�ر� ��ǩ
                else {
                    // ������ǩ
                    if (impl::is_end_tag(ch)) {
                        this->pop();
                        this->end_element(this_element);
#ifndef DEBUG
                        std::memset(&this_element, 0, sizeof(this_element));
#endif
                        // �ܵ��ر�
                        str++; 
#ifdef SAX_DO_ERROR_CHECK
                        if (!impl::is_begin_tag_close(*str)) {
                            return make_error(Result::Code_BadElement);
                        }
#endif
                    }
                    this_text.a = str + 1;
                }
            }
            break;
        case sm_ele_end:
            // �ȴ��հ� ��ǩ�ر�
            if (impl::is_space(ch) | impl::is_begin_tag_close(ch)) {
                this_element.b = str;
                // ���ƥ�����
#ifdef SAX_DO_ERROR_CHECK
                if (m_pStackTop == m_pStackBase || this->stack_top() != this_element) {
                    return make_error(Result::Code_Mismatched);
                }
#endif
                // �����հ�, ֱ���ر�
                while (impl::is_begin_tag_close(*str)) ++str;
                this_text.a = str;
                state = sm_standby;
                this->pop();
                this->end_element(this_element);
#ifndef DEBUG
                std::memset(&this_element, 0, sizeof(this_element));
#endif
                continue;
            }
            break;
        case sm_key_begin:
            // ��������
            if (impl::is_end_tag(ch)) {
                state = sm_standby;
                this->pop();
                this->end_element(this_element);
#ifndef DEBUG
                std::memset(&this_element, 0, sizeof(this_element));
#endif
                // �ܵ��ر�
                str++; 
#ifdef SAX_DO_ERROR_CHECK
                if (!impl::is_begin_tag_close(*str))
                    return make_error(Result::Code_BadElement);
#endif
            }
            // ��������>
            else if (impl::is_begin_tag_close(ch)) {
                state = sm_standby;
                this_text.a = str+1;
            }
            // �����ǿհ�, ����
            else if (!impl::is_space(ch)) {
                this_attr.key.a = str;
                state = sm_key_end;
            }
            break;
        case sm_key_end:
            // �����հ�
            if (impl::is_space(ch) | impl::is_attr_eq(ch)) {
                this_attr.key.b = str;
                if (impl::is_attr_eq(ch)) state = sm_value_begin;
                else state = sm_attr_eq;
            }
            break;
        case sm_attr_eq:
            state = sm_value_begin;
            break;
        case sm_value_begin:
            // �ȴ�����
            if (impl::is_quot(ch)) {
                // ��¼��ǰ��������
                this_quot = ch;
                this_attr.value.a = str + 1;
                //value_escape = false;
                state = sm_value_end;
            }
            break;
        case sm_value_end:
            assert(impl::is_quot(this_quot));
            // ��¼ת�����
            value_escape |= impl::is_escape(ch);
            // ���Ѵ��ڵ�����ƥ��
            if (ch == this_quot) {
                this_attr.value.b = str;
                // ����ת�����
                if (value_escape) {
                    value_escape = false;
                    this->interpret_escape(this_attr.value);
                    if (!m_pEscapeBuffer) return make_error(Result::Code_OOM);
                }
                state = sm_key_begin;
                this->add_attribute(this_attr);
#ifndef DEBUG
                std::memset(&this_attr, 0, sizeof(this_attr));
                this_quot = 0;
                this->free(m_pEscapeBuffer);
                m_pEscapeBuffer = nullptr;
                m_pEscapeBufferEnd = nullptr;
#endif
            }
            break;
        case sm_cdata_end:
            // �ȴ�]]>, �ύTEXT��ת�����״̬
            if (auto cdata = impl::is_str_head_equal(str, "]]>")) {
                auto tmp = this_text; tmp.b = str;
                this_text.a = str = cdata; 
                state = sm_standby;
                this->add_text(tmp);
                continue;
            }
            break;
        }
        ++str;
    }
    // ����TEXT
    this_text.b = str; if (this_text.b != this_text.a) {
        // ת�����
        if (text_escape) {
            text_escape = false;
            this->interpret_escape(this_text);
            if (!m_pEscapeBuffer) return make_error(Result::Code_OOM);
        }
        this->add_text(this_text);
    }
    // ������
    Result re{ Result::Code::Code_OK, 0 };
    if (m_pStackTop != m_pStackBase) re.code = Result::Code_SyntaxError;
    if (state != sm_standby) re.code = Result::Code_InternalError;
#ifdef SAX_PROFILER
    double times[sm_count];
    auto time = times;
    double all = 0.0;
    for (auto t : tps) {
        std::chrono::duration<double> diff = t - tp{};
        *time = diff.count();
        all += *time;
        ++time;
    }
    for (int i = 0; i != sm_count; ++i) {
        std::printf(
            "PROFILER[%x]: (%5.2f%%) %lfms \n", 
            i, times[i] * 100. / all, times[i]
        );
    }
#endif
    return re;
}

/// <summary>
/// Adds the processing.
/// </summary>
/// <param name="attr">The attribute.</param>
/// <returns></returns>
void SimpAX::CAXStream::add_processing(const PIs& attr) noexcept {
    attr;
}

/// <summary>
/// Begins the element.
/// </summary>
/// <param name="pair">The pair.</param>
/// <returns></returns>
void SimpAX::CAXStream::begin_element(const StrPair element) noexcept {
    element;
}

/// <summary>
/// Ends the element.
/// </summary>
/// <param name="pair">The pair.</param>
/// <returns></returns>
void SimpAX::CAXStream::end_element(const StrPair element) noexcept {
    element;
}

/// <summary>
/// Adds the attribute.
/// </summary>
/// <param name="attr">The attribute.</param>
/// <returns></returns>
void SimpAX::CAXStream::add_attribute(const ATTRs& attr) noexcept {
    attr;
}

/// <summary>
/// Adds the text.
/// </summary>
/// <param name="text">The text.</param>
/// <returns></returns>
void SimpAX::CAXStream::add_text(const StrPair text) noexcept {
    text;
}

/// <summary>
/// Adds the comment.
/// </summary>
/// <param name="comment">The comment.</param>
/// <returns></returns>
void SimpAX::CAXStream::add_comment(const StrPair comment) noexcept {
    comment;
}

/// <summary>
/// Grows up.
/// </summary>
void SimpAX::CAXStream::grow_up() {
    // Stack overflow   �Ҿ�������⼸���ʶ���
    // Ĭ�ϳ���32, ��������
    const auto cap = m_pStackCap - m_pStackBase;
    const auto len = m_pStackTop - m_pStackBase;
    const auto newcap = cap ? cap * 2 : 32;
    const auto ptr = this->malloc(newcap * sizeof(StrPair));
    // ����
    if (len) std::memcpy(ptr, m_pStackBase, len * sizeof(StrPair));
    // �滻
    if (m_pStackBase != m_aStackBuffer) this->free(m_pStackBase);
    m_pStackBase = reinterpret_cast<StrPair*>(ptr);
    m_pStackCap = m_pStackBase + newcap;
    m_pStackTop = m_pStackBase + len;
}

/// <summary>
/// Finds the 1ST namespace.
/// </summary>
/// <param name="pair">The pair.</param>
/// <returns></returns>
void SimpAX::CAXStream::find_1st_namespace(StrPair& pair) noexcept {
    auto str = pair;
    while (str.a < str.b) { if (impl::is_namespace(*str.a)) break; str.a++; }
    pair.b = str.a;
}

/// <summary>
/// Tries the get instruction value.
/// </summary>
/// <param name="key">The key.</param>
/// <param name="ins">The ins.</param>
/// <returns></returns>
bool SimpAX::CAXStream::try_get_instruction_value(
    const Char* key, StrPair& ins) noexcept {
    auto pair = ins;
    while (pair.a < pair.b) {
        // ƥ��KEY
        if (auto str = impl::is_str_head_equal(pair.a, key)) {
            // ƥ��Ⱥ�
            while (!impl::is_attr_eq(*str)) {
                ++str; if (str == pair.b) return false;
            }
            // ƥ������
            while (!impl::is_quot(*str)) {
                ++str; if (str == pair.b) return false;
            }
            // ��¼����
            const auto this_quot = *str; ++str; pair.a = str;
            // ƥ������
            while (*str != this_quot) {
                ++str; if (str == pair.b) return false;
            }
            pair.b = str;
            ins = pair;
        }
        // �ƽ�����
        ++pair.a;
    }
    return true;
}

#ifndef NDEBUG
/// <summary>
/// Stacks the top.
/// </summary>
/// <returns></returns>
auto SimpAX::CAXStream::stack_top() noexcept ->StrPair& {
    assert(m_pStackTop > m_pStackBase && "no element in stack");
    return m_pStackTop[-1]; 
}
#endif

/// <summary>
/// Pops this instance.
/// </summary>
void SimpAX::CAXStream::pop() {
    assert(m_pStackTop != m_pStackBase);
    --m_pStackTop;
#ifndef NDEBUG
    std::memset(m_pStackTop, -1, sizeof(*m_pStackTop));
#endif
}

/// <summary>
/// Pushes the specified string.
/// </summary>
/// <param name="str">The string.</param>
bool SimpAX::CAXStream::push(StrPair str) {
    if (m_pStackTop == m_pStackCap) this->grow_up();
    if (!m_pStackBase) return true;
    *m_pStackTop = str; m_pStackTop++;
    return false;
}

/// <summary>
/// Interprets the escape.
/// </summary>
/// <param name="">The .</param>
/// <returns></returns>
_declspec(noinline)
void SimpAX::CAXStream::interpret_escape(StrPair& pair) noexcept {
    // ��֤����׼ȷ
    const auto length = pair.end() - pair.begin();
    auto nowlen = m_pEscapeBufferEnd - m_pEscapeBuffer;
    m_pEscapeBufferEnd = nullptr;
    // ���治��
    if (length > nowlen) {
#ifndef NDEBUG
        nowlen = length;
#else
        nowlen = length * 2;
#endif
        this->free(m_pEscapeBuffer);
        const auto ptr = this->malloc(sizeof(Char) * nowlen);
        m_pEscapeBuffer = reinterpret_cast<Char*>(ptr);
    }
    // �ڴ治��
    if (!m_pEscapeBuffer) return;
    m_pEscapeBufferEnd = m_pEscapeBuffer + nowlen;
    // ��ʽ����
    /*
        ʵ������ �ַ� ʮ�������� ʮ����������
        quot    "   &#34;   &#x22;
        amp     &   &#38;   &#x26;
        apos    '   &#39;   &#x27;
        lt      <   &#60;   &#x3C;
        gt      >   &#62;   &#x3E;
    */
    auto* __restrict src_itr = pair.a;
    const auto src_end = pair.b;
    auto* __restrict des_itr = m_pEscapeBuffer;
    // ת�����
    const Char* escape = nullptr;
    // �����ַ���
    while (src_itr != src_end) {
        const auto ch = src_itr[0];
        // ת����ʾ��
        if (impl::is_escape(ch)) {
            escape = src_itr;
        }
        // ת�������
        else if (escape && impl::is_end_escape(ch)) {
            constexpr Char A = Char('a');
            constexpr Char G = Char('g');
            constexpr Char M = Char('m');
            constexpr Char P = Char('p');
            constexpr Char O = Char('o');
            constexpr Char S = Char('s');
            constexpr Char L = Char('l');
            constexpr Char T = Char('t');
            constexpr Char Q = Char('q');
            constexpr Char U = Char('u');
            Char escape_ch = 0;
            switch (src_itr - escape)
            {
            case 3:
                if ((escape[1] == L) & (escape[2] == T)) escape_ch = Char('<');
                else if ((escape[1] == G) & (escape[2] == T)) escape_ch = Char('>');
                break;
            case 4:
                if (impl::is_number_start(escape[1])) 
                    escape_ch = impl::get_hex(escape[2]) * 10 + impl::get_hex(escape[3]);
                else if ((escape[1] == A) & (escape[2] == M) & (escape[3] == P)) 
                    escape_ch = Char('&');
                break;
            case 5:
                if (impl::is_number_start(escape[1]) && impl::is_hex_number_start(escape[2])) 
                    escape_ch = impl::get_hex(escape[3]) * 16 | impl::get_hex(escape[4]);
                else if ((escape[1] == A) & (escape[2] == P) & (escape[3] == O) & (escape[4] == S)) 
                    escape_ch = Char('\'');
                else if ((escape[1] == Q) & (escape[2] == U) & (escape[3] == O) & (escape[4] == T))
                    escape_ch = Char('"');
                break;
            }
            // ת��ɹ�
            if (escape_ch) { 
                des_itr -= src_itr - escape;
                *des_itr = escape_ch; 
                ++des_itr;  
                ++src_itr;
                continue;
            }
            escape = nullptr;
        }
        // �����ַ���
        *des_itr = ch;
        ++des_itr;
        ++src_itr;
    }
    // д���ַ���
    pair.a = m_pEscapeBuffer;
    pair.b = des_itr;
}