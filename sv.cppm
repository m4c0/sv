export module sv;

using size_t = decltype(sizeof(0));

template<typename T>
concept stringish = requires (T t, const char * c, size_t len) {
  c = t.data();
  len = t.size();
};

/// Holds a pointer to a string (aka char array) and its size. This is intended
/// to be a non-owning pointer holder. As consequences, it should not outlive
/// the original pointer.
export class sv {
  const char *m_data{};
  size_t m_len{};

public:
  constexpr sv() = default;
  constexpr sv(const char *v, size_t s) : m_data{v}, m_len{s} {}

  // TODO: find a way to avoid holding temporary variables
  // Example: sv invalid { std::string("mystr") };
  constexpr sv(const stringish auto & str) : m_data { str.data() }, m_len { str.size() } {}

  template <unsigned N> constexpr sv(const char (&c)[N]) : sv(c, N - 1) {}

  [[nodiscard]] constexpr auto data() const { return m_data; }
  [[nodiscard]] constexpr auto size() const { return m_len; }

  [[nodiscard]] constexpr auto begin() const { return m_data; }
  [[nodiscard]] constexpr auto end() const { return m_data + m_len; }

  [[nodiscard]] constexpr auto index_of(char c) const {
    for (auto i = 0; i < m_len; i++)
      if (m_data[i] == c) return i;

    return -1;
  }
  [[nodiscard]] constexpr bool starts_with(sv o) const {
    if (size() < o.size()) return false;

    for (auto i = 0; i < o.size(); i++)
      if (m_data[i] != o[i]) return false;

    return true;
  }
  [[nodiscard]] constexpr bool ends_with(sv o) const {
    if (size() < o.size()) return false;

    auto d = m_data + size() - o.size();
    for (auto i = 0; i < o.size(); i++)
      if (d[i] != o[i]) return false;

    return true;
  }

  [[nodiscard]] constexpr auto subsv(unsigned idx) const {
    struct pair {
      sv before;
      sv after;
    };
    if (idx >= m_len) return pair { *this, {} };
    return pair {
      .before { m_data, idx },
      .after { m_data + idx, m_len - idx },
    };
  }
  [[nodiscard]] constexpr auto subsv(unsigned idx, unsigned sz) const {
    struct trio {
      sv before;
      sv middle;
      sv after;
    };
    if (idx >= m_len) return trio { *this, {}, {} };

    auto [b, mm] = subsv(idx);
    if (idx + sz >= m_len) return trio { b, mm, {} };

    auto [m, a] = mm.subsv(sz);
    return trio { b, m, a };
  }
  [[nodiscard]] constexpr auto split(char c) const {
    struct pair {
      sv before;
      sv after;
    };
    for (auto i = 0U; i < m_len; i++) {
      if (m_data[i] != c) continue;

      return pair {
        .before { m_data, i },
        .after { m_data + i + 1, m_len - i - 1 }
      };
    }
    return pair { *this, {} };
  }
  [[nodiscard]] constexpr auto rsplit(char c) const {
    struct pair {
      sv before;
      sv after;
    };
    for (auto i = m_len; i > 0; i--) {
      auto j = i - 1;
      if (m_data[j] != c) continue;

      return pair{.before = {m_data, j},
                  .after = {m_data + j + 1, m_len - j - 1}};
    }
    return pair{{}, *this};
  }

  [[nodiscard]] constexpr char operator[](unsigned idx) const {
    if (idx >= m_len) return 0;
    return m_data[idx];
  }

  [[nodiscard]] constexpr sv trim() const {
    auto d = m_data;
    auto l = m_len;
    while (l > 0 && *d == ' ') {
      d++;
      l--;
    }
    while (l > 0 && d[l - 1] == ' ') --l;
    return { d, l };
  }

  [[nodiscard]] static constexpr sv unsafe(const char *str) {
    auto i = 0U;
    while (str[i]) i++;
    return sv{str, i};
  }
};

export [[nodiscard]] constexpr bool operator==(const sv &a, const sv &b) {
  if (a.size() != b.size()) return false;

  for (auto i = 0; i < a.size(); i++)
    if (a[i] != b[i]) return false;

  return true;
}
export [[nodiscard]] constexpr int operator<=>(const sv &a, const sv &b) {
  for (auto i = 0; i < a.size() && i < b.size(); i++) {
    if (a[i] == b[i]) continue;
    return a[i] < b[i] ? -1 : 1;
  }
  return a.size() - b.size();
}

export [[nodiscard]] constexpr sv operator""_sv(const char * v, size_t size) {
  return sv { v, size };
}

module :private;

static_assert("abcde"_sv[0] == 'a');
static_assert("abcde"_sv[3] == 'd');
static_assert("abcde"_sv[4] == 'e');

static_assert("a"_sv == "a"_sv);
static_assert("abacabb"_sv == "abacabb"_sv);

static_assert("check"_sv == sv{"check"});
static_assert("abacabb"_sv == sv::unsafe("abacabb"));

static_assert("aa"_sv != "aaaa"_sv);
static_assert("aaaa"_sv != "aa"_sv);
static_assert("a"_sv != "b"_sv);
static_assert("aaaaaaaa"_sv != "aaaaaaab"_sv);

static_assert("aabaa"_sv.index_of('b') == 2);
static_assert("aabaa"_sv.index_of('c') == -1);

static_assert("abcd"_sv.starts_with("ab"));
static_assert("abcd"_sv.starts_with("abcd"));
static_assert(!"abcd"_sv.starts_with("abcde"));
static_assert(!"x"_sv.starts_with("y"));

static_assert("abcd"_sv.ends_with("cd"));
static_assert("abcd"_sv.ends_with("abcd"));
static_assert(!"abcd"_sv.ends_with("abcde"));
static_assert(!"x"_sv.ends_with("y"));

static_assert([] {
  const auto &[a, b] = "jute"_sv.subsv(2);
  return a == "ju"_sv && b == "te"_sv;
}());

static_assert([] {
  const auto &[a, b, c] = "jute"_sv.subsv(2, 1);
  return a == "ju"_sv && b == "t"_sv && c == "e"_sv;
}());
static_assert([] {
  const auto &[a, b, c] = "jute"_sv.subsv(2, 0);
  return a == "ju"_sv && b == ""_sv && c == "te"_sv;
}());

static_assert([] {
  const auto &[a, b] = "love"_sv.split('/');
  return a == "love"_sv && b == ""_sv;
}());
static_assert([] {
  const auto &[a, b] = "jute twine etc"_sv.split(' ');
  return a == "jute"_sv && b == "twine etc"_sv;
}());

static_assert([] {
  const auto &[a, b] = "love"_sv.rsplit('/');
  return a == ""_sv && b == "love"_sv;
}());
static_assert([] {
  const auto &[a, b] = "jute twine etc"_sv.rsplit(' ');
  return a == "jute twine"_sv && b == "etc"_sv;
}());

static_assert("abc"_sv.trim() == "abc");
static_assert("  abc"_sv.trim() == "abc");
static_assert("abc   "_sv.trim() == "abc");
static_assert("   abc  "_sv.trim() == "abc");
static_assert("   abc  "_sv.trim() == "abc");
static_assert(" abc 234 "_sv.trim() == "abc 234");
static_assert("   "_sv.trim() == "");
static_assert(""_sv.trim() == "");

static_assert(  "abc"_sv  <  "bcd"_sv  );
static_assert(  "abc"_sv  <  "bcde"_sv );
static_assert(!("bcd"_sv  <  "abc"_sv ));
static_assert(!("bcde"_sv <  "abc"_sv ));
static_assert(  "bcd"_sv  >  "abc"_sv  );
static_assert(  "bcde"_sv >  "abc"_sv  );
static_assert(!("abc"_sv  >  "bcd"_sv ));
static_assert(!("abc"_sv  >  "bcde"_sv));
static_assert(  "abc"_sv  <= "abc"_sv  );
static_assert(!("abcd"_sv <= "abc"_sv ));
static_assert(  "abc"_sv  >= "abc"_sv  );
static_assert(!("abc"_sv  >= "abcd"_sv));
