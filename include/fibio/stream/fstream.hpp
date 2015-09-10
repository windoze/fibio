//
//  fstream.hpp
//  fibio
//
//===------------------------- fstream ------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  Copyright (c) 2014 0d0a.com. All rights reserved.
//

#ifndef fibio_stream_fstream_hpp
#define fibio_stream_fstream_hpp

#include <ostream>
#include <istream>
#include <locale>
#include <cstdio>
#include <fibio/fibers/future/async.hpp>

namespace fibio {
namespace stream {
namespace detail {

std::shared_ptr<fibers::foreign_thread_pool> get_default_executor();

} // End of namespace detail

template <class CharT, class Traits = std::char_traits<CharT>>
class basic_filebuf : public std::basic_streambuf<CharT, Traits>
{
public:
    typedef CharT char_type;
    typedef Traits traits_type;
    typedef typename traits_type::int_type int_type;
    typedef typename traits_type::pos_type pos_type;
    typedef typename traits_type::off_type off_type;
    typedef typename traits_type::state_type state_type;

    // 27.9.1.2 Constructors/destructor:
    basic_filebuf();
    basic_filebuf(basic_filebuf&& rhs);
    virtual ~basic_filebuf();

    // 27.9.1.3 Assign/swap:
    basic_filebuf& operator=(basic_filebuf&& rhs);
    void swap(basic_filebuf& rhs);

    // 27.9.1.4 Members:
    bool is_open() const;
    basic_filebuf* open(const char* s, std::ios_base::openmode mode);
    basic_filebuf* open(const std::string& s, std::ios_base::openmode mode);
    basic_filebuf* close();

    void set_executor(std::shared_ptr<fibers::foreign_thread_pool> e);

protected:
    // 27.9.1.5 Overridden virtual functions:
    virtual int_type underflow();
    virtual int_type pbackfail(int_type c = traits_type::eof());
    virtual int_type overflow(int_type c = traits_type::eof());
    virtual std::basic_streambuf<char_type, traits_type>* setbuf(char_type* s, std::streamsize n);
    virtual pos_type seekoff(off_type off,
                             std::ios_base::seekdir way,
                             std::ios_base::openmode wch = std::ios_base::in | std::ios_base::out);
    virtual pos_type seekpos(pos_type sp,
                             std::ios_base::openmode wch = std::ios_base::in | std::ios_base::out);
    virtual int sync();
    virtual void imbue(const std::locale& loc);

private:
    char* extbuf_;
    const char* extbufnext_;
    const char* extbufend_;
    char extbuf_min_[8];
    size_t ebs_;
    char_type* intbuf_;
    size_t ibs_;
    FILE* file_;
    const std::codecvt<char_type, char, state_type>* cv_;
    state_type st_;
    state_type st_last_;
    std::ios_base::openmode om_;
    std::ios_base::openmode cm_;
    bool owns_eb_;
    bool owns_ib_;
    bool always_noconv_;
    std::shared_ptr<fibers::foreign_thread_pool> executor_;

    bool read_mode();
    void write_mode();
};

template <class CharT, class Traits>
basic_filebuf<CharT, Traits>::basic_filebuf()
: extbuf_(0)
, extbufnext_(0)
, extbufend_(0)
, ebs_(0)
, intbuf_(0)
, ibs_(0)
, file_(0)
, cv_(nullptr)
, st_()
, st_last_()
, om_(std::ios_base::openmode(0))
, cm_(std::ios_base::openmode(0))
, owns_eb_(false)
, owns_ib_(false)
, always_noconv_(false)
, executor_(detail::get_default_executor())
{
    if (std::has_facet<std::codecvt<char_type, char, state_type>>(this->getloc())) {
        cv_ = &std::use_facet<std::codecvt<char_type, char, state_type>>(this->getloc());
        always_noconv_ = cv_->always_noconv();
    }
    setbuf(0, 4096);
}

template <class CharT, class Traits>
basic_filebuf<CharT, Traits>::basic_filebuf(basic_filebuf&& rhs)
: std::basic_streambuf<CharT, Traits>(rhs), executor_(rhs.executor_)
{
    if (rhs.extbuf_ == rhs.extbuf_min_) {
        extbuf_ = extbuf_min_;
        extbufnext_ = extbuf_ + (rhs.extbufnext_ - rhs.extbuf_);
        extbufend_ = extbuf_ + (rhs.extbufend_ - rhs.extbuf_);
    } else {
        extbuf_ = rhs.extbuf_;
        extbufnext_ = rhs.extbufnext_;
        extbufend_ = rhs.extbufend_;
    }
    ebs_ = rhs.ebs_;
    intbuf_ = rhs.intbuf_;
    ibs_ = rhs.ibs_;
    file_ = rhs.file_;
    cv_ = rhs.cv_;
    st_ = rhs.st_;
    st_last_ = rhs.st_last_;
    om_ = rhs.om_;
    cm_ = rhs.cm_;
    owns_eb_ = rhs.owns_eb_;
    owns_ib_ = rhs.owns_ib_;
    always_noconv_ = rhs.always_noconv_;
    if (rhs.pbase()) {
        if (rhs.pbase() == rhs.intbuf_)
            this->setp(intbuf_, intbuf_ + (rhs.epptr() - rhs.pbase()));
        else
            this->setp((char_type*)extbuf_, (char_type*)extbuf_ + (rhs.epptr() - rhs.pbase()));
        this->pbump(rhs.pptr() - rhs.pbase());
    } else if (rhs.eback()) {
        if (rhs.eback() == rhs.intbuf_)
            this->setg(intbuf_, intbuf_ + (rhs.gptr() - rhs.eback()),
                       intbuf_ + (rhs.egptr() - rhs.eback()));
        else
            this->setg((char_type*)extbuf_,
                       (char_type*)extbuf_ + (rhs.gptr() - rhs.eback()),
                       (char_type*)extbuf_ + (rhs.egptr() - rhs.eback()));
    }
    rhs.extbuf_ = 0;
    rhs.extbufnext_ = 0;
    rhs.extbufend_ = 0;
    rhs.ebs_ = 0;
    rhs.intbuf_ = 0;
    rhs.ibs_ = 0;
    rhs.file_ = 0;
    rhs.st_ = state_type();
    rhs.st_last_ = state_type();
    rhs.om_ = std::ios_base::openmode(0);
    rhs.cm_ = std::ios_base::openmode(0);
    rhs.owns_eb_ = false;
    rhs.owns_ib_ = false;
    rhs.setg(0, 0, 0);
    rhs.setp(0, 0);
}

template <class CharT, class Traits>
inline basic_filebuf<CharT, Traits>& basic_filebuf<CharT, Traits>::operator=(basic_filebuf&& rhs)
{
    close();
    swap(rhs);
    return *this;
}

template <class CharT, class Traits>
basic_filebuf<CharT, Traits>::~basic_filebuf()
{
    try {
        close();
    } catch (...) {
    }
    if (owns_eb_) delete[] extbuf_;
    if (owns_ib_) delete[] intbuf_;
}

template <class CharT, class Traits>
void basic_filebuf<CharT, Traits>::swap(basic_filebuf& rhs)
{
    std::basic_streambuf<char_type, traits_type>::swap(rhs);
    if (extbuf_ != extbuf_min_ && rhs.extbuf_ != rhs.extbuf_min_) {
        std::swap(extbuf_, rhs.extbuf_);
        std::swap(extbufnext_, rhs.extbufnext_);
        std::swap(extbufend_, rhs.extbufend_);
    } else {
        ptrdiff_t ln = extbufnext_ - extbuf_;
        ptrdiff_t le = extbufend_ - extbuf_;
        ptrdiff_t rn = rhs.extbufnext_ - rhs.extbuf_;
        ptrdiff_t re = rhs.extbufend_ - rhs.extbuf_;
        if (extbuf_ == extbuf_min_ && rhs.extbuf_ != rhs.extbuf_min_) {
            extbuf_ = rhs.extbuf_;
            rhs.extbuf_ = rhs.extbuf_min_;
        } else if (extbuf_ != extbuf_min_ && rhs.extbuf_ == rhs.extbuf_min_) {
            rhs.extbuf_ = extbuf_;
            extbuf_ = extbuf_min_;
        }
        extbufnext_ = extbuf_ + rn;
        extbufend_ = extbuf_ + re;
        rhs.extbufnext_ = rhs.extbuf_ + ln;
        rhs.extbufend_ = rhs.extbuf_ + le;
    }
    std::swap(ebs_, rhs.ebs_);
    std::swap(intbuf_, rhs.intbuf_);
    std::swap(ibs_, rhs.ibs_);
    std::swap(file_, rhs.file_);
    std::swap(cv_, rhs.cv_);
    std::swap(st_, rhs.st_);
    std::swap(st_last_, rhs.st_last_);
    std::swap(om_, rhs.om_);
    std::swap(cm_, rhs.cm_);
    std::swap(owns_eb_, rhs.owns_eb_);
    std::swap(owns_ib_, rhs.owns_ib_);
    std::swap(always_noconv_, rhs.always_noconv_);
    if (this->eback() == (char_type*)rhs.extbuf_min_) {
        ptrdiff_t n = this->gptr() - this->eback();
        ptrdiff_t e = this->egptr() - this->eback();
        this->setg((char_type*)extbuf_min_, (char_type*)extbuf_min_ + n,
                   (char_type*)extbuf_min_ + e);
    } else if (this->pbase() == (char_type*)rhs.extbuf_min_) {
        ptrdiff_t n = this->pptr() - this->pbase();
        ptrdiff_t e = this->epptr() - this->pbase();
        this->setp((char_type*)extbuf_min_, (char_type*)extbuf_min_ + e);
        this->pbump(n);
    }
    if (rhs.eback() == (char_type*)extbuf_min_) {
        ptrdiff_t n = rhs.gptr() - rhs.eback();
        ptrdiff_t e = rhs.egptr() - rhs.eback();
        rhs.setg((char_type*)rhs.extbuf_min_,
                 (char_type*)rhs.extbuf_min_ + n,
                 (char_type*)rhs.extbuf_min_ + e);
    } else if (rhs.pbase() == (char_type*)extbuf_min_) {
        ptrdiff_t n = rhs.pptr() - rhs.pbase();
        ptrdiff_t e = rhs.epptr() - rhs.pbase();
        rhs.setp((char_type*)rhs.extbuf_min_, (char_type*)rhs.extbuf_min_ + e);
        rhs.pbump(n);
    }
    std::swap(executor_, rhs.executor_);
}

template <class CharT, class Traits>
inline void swap(basic_filebuf<CharT, Traits>& x, basic_filebuf<CharT, Traits>& y)
{
    x.swap(y);
}

template <class CharT, class Traits>
inline bool basic_filebuf<CharT, Traits>::is_open() const
{
    return file_ != 0;
}

template <class CharT, class Traits>
basic_filebuf<CharT, Traits>* basic_filebuf<CharT, Traits>::open(const char* s,
                                                                 std::ios_base::openmode mode)
{
    basic_filebuf<CharT, Traits>* rt = 0;
    if (file_ == 0) {
        rt = this;
        const char* mdstr;
        switch (mode & ~std::ios_base::ate) {
        case std::ios_base::out:
        case std::ios_base::out | std::ios_base::trunc:
            mdstr = "w";
            break;
        case std::ios_base::out | std::ios_base::app:
        case std::ios_base::app:
            mdstr = "a";
            break;
        case std::ios_base::in:
            mdstr = "r";
            break;
        case std::ios_base::in | std::ios_base::out:
            mdstr = "r+";
            break;
        case std::ios_base::in | std::ios_base::out | std::ios_base::trunc:
            mdstr = "w+";
            break;
        case std::ios_base::in | std::ios_base::out | std::ios_base::app:
        case std::ios_base::in | std::ios_base::app:
            mdstr = "a+";
            break;
        case std::ios_base::out | std::ios_base::binary:
        case std::ios_base::out | std::ios_base::trunc | std::ios_base::binary:
            mdstr = "wb";
            break;
        case std::ios_base::out | std::ios_base::app | std::ios_base::binary:
        case std::ios_base::app | std::ios_base::binary:
            mdstr = "ab";
            break;
        case std::ios_base::in | std::ios_base::binary:
            mdstr = "rb";
            break;
        case std::ios_base::in | std::ios_base::out | std::ios_base::binary:
            mdstr = "r+b";
            break;
        case std::ios_base::in | std::ios_base::out | std::ios_base::trunc | std::ios_base::binary:
            mdstr = "w+b";
            break;
        case std::ios_base::in | std::ios_base::out | std::ios_base::app | std::ios_base::binary:
        case std::ios_base::in | std::ios_base::app | std::ios_base::binary:
            mdstr = "a+b";
            break;
        default:
            rt = 0;
            break;
        }
        if (rt) {
            file_ = (*executor_)(fopen, s, mdstr);
            if (file_) {
                om_ = mode;
                if (mode & std::ios_base::ate) {
                    if ((*executor_)(fseek, file_, 0, SEEK_END)) {
                        fclose(file_);
                        file_ = 0;
                        rt = 0;
                    }
                }
            } else
                rt = 0;
        }
    }
    return rt;
}

template <class CharT, class Traits>
inline basic_filebuf<CharT, Traits>*
basic_filebuf<CharT, Traits>::open(const std::string& s, std::ios_base::openmode mode)
{
    return open(s.c_str(), mode);
}

template <class CharT, class Traits>
basic_filebuf<CharT, Traits>* basic_filebuf<CharT, Traits>::close()
{
    basic_filebuf<CharT, Traits>* rt = 0;
    if (file_) {
        rt = this;
        std::unique_ptr<FILE, int (*)(FILE*)> h(file_, fclose);
        if (sync()) rt = 0;
        if (fclose(h.release()) == 0)
            file_ = 0;
        else
            rt = 0;
    }
    return rt;
}

template <class CharT, class Traits>
void basic_filebuf<CharT, Traits>::set_executor(std::shared_ptr<fibers::foreign_thread_pool> e)
{
    executor_ = e;
}

template <class CharT, class Traits>
typename basic_filebuf<CharT, Traits>::int_type basic_filebuf<CharT, Traits>::underflow()
{
    if (file_ == 0) return traits_type::eof();
    bool initial = read_mode();
    char_type buf1;
    if (this->gptr() == 0) this->setg(&buf1, &buf1 + 1, &buf1 + 1);
    const size_t unget_sz = initial ? 0 : std::min<size_t>((this->egptr() - this->eback()) / 2, 4);
    int_type c = traits_type::eof();
    if (this->gptr() == this->egptr()) {
        memmove(this->eback(), this->egptr() - unget_sz, unget_sz * sizeof(char_type));
        if (always_noconv_) {
            size_t nmemb = static_cast<size_t>(this->egptr() - this->eback() - unget_sz);
            nmemb = (*executor_)(fread, this->eback() + unget_sz, 1, nmemb, file_);
            if (nmemb != 0) {
                this->setg(this->eback(), this->eback() + unget_sz,
                           this->eback() + unget_sz + nmemb);
                c = traits_type::to_int_type(*this->gptr());
            }
        } else {
            memmove(extbuf_, extbufnext_, extbufend_ - extbufnext_);
            extbufnext_ = extbuf_ + (extbufend_ - extbufnext_);
            extbufend_ = extbuf_ + (extbuf_ == extbuf_min_ ? sizeof(extbuf_min_) : ebs_);
            size_t nmemb = std::min(static_cast<size_t>(ibs_ - unget_sz),
                                    static_cast<size_t>(extbufend_ - extbufnext_));
            std::codecvt_base::result r;
            st_last_ = st_;
            size_t nr = (*executor_)(fread, (void*)extbufnext_, 1, nmemb, file_);
            if (nr != 0) {
                if (!cv_) throw std::bad_cast();
                extbufend_ = extbufnext_ + nr;
                char_type* inext;
                r = cv_->in(st_, extbuf_, extbufend_, extbufnext_, this->eback() + unget_sz,
                            this->eback() + ibs_, inext);
                if (r == std::codecvt_base::noconv) {
                    this->setg((char_type*)extbuf_, (char_type*)extbuf_, (char_type*)extbufend_);
                    c = traits_type::to_int_type(*this->gptr());
                } else if (inext != this->eback() + unget_sz) {
                    this->setg(this->eback(), this->eback() + unget_sz, inext);
                    c = traits_type::to_int_type(*this->gptr());
                }
            }
        }
    } else
        c = traits_type::to_int_type(*this->gptr());
    if (this->eback() == &buf1) this->setg(0, 0, 0);
    return c;
}

template <class CharT, class Traits>
typename basic_filebuf<CharT, Traits>::int_type basic_filebuf<CharT, Traits>::pbackfail(int_type c)
{
    if (file_ && this->eback() < this->gptr()) {
        if (traits_type::eq_int_type(c, traits_type::eof())) {
            this->gbump(-1);
            return traits_type::not_eof(c);
        }
        if ((om_ & std::ios_base::out)
            || traits_type::eq(traits_type::to_char_type(c), this->gptr()[-1])) {
            this->gbump(-1);
            *this->gptr() = traits_type::to_char_type(c);
            return c;
        }
    }
    return traits_type::eof();
}

template <class CharT, class Traits>
typename basic_filebuf<CharT, Traits>::int_type basic_filebuf<CharT, Traits>::overflow(int_type c)
{
    if (file_ == 0) return traits_type::eof();
    write_mode();
    char_type buf1;
    char_type* pb_save = this->pbase();
    char_type* epb_save = this->epptr();
    if (!traits_type::eq_int_type(c, traits_type::eof())) {
        if (this->pptr() == 0) this->setp(&buf1, &buf1 + 1);
        *this->pptr() = traits_type::to_char_type(c);
        this->pbump(1);
    }
    if (this->pptr() != this->pbase()) {
        if (always_noconv_) {
            size_t nmemb = static_cast<size_t>(this->pptr() - this->pbase());
            if ((*executor_)(fwrite, this->pbase(), sizeof(char_type), nmemb, file_) != nmemb)
                return traits_type::eof();
        } else {
            char* extbe = extbuf_;
            std::codecvt_base::result r;
            do {
                if (!cv_) throw std::bad_cast();
                const char_type* e;
                r = cv_->out(st_, this->pbase(), this->pptr(), e, extbuf_, extbuf_ + ebs_, extbe);
                if (e == this->pbase()) return traits_type::eof();
                if (r == std::codecvt_base::noconv) {
                    size_t nmemb = static_cast<size_t>(this->pptr() - this->pbase());
                    if ((*executor_)(fwrite, this->pbase(), 1, nmemb, file_) != nmemb)
                        return traits_type::eof();
                } else if (r == std::codecvt_base::ok || r == std::codecvt_base::partial) {
                    size_t nmemb = static_cast<size_t>(extbe - extbuf_);
                    if ((*executor_)(fwrite, extbuf_, 1, nmemb, file_) != nmemb)
                        return traits_type::eof();
                    if (r == std::codecvt_base::partial) {
                        this->setp((char_type*)e, this->pptr());
                        this->pbump(this->epptr() - this->pbase());
                    }
                } else
                    return traits_type::eof();
            } while (r == std::codecvt_base::partial);
        }
        this->setp(pb_save, epb_save);
    }
    return traits_type::not_eof(c);
}

template <class CharT, class Traits>
std::basic_streambuf<CharT, Traits>* basic_filebuf<CharT, Traits>::setbuf(char_type* s,
                                                                          std::streamsize n)
{
    this->setg(0, 0, 0);
    this->setp(0, 0);
    if (owns_eb_) delete[] extbuf_;
    if (owns_ib_) delete[] intbuf_;
    ebs_ = n;
    if (ebs_ > sizeof(extbuf_min_)) {
        if (always_noconv_ && s) {
            extbuf_ = (char*)s;
            owns_eb_ = false;
        } else {
            extbuf_ = new char[ebs_];
            owns_eb_ = true;
        }
    } else {
        extbuf_ = extbuf_min_;
        ebs_ = sizeof(extbuf_min_);
        owns_eb_ = false;
    }
    if (!always_noconv_) {
        ibs_ = std::max<std::streamsize>(n, sizeof(extbuf_min_));
        if (s && ibs_ >= sizeof(extbuf_min_)) {
            intbuf_ = s;
            owns_ib_ = false;
        } else {
            intbuf_ = new char_type[ibs_];
            owns_ib_ = true;
        }
    } else {
        ibs_ = 0;
        intbuf_ = 0;
        owns_ib_ = false;
    }
    return this;
}

template <class CharT, class Traits>
typename basic_filebuf<CharT, Traits>::pos_type basic_filebuf<CharT, Traits>::seekoff(
    off_type off, std::ios_base::seekdir way, std::ios_base::openmode)
{
    if (!cv_) throw std::bad_cast();
    int width = cv_->encoding();
    if (file_ == 0 || (width <= 0 && off != 0) || sync()) return pos_type(off_type(-1));
    // width > 0 || off == 0
    int whence;
    switch (way) {
    case std::ios_base::beg:
        whence = SEEK_SET;
        break;
    case std::ios_base::cur:
        whence = SEEK_CUR;
        break;
    case std::ios_base::end:
        whence = SEEK_END;
        break;
    default:
        return pos_type(off_type(-1));
    }
#if defined(_WIN32) || defined(_NEWLIB_VERSION)
    if ((*executor_)(fseek, file_, width > 0 ? width * off : 0, whence))
        return pos_type(off_type(-1));
    pos_type r = ftell(file_);
#else
    if ((*executor_)(fseeko, file_, width > 0 ? width * off : 0, whence))
        return pos_type(off_type(-1));
    pos_type r = ftello(file_);
#endif
    r.state(st_);
    return r;
}

template <class CharT, class Traits>
typename basic_filebuf<CharT, Traits>::pos_type
basic_filebuf<CharT, Traits>::seekpos(pos_type sp, std::ios_base::openmode)
{
    if (file_ == 0 || sync()) return pos_type(off_type(-1));
#if defined(_WIN32) || defined(_NEWLIB_VERSION)
    if ((*executor_)(fseek, file_, sp, SEEK_SET)) return pos_type(off_type(-1));
#else
    if ((*executor_)(fseeko, file_, sp, SEEK_SET)) return pos_type(off_type(-1));
#endif
    st_ = sp.state();
    return sp;
}

template <class CharT, class Traits>
int basic_filebuf<CharT, Traits>::sync()
{
    if (file_ == 0) return 0;
    if (!cv_) throw std::bad_cast();
    if (cm_ & std::ios_base::out) {
        if (this->pptr() != this->pbase())
            if (overflow() == traits_type::eof()) return -1;
        std::codecvt_base::result r;
        do {
            char* extbe;
            r = cv_->unshift(st_, extbuf_, extbuf_ + ebs_, extbe);
            size_t nmemb = static_cast<size_t>(extbe - extbuf_);
            if ((*executor_)(fwrite, extbuf_, 1, nmemb, file_) != nmemb) return -1;
        } while (r == std::codecvt_base::partial);
        if (r == std::codecvt_base::error) return -1;
        if (fflush(file_)) return -1;
    } else if (cm_ & std::ios_base::in) {
        off_type c;
        state_type state = st_last_;
        bool update_st = false;
        if (always_noconv_)
            c = this->egptr() - this->gptr();
        else {
            int width = cv_->encoding();
            c = extbufend_ - extbufnext_;
            if (width > 0)
                c += width * (this->egptr() - this->gptr());
            else {
                if (this->gptr() != this->egptr()) {
                    const int off
                        = cv_->length(state, extbuf_, extbufnext_, this->gptr() - this->eback());
                    c += extbufnext_ - extbuf_ - off;
                    update_st = true;
                }
            }
        }
#if defined(_WIN32) || defined(_NEWLIB_VERSION)
        if ((*executor_)(fseek, file_, -c, SEEK_CUR)) return -1;
#else
        if ((*executor_)(fseeko, file_, -c, SEEK_CUR)) return -1;
#endif
        if (update_st) st_ = state;
        extbufnext_ = extbufend_ = extbuf_;
        this->setg(0, 0, 0);
        cm_ = std::ios_base::openmode(0);
    }
    return 0;
}

template <class CharT, class Traits>
void basic_filebuf<CharT, Traits>::imbue(const std::locale& loc)
{
    sync();
    cv_ = &std::use_facet<std::codecvt<char_type, char, state_type>>(loc);
    bool old_anc = always_noconv_;
    always_noconv_ = cv_->always_noconv();
    if (old_anc != always_noconv_) {
        this->setg(0, 0, 0);
        this->setp(0, 0);
        // invariant, char_type is char, else we couldn't get here
        if (always_noconv_) // need to dump intbuf_
        {
            if (owns_eb_) delete[] extbuf_;
            owns_eb_ = owns_ib_;
            ebs_ = ibs_;
            extbuf_ = (char*)intbuf_;
            ibs_ = 0;
            intbuf_ = 0;
            owns_ib_ = false;
        } else // need to obtain an intbuf_.
        { // If extbuf_ is user-supplied, use it, else new intbuf_
            if (!owns_eb_ && extbuf_ != extbuf_min_) {
                ibs_ = ebs_;
                intbuf_ = (char_type*)extbuf_;
                owns_ib_ = false;
                extbuf_ = new char[ebs_];
                owns_eb_ = true;
            } else {
                ibs_ = ebs_;
                intbuf_ = new char_type[ibs_];
                owns_ib_ = true;
            }
        }
    }
}

template <class CharT, class Traits>
bool basic_filebuf<CharT, Traits>::read_mode()
{
    if (!(cm_ & std::ios_base::in)) {
        this->setp(0, 0);
        if (always_noconv_)
            this->setg((char_type*)extbuf_, (char_type*)extbuf_ + ebs_, (char_type*)extbuf_ + ebs_);
        else
            this->setg(intbuf_, intbuf_ + ibs_, intbuf_ + ibs_);
        cm_ = std::ios_base::in;
        return true;
    }
    return false;
}

template <class CharT, class Traits>
void basic_filebuf<CharT, Traits>::write_mode()
{
    if (!(cm_ & std::ios_base::out)) {
        this->setg(0, 0, 0);
        if (ebs_ > sizeof(extbuf_min_)) {
            if (always_noconv_)
                this->setp((char_type*)extbuf_, (char_type*)extbuf_ + (ebs_ - 1));
            else
                this->setp(intbuf_, intbuf_ + (ibs_ - 1));
        } else
            this->setp(0, 0);
        cm_ = std::ios_base::out;
    }
}

// basic_ifstream

template <class CharT, class Traits = std::char_traits<CharT>>
class basic_ifstream : public std::basic_istream<CharT, Traits>
{
public:
    typedef CharT char_type;
    typedef Traits traits_type;
    typedef typename traits_type::int_type int_type;
    typedef typename traits_type::pos_type pos_type;
    typedef typename traits_type::off_type off_type;

    basic_ifstream();
    explicit basic_ifstream(const char* s, std::ios_base::openmode mode = std::ios_base::in);
    explicit basic_ifstream(const std::string& s, std::ios_base::openmode mode = std::ios_base::in);
    basic_ifstream(basic_ifstream&& rhs);

    basic_ifstream& operator=(basic_ifstream&& rhs);
    void swap(basic_ifstream& rhs);

    basic_filebuf<char_type, traits_type>* rdbuf() const;
    bool is_open() const;
    void open(const char* s, std::ios_base::openmode mode = std::ios_base::in);
    void open(const std::string& s, std::ios_base::openmode mode = std::ios_base::in);
    void close();

    void set_executor(std::shared_ptr<fibers::foreign_thread_pool> e) { sb_.set_executor(e); }

private:
    basic_filebuf<char_type, traits_type> sb_;
};

template <class CharT, class Traits>
inline basic_ifstream<CharT, Traits>::basic_ifstream()
: std::basic_istream<char_type, traits_type>(&sb_)
{
}

template <class CharT, class Traits>
inline basic_ifstream<CharT, Traits>::basic_ifstream(const char* s, std::ios_base::openmode mode)
: std::basic_istream<char_type, traits_type>(&sb_)
{
    if (sb_.open(s, mode | std::ios_base::in) == 0) this->setstate(std::ios_base::failbit);
}

template <class CharT, class Traits>
inline basic_ifstream<CharT, Traits>::basic_ifstream(const std::string& s,
                                                     std::ios_base::openmode mode)
: std::basic_istream<char_type, traits_type>(&sb_)
{
    if (sb_.open(s, mode | std::ios_base::in) == 0) this->setstate(std::ios_base::failbit);
}

template <class CharT, class Traits>
inline basic_ifstream<CharT, Traits>::basic_ifstream(basic_ifstream&& rhs)
: std::basic_istream<char_type, traits_type>(std::move(rhs)), sb_(std::move(rhs.sb_))
{
    this->set_rdbuf(&sb_);
}

template <class CharT, class Traits>
inline basic_ifstream<CharT, Traits>& basic_ifstream<CharT, Traits>::operator=(basic_ifstream&& rhs)
{
    std::basic_istream<char_type, traits_type>::operator=(std::move(rhs));
    sb_ = std::move(rhs.sb_);
    return *this;
}

template <class CharT, class Traits>
inline void basic_ifstream<CharT, Traits>::swap(basic_ifstream& rhs)
{
    std::basic_istream<char_type, traits_type>::swap(rhs);
    sb_.swap(rhs.sb_);
}

template <class CharT, class Traits>
inline void swap(basic_ifstream<CharT, Traits>& x, basic_ifstream<CharT, Traits>& y)
{
    x.swap(y);
}

template <class CharT, class Traits>
inline basic_filebuf<CharT, Traits>* basic_ifstream<CharT, Traits>::rdbuf() const
{
    return const_cast<basic_filebuf<char_type, traits_type>*>(&sb_);
}

template <class CharT, class Traits>
inline bool basic_ifstream<CharT, Traits>::is_open() const
{
    return sb_.is_open();
}

template <class CharT, class Traits>
void basic_ifstream<CharT, Traits>::open(const char* s, std::ios_base::openmode mode)
{
    if (sb_.open(s, mode | std::ios_base::in))
        this->clear();
    else
        this->setstate(std::ios_base::failbit);
}

template <class CharT, class Traits>
void basic_ifstream<CharT, Traits>::open(const std::string& s, std::ios_base::openmode mode)
{
    if (sb_.open(s, mode | std::ios_base::in))
        this->clear();
    else
        this->setstate(std::ios_base::failbit);
}

template <class CharT, class Traits>
inline void basic_ifstream<CharT, Traits>::close()
{
    if (sb_.close() == 0) this->setstate(std::ios_base::failbit);
}

// basic_ofstream

template <class CharT, class Traits = std::char_traits<CharT>>
class basic_ofstream : public std::basic_ostream<CharT, Traits>
{
public:
    typedef CharT char_type;
    typedef Traits traits_type;
    typedef typename traits_type::int_type int_type;
    typedef typename traits_type::pos_type pos_type;
    typedef typename traits_type::off_type off_type;

    basic_ofstream();
    explicit basic_ofstream(const char* s, std::ios_base::openmode mode = std::ios_base::out);
    explicit basic_ofstream(const std::string& s,
                            std::ios_base::openmode mode = std::ios_base::out);
    basic_ofstream(basic_ofstream&& rhs);

    basic_ofstream& operator=(basic_ofstream&& rhs);
    void swap(basic_ofstream& rhs);

    basic_filebuf<char_type, traits_type>* rdbuf() const;
    bool is_open() const;
    void open(const char* s, std::ios_base::openmode mode = std::ios_base::out);
    void open(const std::string& s, std::ios_base::openmode mode = std::ios_base::out);
    void close();

    void set_executor(std::shared_ptr<fibers::foreign_thread_pool> e) { sb_.set_executor(e); }

private:
    basic_filebuf<char_type, traits_type> sb_;
};

template <class CharT, class Traits>
inline basic_ofstream<CharT, Traits>::basic_ofstream()
: std::basic_ostream<char_type, traits_type>(&sb_)
{
}

template <class CharT, class Traits>
inline basic_ofstream<CharT, Traits>::basic_ofstream(const char* s, std::ios_base::openmode mode)
: std::basic_ostream<char_type, traits_type>(&sb_)
{
    if (sb_.open(s, mode | std::ios_base::out) == 0) this->setstate(std::ios_base::failbit);
}

template <class CharT, class Traits>
inline basic_ofstream<CharT, Traits>::basic_ofstream(const std::string& s,
                                                     std::ios_base::openmode mode)
: std::basic_ostream<char_type, traits_type>(&sb_)
{
    if (sb_.open(s, mode | std::ios_base::out) == 0) this->setstate(std::ios_base::failbit);
}

template <class CharT, class Traits>
inline basic_ofstream<CharT, Traits>::basic_ofstream(basic_ofstream&& rhs)
: std::basic_ostream<char_type, traits_type>(std::move(rhs)), sb_(std::move(rhs.sb_))
{
    this->set_rdbuf(&sb_);
}

template <class CharT, class Traits>
inline basic_ofstream<CharT, Traits>& basic_ofstream<CharT, Traits>::operator=(basic_ofstream&& rhs)
{
    std::basic_ostream<char_type, traits_type>::operator=(std::move(rhs));
    sb_ = std::move(rhs.sb_);
    return *this;
}

template <class CharT, class Traits>
inline void basic_ofstream<CharT, Traits>::swap(basic_ofstream& rhs)
{
    std::basic_ostream<char_type, traits_type>::swap(rhs);
    sb_.swap(rhs.sb_);
}

template <class CharT, class Traits>
inline void swap(basic_ofstream<CharT, Traits>& x, basic_ofstream<CharT, Traits>& y)
{
    x.swap(y);
}

template <class CharT, class Traits>
inline basic_filebuf<CharT, Traits>* basic_ofstream<CharT, Traits>::rdbuf() const
{
    return const_cast<basic_filebuf<char_type, traits_type>*>(&sb_);
}

template <class CharT, class Traits>
inline bool basic_ofstream<CharT, Traits>::is_open() const
{
    return sb_.is_open();
}

template <class CharT, class Traits>
void basic_ofstream<CharT, Traits>::open(const char* s, std::ios_base::openmode mode)
{
    if (sb_.open(s, mode | std::ios_base::out))
        this->clear();
    else
        this->setstate(std::ios_base::failbit);
}

template <class CharT, class Traits>
void basic_ofstream<CharT, Traits>::open(const std::string& s, std::ios_base::openmode mode)
{
    if (sb_.open(s, mode | std::ios_base::out))
        this->clear();
    else
        this->setstate(std::ios_base::failbit);
}

template <class CharT, class Traits>
inline void basic_ofstream<CharT, Traits>::close()
{
    if (sb_.close() == 0) this->setstate(std::ios_base::failbit);
}

// basic_fstream

template <class CharT, class Traits = std::char_traits<CharT>>
class basic_fstream : public std::basic_iostream<CharT, Traits>
{
public:
    typedef CharT char_type;
    typedef Traits traits_type;
    typedef typename traits_type::int_type int_type;
    typedef typename traits_type::pos_type pos_type;
    typedef typename traits_type::off_type off_type;

    basic_fstream();
    explicit basic_fstream(const char* s,
                           std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out);
    explicit basic_fstream(const std::string& s,
                           std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out);
    basic_fstream(basic_fstream&& rhs);

    basic_fstream& operator=(basic_fstream&& rhs);
    void swap(basic_fstream& rhs);

    basic_filebuf<char_type, traits_type>* rdbuf() const;
    bool is_open() const;
    void open(const char* s, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out);
    void open(const std::string& s,
              std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out);
    void close();

    void set_executor(std::shared_ptr<fibers::foreign_thread_pool> e) { sb_.set_executor(e); }

private:
    basic_filebuf<char_type, traits_type> sb_;
};

template <class CharT, class Traits>
inline basic_fstream<CharT, Traits>::basic_fstream()
: std::basic_iostream<char_type, traits_type>(&sb_)
{
}

template <class CharT, class Traits>
inline basic_fstream<CharT, Traits>::basic_fstream(const char* s, std::ios_base::openmode mode)
: std::basic_iostream<char_type, traits_type>(&sb_)
{
    if (sb_.open(s, mode) == 0) this->setstate(std::ios_base::failbit);
}

template <class CharT, class Traits>
inline basic_fstream<CharT, Traits>::basic_fstream(const std::string& s,
                                                   std::ios_base::openmode mode)
: std::basic_iostream<char_type, traits_type>(&sb_)
{
    if (sb_.open(s, mode) == 0) this->setstate(std::ios_base::failbit);
}

template <class CharT, class Traits>
inline basic_fstream<CharT, Traits>::basic_fstream(basic_fstream&& rhs)
: std::basic_iostream<char_type, traits_type>(std::move(rhs)), sb_(std::move(rhs.sb_))
{
    this->set_rdbuf(&sb_);
}

template <class CharT, class Traits>
inline basic_fstream<CharT, Traits>& basic_fstream<CharT, Traits>::operator=(basic_fstream&& rhs)
{
    std::basic_iostream<char_type, traits_type>::operator=(std::move(rhs));
    sb_ = std::move(rhs.sb_);
    return *this;
}

template <class CharT, class Traits>
inline void basic_fstream<CharT, Traits>::swap(basic_fstream& rhs)
{
    std::basic_iostream<char_type, traits_type>::swap(rhs);
    sb_.swap(rhs.sb_);
}

template <class CharT, class Traits>
inline void swap(basic_fstream<CharT, Traits>& x, basic_fstream<CharT, Traits>& y)
{
    x.swap(y);
}

template <class CharT, class Traits>
inline basic_filebuf<CharT, Traits>* basic_fstream<CharT, Traits>::rdbuf() const
{
    return const_cast<basic_filebuf<char_type, traits_type>*>(&sb_);
}

template <class CharT, class Traits>
inline bool basic_fstream<CharT, Traits>::is_open() const
{
    return sb_.is_open();
}

template <class CharT, class Traits>
void basic_fstream<CharT, Traits>::open(const char* s, std::ios_base::openmode mode)
{
    if (sb_.open(s, mode))
        this->clear();
    else
        this->setstate(std::ios_base::failbit);
}

template <class CharT, class Traits>
void basic_fstream<CharT, Traits>::open(const std::string& s, std::ios_base::openmode mode)
{
    if (sb_.open(s, mode))
        this->clear();
    else
        this->setstate(std::ios_base::failbit);
}

template <class CharT, class Traits>
inline void basic_fstream<CharT, Traits>::close()
{
    if (sb_.close() == 0) this->setstate(std::ios_base::failbit);
}

} // End of namespace stream

typedef stream::basic_ifstream<char> ifstream;
typedef stream::basic_ofstream<char> ofstream;
typedef stream::basic_fstream<char> fstream;

typedef stream::basic_ifstream<wchar_t> wifstream;
typedef stream::basic_ofstream<wchar_t> wofstream;
typedef stream::basic_fstream<wchar_t> wfstream;

} // End of namespace fibio
#endif // fibio_stream_fstream_hpp
